#include "webserver.h"
#include "gps_data.h"

#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/ip4_addr.h"      // for IP4_ADDR

#include <cstdio>
#include <sstream>
#include <string>

extern "C" {
    #include "dhcpserver/dhcpserver.h"
    #include "dnsserver/dnsserver.h"
}


//— a single DHCP and DNS server instance must live for the life of the AP
static dhcp_server_t dhcp_server;
static dns_server_t  dns_server;

//————————————————————————————————————————————————————————————————————————
// TCP callbacks
//————————————————————————————————————————————————————————————————————————

static err_t on_sent(void* arg, struct tcp_pcb* tpcb, u16_t len) {
    tcp_close(tpcb);
    return ERR_OK;
}

static err_t tcp_recv_cb(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char* req = static_cast<char*>(p->payload);

    if (strncmp(req, "GET /data", 9) == 0) {
        std::ostringstream json;
        mutex_enter_blocking(&raw_data_mutex);
        GPSFix raw = raw_data;
        mutex_exit(&raw_data_mutex);

        mutex_enter_blocking(&filtered_data_mutex);
        GPSFix filtered = filtered_data;
        mutex_exit(&filtered_data_mutex);

        // Create JSON response
        json << "{"
             << "\"raw\": {\"lat\":" << raw.lat << ",\"lon\":" << raw.lon << "},"
             << "\"filtered\": {\"lat\":" << filtered.lat << ",\"lon\":" << filtered.lon << "}"
             << "}";

        std::string body = json.str();
        char hdr[128];
        int h = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %u\r\n"
            "\r\n",
            (unsigned)body.size());

        err_t err;
        err = tcp_write(tpcb, hdr, h, TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            pbuf_free(p);
            tcp_abort(tpcb);
            return ERR_ABRT;
        }

        err = tcp_write(tpcb, body.c_str(), body.size(), TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            pbuf_free(p);
            tcp_abort(tpcb);
            return ERR_ABRT;
        }

        tcp_output(tpcb);
        tcp_sent(tpcb, on_sent);
    } else if (strncmp(req, "GET / ", 6) == 0 || strncmp(req, "GET /HTTP", 9) == 0) {
        const char* html =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "\r\n"
            "<!DOCTYPE html><html><head><title>GPS Plot</title></head><body>"
            "<canvas id='canvas' width='800' height='600' style='border:1px solid black;'></canvas>"
            "<script>"
            "const canvas=document.getElementById('canvas');"
            "const ctx=canvas.getContext('2d');"
            "let raw=[], filtered=[];"
            "function draw() {"
            "ctx.clearRect(0,0,canvas.width,canvas.height);"
            "ctx.fillStyle='red';"
            "raw.forEach(p=>{ctx.beginPath();ctx.arc(p.x,p.y,2,0,2*Math.PI);ctx.fill();});"
            "ctx.strokeStyle='blue';ctx.beginPath();"
            "filtered.forEach((p,i)=>{i==0?ctx.moveTo(p.x,p.y):ctx.lineTo(p.x,p.y);});ctx.stroke();"
            "}"
            "function fetchData() {"
            "fetch('/data').then(r=>r.json()).then(d=>{"
            "let rx=(d.raw.lon+180)*(canvas.width/360);"
            "let ry=(90-d.raw.lat)*(canvas.height/180);"
            "raw.push({x:rx,y:ry});if(raw.length>1000)raw.shift();"
            "let fx=(d.filtered.lon+180)*(canvas.width/360);"
            "let fy=(90-d.filtered.lat)*(canvas.height/180);"
            "filtered.push({x:fx,y:fy});if(filtered.length>1000)filtered.shift();draw();});"
            "}"
            "setInterval(fetchData, 1000);"
            "</script></body></html>";

        err_t err = tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            pbuf_free(p);
            tcp_abort(tpcb);
            return ERR_ABRT;
        }

        tcp_output(tpcb);
        tcp_sent(tpcb, on_sent);
    } else {
        const char* not_found = "HTTP/1.1 404 Not Found\r\n\r\n";
        err_t err = tcp_write(tpcb, not_found, strlen(not_found), TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            pbuf_free(p);
            tcp_abort(tpcb);
            return ERR_ABRT;
        }

        tcp_output(tpcb);
        tcp_sent(tpcb, on_sent);
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_accept_cb(void* arg, struct tcp_pcb* newpcb, err_t err) {
    tcp_recv(newpcb, tcp_recv_cb);
    tcp_arg(newpcb, arg);
    return ERR_OK;
}

//————————————————————————————————————————————————————————————————————————
// WebServer implementation
//————————————————————————————————————————————————————————————————————————

WebServer::WebServer(
    GPSFix& fix,
    mutex_t* m,
    const char* mode,
    const char* ssid,
    const char* pw)
: context_{ &fix, m }, mode_(parse_mode(mode)), ssid_(ssid), pw_(pw) {}

WifiMode WebServer::parse_mode(const char* mode) {
    if (strcmp(mode, "AP") == 0) return WifiMode::AP;
    if (strcmp(mode, "STA") == 0) return WifiMode::STA;
    printf("Invalid Wi-Fi mode: %s\n", mode);
    return WifiMode::AP;
}

void WebServer::start() {

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_WORLDWIDE)) {
        printf("Wi-Fi init failed\n");
        return;
    }

    if (mode_ == WifiMode::AP) {
        printf("Starting AP mode with SSID \"%s\"\n", ssid_);
        // ————————————————— AP path —————————————————
        cyw43_arch_enable_ap_mode(ssid_, pw_, CYW43_AUTH_WPA2_AES_PSK);
        printf("AP mode on 192.168.4.1, SSID=\"%s\"\n", ssid_);
    
        // DHCP & DNS servers for clients on our AP
        ip4_addr_t gw, mask;
        IP4_ADDR(&gw,   192,168,4,1);
        IP4_ADDR(&mask,255,255,255,0);
        dhcp_server_init(&dhcp_server, (ip_addr_t*)&gw, (ip_addr_t*)&mask);
        dns_server_init(&dns_server,  (ip_addr_t*)&gw);
    
    } else {
        // ————————————————— STA path —————————————————
        printf("STA mode, connecting to %s\n", ssid_);
        cyw43_arch_enable_sta_mode();  // switch into station mode :contentReference[oaicite:0]{index=0}

        int r = cyw43_arch_wifi_connect_blocking(ssid_, pw_, CYW43_AUTH_WPA2_AES_PSK);
        if (r) {
            printf("STA connect failed (%d)\n", r);
            return;
        }
        // once joined, lwIP DHCP client will assign an IP to netif_default :contentReference[oaicite:1]{index=1}
        ip = netif_ip4_addr(netif_default);
        printf("STA mode, got IP %s\n", ip4addr_ntoa(ip));
    }

    // ————————————————— common TCP setup —————————————————
    struct tcp_pcb* pcb = tcp_new();
    if (!pcb || tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("tcp bind failed\n");
        return;
    }
    auto* lpcb = tcp_listen_with_backlog(pcb, 1);
    tcp_accept(lpcb, tcp_accept_cb);
    tcp_arg(lpcb, &context_);
    printf("HTTP server started\n");

}

void WebServer::poll() {
    // 1) service the CYW43 driver
    cyw43_arch_poll();
    // 2) drive lwIP timers (DHCP leasing, ARP, TCP timeouts…)
    sys_check_timeouts();
}

