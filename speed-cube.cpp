#include <stdio.h>
#include <cmath>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "L76B.h"
extern "C" {
    #include "DEV_Config.h"
    #include "LCD_Driver.h"
    #include "LCD_Touch.h"
    #include "LCD_GUI.h"
    #include "LCD_Bmp.h"
}


L76B l76b;

// Core 1 function: GPS processing
void core1_main() {
    // Initialize UART and interrupts on Core 1

    printf("Starting GPS on Core 1 with UART interrupts...\n");

    l76b.init();

    while (true) {
        sleep_ms(100);  // Allow interrupt-driven GPS reading
    }
}

// Core 0 function: Main program
int main() {
    // Initialize peripherals
    System_Init();

    // Initialize L76B GPS module
    multicore_launch_core1(core1_main);

    // Initialize SD card
    SD_Init();

    // Initialize LCD
    LCD_SCAN_DIR lcd_scan_dir = SCAN_DIR_DFT;
    LCD_Init(lcd_scan_dir, 800);
    GUI_Clear(LCD_BACKGROUND);


    
    GUI_DisString_EN(20, 20, "SOG", &Font24, LCD_BACKGROUND, WHITE);
    GUI_DisString_EN(120, 30, "kts", &Font24, LCD_BACKGROUND, WHITE);
    GUI_DisString_EN(20, 120, "Heading", &Font24, LCD_BACKGROUND, WHITE);
    GUI_DrawLine(0, 200, 320, 200, WHITE, LINE_SOLID, DOT_PIXEL_1X1);
    

    GUI_DisString_EN(20, 260, "VMG", &Font24, LCD_BACKGROUND, WHITE);
    GUI_DisString_EN(120, 280, "kts", &Font24, LCD_BACKGROUND, WHITE);
    GUI_DisString_EN(20, 360, "Wind Angle", &Font24, LCD_BACKGROUND, WHITE);

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

        GUI_DisString_EN(180, 0, timestamp, &Font24, BLACK, WHITE);

        // Print speed under SOG
        char speedStr[20];
        snprintf(speedStr, sizeof(speedStr), "%.2f", data.speed);
        GUI_DisString_EN(20, 40, speedStr, &Font24, LCD_BACKGROUND, WHITE);
        
        // Print heading under speed
        char headingStr[20];
        snprintf(headingStr, sizeof(headingStr), "%.2f", data.heading);
        GUI_DisString_EN(20, 140, headingStr, &Font24, LCD_BACKGROUND, WHITE);

        sleep_ms(1000);
    }
    
    return 0;
}