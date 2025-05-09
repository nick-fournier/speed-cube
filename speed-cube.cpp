#include <stdio.h>
#include <cmath>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "L76B.h"
#include "kalman.h"
#include "navigation/gui.h"
#include "navigation/control.h"


L76B l76b;
KalmanFilter kf;
NavigationController navCon(l76b, kf);
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

    while (true) {
        navCon.update();
        if (navCon.hasFix()) {
            navGui.update(navCon.getDisplayData());
        } else {
            printf("Waiting for GPS fix...\n");
        }
        sleep_ms(200);  // GUI update rate
    }

    while (true) {

        GPSData data = l76b.getData();
        // If GPS is ready, update the GUI
        if (l76b.Status()) {
            navGui.update(data);
        } else {
            printf("Waiting for valid GPS data...\n");
        }

       
        sleep_ms(200);
    }
    
    return 0;
}