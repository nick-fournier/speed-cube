#include <stdio.h>
#include <cmath>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "L76B.h"
#include "navigation/gui.h"
// #include "webserver.h"


L76B l76b;
KalmanFilter kf;
NavigationGUI navGui;

// Core 1 function: GPS processing
void core1_main() {
    // Initialize UART and interrupts on Core 1

    printf("Starting GPS on Core 1 with UART interrupts...\n");

    l76b.init();

    // Stay alive and process GPS data asynchronously
    // UART interrupt will call l76b.on_uart_rx() when data is received
    while (true) {
        sleep_ms(100);
    }
}


// Core 0 function: Main program
int main() {
    navGui.init();

    // Initialize L76B GPS module
    multicore_launch_core1(core1_main);

    GPSFix raw_snapshot;
    GPSFix filtered_snapshot;

    // Read raw GPS data. Comment me out once tuned
    mutex_enter_blocking(&raw_data_mutex);
    raw_snapshot = raw_data;
    mutex_exit(&raw_data_mutex);

    // Read filtered GPS data
    mutex_enter_blocking(&filtered_mutex);
    filtered_snapshot = filtered_data;
    mutex_exit(&filtered_mutex);

    if (filtered_data.status) {
        navGui.update(raw_snapshot);
    } else {
        printf("Waiting for GPS fix...\n");
    }

    sleep_ms(200);  // GUI update rate
    
    return 0;
}