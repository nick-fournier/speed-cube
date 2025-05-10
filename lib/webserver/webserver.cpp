#include "webserver.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include <sstream>
#include <string>

struct WebServerContext {
    GPSFix* fix;
    mutex_t* mutex;
};

static err_t tcp_recv_cb(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    auto* ctx = static_cast<WebServerContext*>(arg);
    std::ostringstream json;

    mutex_enter_blocking(ctx->mutex);
    json << "{"
         << "\"lat\":" << ctx->fix->lat << ","
         << "\"lon\":" << ctx->fix->lon << ","
         << "\"speed\":" << ctx->fix->speed << ","
         << "\"course\":" << ctx->fix->course
         << "}";
    mutex_exit(ctx->mutex);

    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + json.str();
    tcp_write(tpcb, response.c_str(), response.length(), TCP_WRITE_FLAG_COPY);

    pbuf_free(p);
    tcp_close(tpcb);
    return ERR_OK;
}

static err_t tcp_accept_cb(void* arg, struct tcp_pcb* newpcb, err_t err) {
    tcp_recv(newpcb, tcp_recv_cb);
    tcp_arg(newpcb, arg);
    return ERR_OK;
}

WebServer::WebServer(GPSFix& fix, mutex_t* mutex) {
    context_ = new WebServerContext{ &fix, mutex };
}

void WebServer::start() {
    struct tcp_pcb* pcb = tcp_new();
    tcp_bind(pcb, IP_ADDR_ANY, 80);
    struct tcp_pcb* listen_pcb = tcp_listen_with_backlog(pcb, 1);
    tcp_accept(listen_pcb, tcp_accept_cb);
    tcp_arg(listen_pcb, context_);
}
