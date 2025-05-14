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


extern "C" {
    #include <malloc.h>
}

void print_heap_stats() {
    malloc_stats();  // prints to stdout
}

// Core 1: GPS handling
void core1_main() {
    printf("Starting GPS on Core 1 with UART interrupts...\n");
    l76b.init();

    while (true) {
        sleep_ms(100);
    }
}

// Function to update the GPS buffer with both raw and filtered data
void update_gps_buffer(
    const GPSFix& raw_data,
    const GPSFix& filtered_data
) {
    mutex_enter_blocking(&gps_buffer_mutex);

    // Get the current buffer slot
    GPSBuffer& slot = gps_buffer[gps_buffer_index];

    // Update the slot with the latest raw and filtered data
    slot.raw.lat = raw_data.lat;
    slot.raw.lon = raw_data.lon;
    slot.raw.speed = raw_data.speed;
    slot.raw.course = raw_data.course;
    slot.raw.timestamp = raw_data.timestamp;
    slot.raw.status = raw_data.status;

    slot.filtered.lat = filtered_data.lat;
    slot.filtered.lon = filtered_data.lon;
    slot.filtered.speed = filtered_data.speed;
    slot.filtered.course = filtered_data.course;
    slot.filtered.timestamp = filtered_data.timestamp;
    slot.filtered.status = filtered_data.status;

    // Iterate the buffer index
    gps_buffer_index = (gps_buffer_index + 1) % GPS_BUFFER_SIZE;

    // ✅ Only increase count if buffer isn’t already full
    if (gps_buffer_count < GPS_BUFFER_SIZE) {
        gps_buffer_count++;
    }

    mutex_exit(&gps_buffer_mutex);
}


int main() {
    stdio_init_all();
    sleep_ms(1000);  // Allow USB CDC to settle for serial output
    printf("Booting Speed-Cube system...\n");

    // Initialize mutexes
    printf("Initializing filtered_mutex...\n");
    mutex_init(&filtered_data_mutex);
    mutex_init(&raw_data_mutex);
    mutex_init(&gps_buffer_mutex);

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
    static uint32_t last_logged_timestamp = 0;

    while (true) {
        // Poll the Wi-Fi stack
        server.poll();

        GPSFix raw_snapshot;
        GPSFix filtered_snapshot;

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

            // Update the GPS buffer with both raw and filtered data
            // for ever 5 seconds
            if (
                raw_snapshot.timestamp % 5 == 0 &&
                raw_snapshot.timestamp != last_logged_timestamp
            ) {

                // print_heap_stats();

                update_gps_buffer(raw_snapshot, filtered_snapshot);
                last_logged_timestamp = raw_snapshot.timestamp;
            }

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
