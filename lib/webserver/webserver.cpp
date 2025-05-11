#include "webserver.h"

#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/ip4_addr.h"      // for IP4_ADDR
#include "dhcpserver.h"
#include "dnsserver.h"

#include <cstdio>
#include <sstream>
#include <string>


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

WebServer::WebServer(GPSFix& fix, mutex_t* mutex)
    : context_{ &fix, mutex }
{}

void WebServer::start() {
    // 1) init Wi-Fi
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_WORLDWIDE)) {
        printf("WiFi init failed\n");
        return;
    }

    // 2) bring up AP — this also registers the netif as 192.168.4.1/24
    cyw43_arch_enable_ap_mode("PicoAP", "password123", CYW43_AUTH_WPA2_AES_PSK);
    printf("AP mode on 192.168.4.1, SSID=\"PicoAP\"\n");

    // 3) start DHCP + DNS on that interface
    //    netmask is 255.255.255.0
    ip4_addr_t gw, mask;
    IP4_ADDR(&gw,   192,168,4,1);
    IP4_ADDR(&mask,255,255,255,0);

    dhcp_server_init(&dhcp_server, (ip_addr_t*)&gw, (ip_addr_t*)&mask);
    dns_server_init(&dns_server,  (ip_addr_t*)&gw);

    // 4) start listening on TCP port 80
    struct tcp_pcb* pcb = tcp_new();
    if (!pcb) {
        printf("tcp_new failed\n");
        return;
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("tcp_bind failed\n");
        return;
    }
    auto* listen_pcb = tcp_listen_with_backlog(pcb, 1);
    tcp_accept(listen_pcb, tcp_accept_cb);
    tcp_arg(listen_pcb, &context_);

    printf("HTTP server started\n");
}

void WebServer::poll() {
    // 1) service the CYW43 driver
    cyw43_arch_poll();
    // 2) drive lwIP timers (DHCP leasing, ARP, TCP timeouts…)
    sys_check_timeouts();
}
