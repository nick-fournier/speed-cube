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


// Convert degrees to radians
constexpr float DEG_TO_RAD = 3.14159265 / 180.0;
static float prev_bearing_deg = -1;  // -1 indicates "no arrow drawn"

// Target coordinates
const struct {
    float lat;
    float lon;
} target = {
    37.77818748002346, -122.38120635348626
};


// Draw a simple "^" carrot or arrow at the given bearing around a circle
void drawCarrotAtBearing(float bearing_deg, int radius, int centerX, int centerY) {
    // Helper lambda to draw or erase
    auto drawArrow = [&](float angle_deg, int color) {
        float angle_rad = (angle_deg - 90) * DEG_TO_RAD;

        int outerRadius = radius + 4; // Tip of carrot just outside circle
        int baseRadius  = radius + 20; // Wider base further outside

        // Tip point
        int tipX = centerX + outerRadius * cos(angle_rad);
        int tipY = centerY + outerRadius * sin(angle_rad);

        // Base of the stubby carrot (wide and short)
        float baseAngleOffset = 0.1;  // Wider = bigger number
        int baseX1 = centerX + baseRadius * cos(angle_rad + baseAngleOffset);
        int baseY1 = centerY + baseRadius * sin(angle_rad + baseAngleOffset);
        int baseX2 = centerX + baseRadius * cos(angle_rad - baseAngleOffset);
        int baseY2 = centerY + baseRadius * sin(angle_rad - baseAngleOffset);

        // Fill in the entire carrot as a triangle
        GUI_DrawTriangle(
            tipX, tipY, baseX1, baseY1, baseX2, baseY2, color,
            DOT_PIXEL_1X1, DRAW_FULL);
    };

    static float prev_bearing_deg = -1;
    if (prev_bearing_deg >= 0) {
        drawArrow(prev_bearing_deg, BLACK); // Erase old
    }

    drawArrow(bearing_deg, WHITE); // Draw new
    prev_bearing_deg = bearing_deg;
}

// Calculate bearing between two points
float calculateBearing(float lat1, float lon1, float lat2, float lon2) {
    float dLon = (lon2 - lon1) * DEG_TO_RAD;
    lat1 *= DEG_TO_RAD;
    lat2 *= DEG_TO_RAD;

    float x = sin(dLon) * cos(lat2);
    float y = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
    float bearing = atan2(x, y);

    return fmod((bearing * 180.0 / M_PI + 360), 360); // Normalize to 0-360
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

    // Draw a circle in the top half of the screen
    // Screen is 320 x 480
    GUI_DrawCircle(160, 150, 120, WHITE, DRAW_EMPTY, DOT_PIXEL_2X2);
    GUI_DrawLine(0, 310, 320, 310, WHITE, LINE_SOLID, DOT_PIXEL_1X1);

    // Draw labels
    GUI_DisString_EN(100, 80, "kts (SOG)", &Font20, BLACK, WHITE);

    int test_heading = 0;

    while (true) {
        L76B::GPSData data = l76b.getData();

        // Draw the carrot at the current heading
        if (l76b.Status()) {

            // Calculate the bearing to the target
            float target_bearing = calculateBearing(
                data.latitude, data.longitude,
                target.lat, target.lon
            );

            // Draw target bearing indicator
            drawCarrotAtBearing(target_bearing, 120, 160, 150);

            // Show speed in center of circle
            char speedStr[20];
            snprintf(speedStr, sizeof(speedStr), "%.1f", data.speed);
            GUI_DisString_EN(75, 100, speedStr, &Font96, LCD_BACKGROUND, WHITE);
            // GUI_DisString_EN(20, 40, vmdStr, &Font24, LCD_BACKGROUND, WHITE);


            // Print timestamp
            char timestamp[10];
            snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d",
                (int)(data.time / 10000),
                (int)(fmod((data.time / 100), 100)),
                (int)(fmod(data.time, 100))
            );

            GUI_DisString_EN(0, 320, timestamp, &Font24, BLACK, WHITE);
            
            // Print heading under speed
            // char headingStr[20];
            // snprintf(headingStr, sizeof(headingStr), "%.2f", data.heading);
            // GUI_DisString_EN(20, 140, headingStr, &Font24, LCD_BACKGROUND, WHITE);



            printf(
                "Time: %.3f, Lat: %.6f, Lon: %.6f, Speed: %.2f knots, Heading: %.2f°\n",
                data.time, data.latitude, data.longitude, data.speed, data.heading
            );
        } else {
            printf("Waiting for valid GPS data...\n");
        }

       
        sleep_ms(200);
    }
    
    return 0;
}