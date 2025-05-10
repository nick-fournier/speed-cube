#pragma once

#include "pico/mutex.h"
#include "gps_data.h"

class WebServer {
public:
    WebServer(GPSFix& fix, mutex_t* mutex);
    void start();

private:
    struct WebServerContext* context_;
};
