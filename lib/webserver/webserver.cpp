#include "webserver.h"

#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/ip4_addr.h"      // for IP4_ADDR

#include <cstdio>
#include <sstream>
#include <string>


#ifdef __cplusplus
extern "C" {
#endif

#include "dhcpserver.h"
#include "dnsserver.h"

#ifdef __cplusplus
}
#endif

//— a single DHCP and DNS server instance must live for the life of the AP
static dhcp_server_t dhcp_server;
static dns_server_t  dns_server;

//————————————————————————————————————————————————————————————————————————
// TCP callbacks
//————————————————————————————————————————————————————————————————————————

static err_t tcp_recv_cb(void* arg, struct tcp_pcb* tpcb,
                        struct pbuf* p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    auto* ctx = static_cast<WebServerContext*>(arg);
    std::ostringstream json;

    // copy under lock
    mutex_enter_blocking(ctx->mutex);
    json << "{"
         << "\"lat\":"    << ctx->fix->lat    << ","
         << "\"lon\":"    << ctx->fix->lon    << ","
         << "\"speed\":"  << ctx->fix->speed  << ","
         << "\"course\":" << ctx->fix->course
         << "}";
    mutex_exit(ctx->mutex);

    std::string body = json.str();
    char hdr[128];
    int h = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %u\r\n"
        "\r\n",
        (unsigned)body.size());

    tcp_write(tpcb, hdr, h, TCP_WRITE_FLAG_COPY);
    tcp_write(tpcb, body.c_str(), body.size(), TCP_WRITE_FLAG_COPY);

    pbuf_free(p);
    tcp_close(tpcb);
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

WebServer::WebServer(GPSFix& fix, mutex_t* m,
    const char* mode,
    const char* ssid,
    const char* pw)
: context_{ &fix, m }
, mode_(parse_mode(mode))
, ssid_(ssid)
, pw_(pw)
{}

WifiMode WebServer::parse_mode(const char* mode) {
    if (strcmp(mode, "AP") == 0) {
        return WifiMode::AP;
    } else if (strcmp(mode, "STA") == 0) {
        return WifiMode::STA;
    } else {
        printf("Invalid Wi-Fi mode: %s\n", mode);
        return WifiMode::AP;  // default to AP
    }
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
        const ip4_addr_t* ip = netif_ip4_addr(netif_default);
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
