#include <stdio.h>
#include <cmath>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "pico/cyw43_arch.h"

#include "L76B.h"
#include "navigation/gui.h"
#include "webserver.h"
#include "gps_data.h"  // defines externs for filtered/raw data and mutexes
#include "config.h"

L76B l76b;
KalmanFilter kf;
NavigationGUI navGui;


// Core 1: GPS handling
void core1_main() {
    printf("Starting GPS on Core 1 with UART interrupts...\n");
    l76b.init();

    while (true) {
        sleep_ms(100);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(1000);  // Allow USB CDC to settle for serial output
    printf("Booting Speed-Cube system...\n");

    // Initialize mutexes
    printf("Initializing filtered_mutex...\n");
    mutex_init(&filtered_data_mutex);
    mutex_init(&raw_data_mutex);

    // Start webserver using filtered data
    WebServer server(
        filtered_data,
        &filtered_data_mutex,
        WIFI_MODE,
        WIFI_SSID,
        WIFI_PASS
    );

    server.start();

    navGui.init();
    multicore_launch_core1(core1_main);

    while (true) {
        // Poll the Wi-Fi stack
        server.poll();

        GPSFix raw_snapshot;
        GPSFix filtered_snapshot;

        // Read working data
        // raw_snapshot = l76b.getData();

        // Read raw GPS data
        mutex_enter_blocking(&raw_data_mutex);
        raw_snapshot = raw_data;
        mutex_exit(&raw_data_mutex);

        // Read filtered GPS data
        mutex_enter_blocking(&filtered_data_mutex);
        filtered_snapshot = filtered_data;
        mutex_exit(&filtered_data_mutex);

        // Update GUI using raw GPS fix
        if (raw_snapshot.status) {
            navGui.update(raw_snapshot);
        } else {
            printf("Waiting for raw GPS fix...\n");
        }

        // Print both to the console for debugging
        // printf("[RAW] time: %.2f, lat: %.6f, lon: %.6f, speed: %.2f, course: %.2f\n",
        //     raw_snapshot.time,
        //     raw_snapshot.lat, raw_snapshot.lon,
        //     raw_snapshot.speed, raw_snapshot.course);

        // printf("[FILTERED] time: %.2f, lat: %.6f, lon: %.6f, speed: %.2f, course: %.2f\n",
        //     filtered_snapshot.time,
        //     filtered_snapshot.lat, filtered_snapshot.lon,
        //     filtered_snapshot.speed, filtered_snapshot.course);

        sleep_ms(200);  // GUI + print update rate
    }

    return 0;
}
