#include "webserver.h"
#include "gps_data.h"
#include "index_html.h"
#include "uPlot_iife_min_js.h"
#include "uPlot_min_css.h"

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

struct StreamedHttpResponse {
    const uint8_t* data;
    size_t total_len;
    size_t offset;
    const char* content_type;
};


//————————————————————————————————————————————————————————————————————————
// Helper functions
//————————————————————————————————————————————————————————————————————————

std::string get_html_page() {
    return std::string(reinterpret_cast<const char*>
        (index_html),
        index_html_len
    );
}

uint32_t extract_after_timestamp(const char* req) {
    const char* query = strstr(req, "?after=");
    if (!query) return 0;

    uint32_t val = 0;
    if (sscanf(query + 7, "%u", &val) == 1) {
        return val;
    }
    return 0;
}

//————————————————————————————————————————————————————————————————————————
// TCP callbacks
//————————————————————————————————————————————————————————————————————————

static err_t on_sent(void* arg, struct tcp_pcb* tpcb, u16_t len) {
    tcp_close(tpcb);
    return ERR_OK;
}

static err_t on_stream_sent(void* arg, struct tcp_pcb* tpcb, u16_t len) {
    auto* resp = static_cast<StreamedHttpResponse*>(arg);
    const size_t chunk_size = 1024;

    while (resp->offset < resp->total_len) {
        size_t remaining = resp->total_len - resp->offset;
        size_t to_write = std::min(chunk_size, remaining);
        err_t err = tcp_write(tpcb, resp->data + resp->offset, to_write, TCP_WRITE_FLAG_COPY);
        if (err == ERR_OK) {
            resp->offset += to_write;
        } else if (err == ERR_MEM) {
            // Try again after next sent event
            return ERR_OK;
        } else {
            printf("Streaming error: %d\n", err);
            delete resp;
            tcp_abort(tpcb);
            return ERR_ABRT;
        }
    }

    if (resp->offset >= resp->total_len) {
        delete resp;
        tcp_arg(tpcb, nullptr);
    }

    tcp_output(tpcb);
    return ERR_OK;
}

static void send_http_response(struct tcp_pcb* tpcb, const std::string& body, const char* content_type = "application/json") {
    char hdr[128];
    int h = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %u\r\n"
        "\r\n",
        content_type, (unsigned)body.size());

    if (tcp_write(tpcb, hdr, h, TCP_WRITE_FLAG_COPY) != ERR_OK ||
        tcp_write(tpcb, body.c_str(), body.size(), TCP_WRITE_FLAG_COPY) != ERR_OK) {
        tcp_abort(tpcb);
        return;
    }
    tcp_output(tpcb);
    tcp_sent(tpcb, on_sent);
}

void send_streaming_http_response(struct tcp_pcb* tpcb, const uint8_t* data, size_t len, const char* content_type) {
    char hdr[128];
    int h = snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "\r\n", content_type, len);

    err_t err = tcp_write(tpcb, hdr, h, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("Failed to write header: %d\n", err);
        tcp_abort(tpcb);
        return;
    }

    auto* resp = new StreamedHttpResponse {
        .data = data,
        .total_len = len,
        .offset = 0,
        .content_type = content_type
    };

    tcp_arg(tpcb, resp);
    tcp_sent(tpcb, on_stream_sent);

    // Kickstart the first chunk
    on_stream_sent(resp, tpcb, 0);
}



