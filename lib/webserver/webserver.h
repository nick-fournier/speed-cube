#pragma once

#include "gps_data.h"
#include "pico/sync.h"

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

private:
    WebServerContext context_;
    WifiMode     mode_;
    const char*  ssid_;
    const char*  pw_;
    WifiMode parse_mode(const char* mode);
};
