#include "webserver.h"
#include "lwip/tcp.h"
#include "pico/multicore.h"

GPSWebServer* GPSWebServer::instance_ = nullptr;

static err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

GPSWebServer::GPSWebServer(const std::string& ssid, const std::string& password)
    : ssid_(ssid), password_(password), gps_data_("{}"), running_(false) {}

void GPSWebServer::start() {
    running_ = true;
    instance_ = this;
    cyw43_arch_enable_ap_mode(ssid_.c_str(), password_.c_str(), CYW43_AUTH_WPA2_AES_PSK);
    multicore_launch_core1(run_server_static);
}

void GPSWebServer::stop() {
    running_ = false;
    cyw43_arch_disable_ap_mode();
}

void GPSWebServer::set_gps_data(const std::string& data) {
    gps_data_ = data;
}

void GPSWebServer::run_server_static() {
    if (instance_) instance_->run_server();
}

static std::string* current_gps_data_ptr = nullptr;

void GPSWebServer::run_server() {
    struct tcp_pcb *pcb = tcp_new();
    tcp_bind(pcb, IP_ADDR_ANY, 80);
    pcb = tcp_listen(pcb);
    current_gps_data_ptr = &gps_data_;
    tcp_accept(pcb, http_accept);

    while (running_) {
        cyw43_arch_poll();
        sleep_ms(10);
    }
}

static err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) return ERR_OK;
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + *current_gps_data_ptr;
    tcp_write(tpcb, response.c_str(), response.size(), TCP_WRITE_FLAG_COPY);
    tcp_close(tpcb);
    pbuf_free(p);
    return ERR_OK;
}
