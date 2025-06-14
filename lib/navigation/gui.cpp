#include "gui.h"


NavigationGUI::NavigationGUI() {}

void NavigationGUI::init() {
    // Initialize the GUI

    // Initialize Waveshare LCD peripherals
    System_Init();

    // Initialize SD card
    SD_Init();

    // Initialize LCD
    LCD_SCAN_DIR lcd_scan_dir = SCAN_DIR_DFT;
    LCD_Init(lcd_scan_dir, 800);
    GUI_Clear(LCD_BACKGROUND);

    // Draw a circle in the top half of the screen
    // Screen is 320 x 480
    GUI_DrawCircle(160, 150, 130, WHITE, DRAW_EMPTY, DOT_PIXEL_2X2);
    GUI_DrawLine(0, 310, 320, 310, WHITE, LINE_SOLID, DOT_PIXEL_1X1);

    // Draw labels
    GUI_DisString_EN(100, 60, "kts (VMG)", &Font20, BLACK, WHITE);
    GUI_DisString_EN(100, 175, "kts (SOG)", &Font20, BLACK, WHITE);
}

void NavigationGUI::update(GPSFix Data) {

    // Update the current bearing to the target mark
    float target_bearing = calculateBearing(
        Data.lat, Data.lon,
        current_target.lat, current_target.lon
    );

    // Draw target bearing indicator
    updateMarkPointer(target_bearing);

    // Calculate VMG
    float vmg = calculateVMG(Data.speed, Data.course, target_bearing);

    // Format speed floats as strings
    char speedStr[8];
    char vmgStr[8];

    // Store the sign of VMG as a string
    char vmg_sign[2] = { (vmg < 0 ? '-' : ' '), '\0' };
    // float vmg_sign = vmg < 0 ? -1 : 1;
    vmg = fabs(vmg);

    snprintf(speedStr, sizeof(speedStr), "%.1f", Data.speed);
    snprintf(vmgStr, sizeof(vmgStr), "%.1f", vmg);
    
    // Show VMG in top half of circle
    GUI_DisString_EN(100, 75, vmgStr, &Font72, LCD_BACKGROUND, WHITE);

    // Show the sign left of the VMG, to keep centered
    GUI_DisString_EN(60, 75, vmg_sign, &Font72, LCD_BACKGROUND, WHITE);

    // Show speed in bottom half of circle
    GUI_DisString_EN(120, 200, speedStr, &Font48, LCD_BACKGROUND, WHITE);

    // Print timestamp
    char time_str[10];
    
    // If Data.timestamp is 0, report "No GPS fix" to display
    if (Data.timestamp == 0) {
        snprintf(time_str, sizeof(time_str), "No GPS...");
        GUI_DisString_EN(0, 320, time_str, &Font24, BLACK, WHITE);
    } else {
        time_from_epoch(Data.timestamp, time_str, sizeof(time_str));
        GUI_DisString_EN(0, 320, time_str, &Font24, BLACK, WHITE);
    }
    
    // Print course under speed
    // char courseStr[20];
    // snprintf(courseStr, sizeof(courseStr), "%.2f", data.course);
    // GUI_DisString_EN(20, 140, courseStr, &Font24, LCD_BACKGROUND, WHITE);

    // printf(
    //     "Time: %.3f, Lat: %.6f, Lon: %.6f, Speed: %.2f knots, Course: %.2f°\n",
    //     Data.time, Data.lat, Data.lon, Data.speed, Data.course
    // );
}

void NavigationGUI::updateMarkPointer(float bearing_deg)  {
    // Erase old pointer
    if (prev_mark_deg >= 0) {
        GUI_DrawRadialCircle(prev_mark_deg, 10, centerX, centerY, radius + 15, YELLOW);
    }

    // Draw new pointer
    GUI_DrawRadialCircle(bearing_deg, 10, centerX, centerY, radius + 15, YELLOW);
    prev_mark_deg = bearing_deg;
}

void NavigationGUI::updateTackPointer(float bearing_deg) {

    // If bearing on starboard, make pointer green
    if (bearing_deg > 180) {
        GUI_DrawRadialTriangle(bearing_deg, radius - 5, centerX, centerY, GREEN, 1);
    } else {
        GUI_DrawRadialTriangle(bearing_deg, radius - 5, centerX, centerY, RED, 1);
    }

    // Erase old pointer
    if (prev_tack_deg >= 0) {
        GUI_DrawRadialTriangle(prev_tack_deg, radius - 5, centerX, centerY, BLACK, 1);
    }

    // Draw new pointer
    GUI_DrawRadialTriangle(bearing_deg, radius  - 5, centerX, centerY, WHITE, 1);
    prev_tack_deg = bearing_deg;
}


// Calculate bearing between two points
float NavigationGUI::calculateBearing(float lat1, float lon1, float lat2, float lon2) {
    float dLon = (lon2 - lon1) * DEG2RAD;
    lat1 *= DEG2RAD;
    lat2 *= DEG2RAD;

    float x = sin(dLon) * cos(lat2);
    float y = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
    float bearing = atan2(x, y);

    return fmod((bearing * RAD2DEG + 360), 360); // Normalize to 0-360
}

// Calculate VMG to mark
float NavigationGUI::calculateVMG(float speed, float course, float target_bearing) {
    // Convert degrees to radians
    float course_rad = course * DEG2RAD;
    float target_bearing_rad = target_bearing * DEG2RAD;

    // Calculate VMG
    return speed * cos(course_rad - target_bearing_rad);
}