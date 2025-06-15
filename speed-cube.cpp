#include <stdio.h>
#include <cmath>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"

#include "L76B.h"
#include "navigation/gui.h"
#include "webserver.h"
#include "gps_data.h"  // defines externs for filtered/raw data and mutexes
#include "config.h"

// Define the GPIO pin for the button
const uint BUTTON_PIN = 2;  // Using GPIO pin 2 as specified by user

// Volatile flag for interrupt-based button handling
static volatile bool button_pressed_flag = false;
static uint32_t last_button_time = 0;

// Interrupt handler for button press
void button_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Simple software debouncing in the interrupt handler
    if (current_time - last_button_time > 50) {  // 50ms debounce in interrupt
        button_pressed_flag = true;
        last_button_time = current_time;
        printf("Button press detected in interrupt at %u ms\n", current_time);
    }
}

L76B l76b;
KalmanFilter kf;
NavigationGUI navGui;


extern "C" {
    #include <malloc.h>
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
    slot.timestamp = raw_data.timestamp;

    // Update the raw and filtered data
    slot.raw.lat = raw_data.lat;
    slot.raw.lon = raw_data.lon;
    slot.raw.speed = raw_data.speed;
    slot.raw.course = raw_data.course;

    slot.filtered.lat = filtered_data.lat;
    slot.filtered.lon = filtered_data.lon;
    slot.filtered.speed = filtered_data.speed;
    slot.filtered.course = filtered_data.course;

    // Iterate the buffer index
    gps_buffer_index = (gps_buffer_index + 1) % GPS_BUFFER_SIZE;

    // Only increase count if buffer isn’t already full
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

    // Initialize the button pin with interrupt
    printf("Setting up button on GPIO %d with interrupt...\n", BUTTON_PIN);
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);  // Enable pull-up resistor
    
    // Set up the interrupt - trigger on falling edge (button press)
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    printf("Button setup with interrupt complete\n");

    // Start webserver using filtered data
    WebServer server(
        filtered_data,
        &filtered_data_mutex,
        WIFI_MODE,
        WIFI_SSID,
        WIFI_PASS
    );

    server.start();

    // Set time series update interval to 10 seconds
    navGui.setTimeSeriesUpdateInterval(10);
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

                // malloc_stats();

                update_gps_buffer(raw_snapshot, filtered_snapshot);
                last_logged_timestamp = raw_snapshot.timestamp;
            }

            navGui.update(raw_snapshot);
        } else {
            printf("Waiting for raw GPS fix...\n");
        }

        // Check if button press flag is set by the interrupt handler
        if (button_pressed_flag) {
            // Process the button press
            printf("Processing button press from interrupt, cycling to next target\n");
            navGui.cycleToNextTarget();
            
            // Clear the flag
            button_pressed_flag = false;
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

        sleep_ms(5);  // Reduced to 5ms for more frequent button polling
    }

    return 0;
}
