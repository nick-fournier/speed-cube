#pragma once

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <string>

class GPSWebServer {
public:
    GPSWebServer(const std::string& ssid, const std::string& password);
    void start();
    void stop();
    void set_gps_data(const std::string& data);

private:
    std::string ssid_;
    std::string password_;
    std::string gps_data_;
    bool running_;

    static GPSWebServer* instance_;
    static void run_server_static();

    void run_server();
};
