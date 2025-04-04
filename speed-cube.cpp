#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "LCD.h"
#include "L76B.h"
#include <cmath>


L76B l76b;
LCD lcd;

// Core 1 function: GPS processing
void core1_main() {
    // Initialize UART and interrupts on Core 1
    l76b.init();

    while (true) {
        sleep_ms(100);  // Allow interrupt-driven GPS reading
    }
}


int main() {
    // Initialize standard library
    stdio_init_all();
    printf("Starting GPS on Core 1 with UART interrupts...\n");

    multicore_launch_core1(core1_main);

    // Initialize the LCD
    lcd.init();
    lcd.setRotation(LCD_ROTATION_180);
    lcd.setContrast(0x40);  // Lower contrast
    lcd.clear(COLOR_BLACK);
    lcd.drawString(20, 20, "SOG", COLOR_WHITE, COLOR_BLACK, 2);
    lcd.drawString(120, 30, "kts", COLOR_WHITE, COLOR_BLACK, 2);

    lcd.drawString(20, 120, "Heading", COLOR_WHITE, COLOR_BLACK, 2);
    
    // Horizontal line
    lcd.drawLine(0, 200, 320, 200, COLOR_WHITE);

    lcd.drawString(20, 260, "VMG", COLOR_WHITE, COLOR_BLACK, 2);
    lcd.drawString(120, 280, "kts", COLOR_WHITE, COLOR_BLACK, 2);

    lcd.drawString(20, 360, "Wind Angle", COLOR_WHITE, COLOR_BLACK, 2);

    while (true) {
        L76B::GPSData data = l76b.getData();

        if (l76b.Status()) {
            printf(
                "Time: %.3f, Lat: %.6f, Lon: %.6f, Speed: %.2f knots, Heading: %.2f°\n",
                data.time, data.latitude, data.longitude, data.speed, data.heading
            );
        } else {
            printf("Waiting for valid GPS data...\n");
        }

        // Print timestamp
        char timestamp[10];
        snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d",
            (int)(data.time / 10000),
            (int)(fmod((data.time / 100), 100)),
            (int)(fmod(data.time, 100))
        );
        lcd.drawString(220, 0, timestamp, COLOR_WHITE, COLOR_BLACK, 2);

        // Print speed under SOG
        char speedStr[20];
        snprintf(speedStr, sizeof(speedStr), "%.2f", data.speed);
        lcd.drawString(20, 40, speedStr, COLOR_WHITE, COLOR_BLACK, 4);
        
        // Print heading under speed
        char headingStr[20];
        snprintf(headingStr, sizeof(headingStr), "%.2f", data.heading);
        lcd.drawString(20, 140, headingStr, COLOR_WHITE, COLOR_BLACK, 4);


        sleep_ms(1000);
    }
    
    return 0;
}