static err_t tcp_recv_cb(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char* req = static_cast<char*>(p->payload);

    auto extract_after_timestamp = [](const char* req) -> uint32_t {
        const char* query = strstr(req, "?after=");
        if (!query) return 0;
        uint32_t val = 0;
        if (sscanf(query + 7, "%u", &val) == 1) return val;
        return 0;
    };

    if (strncmp(req, "GET /location", 9) == 0) {
        std::ostringstream json;
        mutex_enter_blocking(&raw_data_mutex);
        GPSFix raw = raw_data;
        mutex_exit(&raw_data_mutex);

        mutex_enter_blocking(&filtered_data_mutex);
        GPSFix filtered = filtered_data;
        mutex_exit(&filtered_data_mutex);

        json << "{"
             << "\"raw\": {\"lat\":" << raw.lat << ",\"lon\":" << raw.lon << "},"
             << "\"filtered\": {\"lat\":" << filtered.lat << ",\"lon\":" << filtered.lon << "}"
             << "}";

        // send_http_response(tpcb, json.str());
        
        std::string body = json.str();
        size_t len = body.size();

        auto* copy = new uint8_t[len];
        memcpy(copy, body.c_str(), len);

        send_streaming_http_response(tpcb, copy, len, "application/json");

    } else if (strncmp(req, "GET /buffer", 11) == 0) {
        uint32_t after_ts = extract_after_timestamp(req);
        std::ostringstream json;
        json << "[";

        mutex_enter_blocking(&gps_buffer_mutex);
        size_t head = gps_buffer_index;
        size_t count = gps_buffer_count;
        bool first = true;

        for (size_t i = 0; i < count; ++i) {
            size_t idx = (head + GPS_BUFFER_SIZE - count + i) % GPS_BUFFER_SIZE;
            const GPSFix& fix = gps_buffer[idx];

            if (after_ts == 0 || fix.timestamp > after_ts) {
                if (!first) json << ",";
                json << "{\"lat\":" << fix.lat
                     << ",\"lon\":" << fix.lon
                     << ",\"timestamp\":" << fix.timestamp << "}";
                first = false;
            }
        }
        mutex_exit(&gps_buffer_mutex);

        json << "]";
        send_http_response(tpcb, json.str());

    } else if (strncmp(req, "GET / ", 6) == 0 || strncmp(req, "GET /HTTP", 9) == 0) {
        std::string html = get_html_page();
        send_http_response(tpcb, html, "text/html");

    } else if (strncmp(req, "GET /uplot.js", 13) == 0) {
        send_streaming_http_response(
            tpcb,
            uPlot_iife_min_js,
            uPlot_iife_min_js_len,
            "application/javascript"
        );
        tcp_recved(tpcb, p->tot_len);
        pbuf_free(p);
        return ERR_OK;

    } else if (strncmp(req, "GET /uplot.css", 14) == 0) {
        send_http_response(tpcb,
            std::string(reinterpret_cast<const char*>(uPlot_min_css), uPlot_min_css_len),
            "text/css");

    } else {
        const char* not_found = "HTTP/1.1 404 Not Found\r\n\r\n";
        tcp_write(tpcb, not_found, strlen(not_found), TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
        tcp_sent(tpcb, on_sent);
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}


// static err_t tcp_recv_cb(void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) {
//     if (!p) {
//         tcp_close(tpcb);
//         return ERR_OK;
//     }

//     char* req = static_cast<char*>(p->payload);

//     auto extract_after_timestamp = [](const char* req) -> uint32_t {
//         const char* query = strstr(req, "?after=");
//         if (!query) return 0;
//         uint32_t val = 0;
//         if (sscanf(query + 7, "%u", &val) == 1) return val;
//         return 0;
//     };

//     if (strncmp(req, "GET /data", 9) == 0) {
//         std::ostringstream json;
//         mutex_enter_blocking(&raw_data_mutex);
//         GPSFix raw = raw_data;
//         mutex_exit(&raw_data_mutex);

//         mutex_enter_blocking(&filtered_data_mutex);
//         GPSFix filtered = filtered_data;
//         mutex_exit(&filtered_data_mutex);

//         json << "{"
//              << "\"raw\": {\"lat\":" << raw.lat << ",\"lon\":" << raw.lon << "},"
//              << "\"filtered\": {\"lat\":" << filtered.lat << ",\"lon\":" << filtered.lon << "}"
//              << "}";

//         std::string body = json.str();
//         char hdr[128];
//         int h = snprintf(hdr, sizeof(hdr),
//             "HTTP/1.1 200 OK\r\n"
//             "Content-Type: application/json\r\n"
//             "Content-Length: %u\r\n"
//             "\r\n",
//             (unsigned)body.size());

//         if (tcp_write(tpcb, hdr, h, TCP_WRITE_FLAG_COPY) != ERR_OK ||
//             tcp_write(tpcb, body.c_str(), body.size(), TCP_WRITE_FLAG_COPY) != ERR_OK) {
//             pbuf_free(p);
//             tcp_abort(tpcb);
//             return ERR_ABRT;
//         }

//         tcp_output(tpcb);
//         tcp_sent(tpcb, on_sent);

//     } else if (strncmp(req, "GET /buffer", 11) == 0) {
//         uint32_t after_ts = extract_after_timestamp(req);
//         std::ostringstream json;
//         json << "[";

//         mutex_enter_blocking(&gps_buffer_mutex);
//         size_t head = gps_buffer_index;
//         size_t count = gps_buffer_count;
//         bool first = true;

//         for (size_t i = 0; i < count; ++i) {
//             size_t idx = (head + GPS_BUFFER_SIZE - count + i) % GPS_BUFFER_SIZE;
//             const GPSFix& fix = gps_buffer[idx];

//             if (fix.timestamp > after_ts) {
//                 if (!first) json << ",";
//                 json << "{\"lat\":" << fix.lat
//                      << ",\"lon\":" << fix.lon
//                      << ",\"timestamp\":" << fix.timestamp << "}";
//                 first = false;
//             }
//         }
//         mutex_exit(&gps_buffer_mutex);

//         json << "]";

//         std::string body = json.str();
//         char hdr[128];
//         int h = snprintf(hdr, sizeof(hdr),
//             "HTTP/1.1 200 OK\r\n"
//             "Content-Type: application/json\r\n"
//             "Content-Length: %u\r\n"
//             "\r\n",
//             (unsigned)body.size());

//         if (tcp_write(tpcb, hdr, h, TCP_WRITE_FLAG_COPY) != ERR_OK ||
//             tcp_write(tpcb, body.c_str(), body.size(), TCP_WRITE_FLAG_COPY) != ERR_OK) {
//             pbuf_free(p);
//             tcp_abort(tpcb);
//             return ERR_ABRT;
//         }

//         tcp_output(tpcb);
//         tcp_sent(tpcb, on_sent);

//     } else if (strncmp(req, "GET / ", 6) == 0 || strncmp(req, "GET /HTTP", 9) == 0) {
//         const char* html =
//             "HTTP/1.1 200 OK\r\n"
//             "Content-Type: text/html\r\n"
//             "\r\n"
//             "<!DOCTYPE html><html><head><title>GPS Plot</title></head><body>"
//             "<canvas id='canvas' width='800' height='600' style='border:1px solid black;'></canvas>"
//             "<script>"
//             "const canvas=document.getElementById('canvas');"
//             "const ctx=canvas.getContext('2d');"
//             "let raw=[], filtered=[];"
//             "let lastTimestamp=0;"
//             "function draw() {"
//             "ctx.clearRect(0,0,canvas.width,canvas.height);"
//             "ctx.fillStyle='red';"
//             "raw.forEach(p=>{ctx.beginPath();ctx.arc(p.x,p.y,2,0,2*Math.PI);ctx.fill();});"
//             "ctx.strokeStyle='blue';ctx.beginPath();"
//             "filtered.forEach((p,i)=>{i==0?ctx.moveTo(p.x,p.y):ctx.lineTo(p.x,p.y);});ctx.stroke();"
//             "}"
//             "function fetchData() {"
//             "fetch('/data').then(r=>r.json()).then(d=>{"
//             "let rx=(d.raw.lon+180)*(canvas.width/360);"
//             "let ry=(90-d.raw.lat)*(canvas.height/180);"
//             "raw.push({x:rx,y:ry});if(raw.length>1000)raw.shift();"
//             "let fx=(d.filtered.lon+180)*(canvas.width/360);"
//             "let fy=(90-d.filtered.lat)*(canvas.height/180);"
//             "filtered.push({x:fx,y:fy});if(filtered.length>1000)filtered.shift();draw();});"
//             "fetch(`/buffer?after=${lastTimestamp}`).then(r=>r.json()).then(points=>{"
//             "points.forEach(p=>{"
//             "let x=(p.lon+180)*(canvas.width/360);"
//             "let y=(90-p.lat)*(canvas.height/180);"
//             "filtered.push({x:x,y:y});if(filtered.length>1000)filtered.shift();"
//             "if(p.timestamp>lastTimestamp)lastTimestamp=p.timestamp;});draw();});"
//             "}"
//             "setInterval(fetchData, 1000);"
//             "</script></body></html>";

//         if (tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY) != ERR_OK) {
//             pbuf_free(p);
//             tcp_abort(tpcb);
//             return ERR_ABRT;
//         }

//         tcp_output(tpcb);
//         tcp_sent(tpcb, on_sent);

//     } else {
//         const char* not_found = "HTTP/1.1 404 Not Found\r\n\r\n";
//         if (tcp_write(tpcb, not_found, strlen(not_found), TCP_WRITE_FLAG_COPY) != ERR_OK) {
//             pbuf_free(p);
//             tcp_abort(tpcb);
//             return ERR_ABRT;
//         }
//         tcp_output(tpcb);
//         tcp_sent(tpcb, on_sent);
//     }

//     tcp_recved(tpcb, p->tot_len);
//     pbuf_free(p);
//     return ERR_OK;
// }


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

