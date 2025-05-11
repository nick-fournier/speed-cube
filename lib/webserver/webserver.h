#pragma once

#include "gps_data.h"
#include "pico/sync.h"

struct WebServerContext {
    GPSFix* fix;
    mutex_t* mutex;
};

class WebServer {
public:
    WebServer(GPSFix& fix, mutex_t* mutex);
    void start();
    void poll();    // call this every loop

private:
    WebServerContext context_;
};
