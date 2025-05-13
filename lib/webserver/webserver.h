#pragma once

#include "gps_data.h"
#include "pico/sync.h"
#include "lwip/ip4_addr.h"      // for IP4_ADDR

enum class WifiMode { AP, STA };

struct WebServerContext {
    GPSFix*    fix;
    mutex_t*   mutex;
};

class WebServer {
public:
    // For AP mode: pass nullptr/empty for ssid/pw
    WebServer(
        GPSFix& fix, mutex_t* mutex,
        const char* mode = "AP",
        const char* ssid = "PicoAP",
        const char* pw   = "password123"
    );
    void start();
    void poll();
    void ip_info();

private:
    WebServerContext context_;
    WifiMode     mode_;
    const char*  ssid_;
    const char*  pw_;
    const ip4_addr_t* ip = nullptr;
    WifiMode parse_mode(const char* mode);
    
};
