#include "gui.h"
#include "simulation.h"
#include "timeseries.h"
#include "pointers.h"

NavigationGUI::NavigationGUI() {
    // Create component objects
    m_timeSeries = new TimeSeriesPlot(this);
    m_simulation = new Simulation(this, m_timeSeries);
    m_pointers = new Pointers(this);
    
    // TackDetector is initialized with its constructor
    
    // Initialize battery monitor
    m_batteryMonitor.begin();
}

NavigationGUI::~NavigationGUI() {
    // Clean up component objects
    delete m_timeSeries;
    delete m_simulation;
    delete m_pointers;
}

void NavigationGUI::init() {
    // Initialize the GUI

    // Initialize Waveshare LCD peripherals
    System_Init();

    // Initialize LCD
    LCD_SCAN_DIR lcd_scan_dir = SCAN_DIR_DFT;
    LCD_Init(lcd_scan_dir, 800);
    GUI_Clear(LCD_BACKGROUND);

    // Draw labels
    updateTarget();
    GUI_DisString_EN(0, 0, "No GPS...", &Font24, BLACK, WHITE);
    GUI_DisString_EN(10, 175, "SOG", &Font20, BLACK, WHITE);
    GUI_DisString_EN(260, 175, "COG", &Font20, BLACK, WHITE);
    GUI_DisString_EN(130, 175, "TACK", &Font20, BLACK, WHITE);
    
    // Initial battery display
    updateBatteryDisplay();

    // Initialize plot area and draw initial plot
    m_timeSeries->clearPlotArea();
    m_timeSeries->drawPlot();
}

// Set the update interval for the time series plot
void NavigationGUI::setTimeSeriesUpdateInterval(uint32_t seconds) {
    printf("Setting time series update interval to %u seconds\n", seconds);
    m_timeSeries->setUpdateInterval(seconds);
}

void NavigationGUI::update(GPSFix data) {
    // Store the data
    Data = data;
    
    // If simulation is active, add incremental simulated data
    if (m_simulation->isActive()) {
        m_simulation->addIncrementalSimulatedData();
        Data.timestamp = m_simulation->getTimestamp();
        Data.speed = m_timeSeries->getLastSOG();
        Data.course = 135.0; // Arbitrary course for simulation
    }

    // Update the current bearing to the target mark
    target_bearing = calculateBearing(
        Data.lat, Data.lon,
        current_target.lat, current_target.lon
    );
    
    // Update tack detector with current heading, speed, timestamp, and position
    if (Data.status) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        m_tackDetector.updatePosition(Data.lat, Data.lon);
        m_tackDetector.update(Data.course, Data.speed, current_time);
    }

    // Calculate VMG and store the sign as a character
    float vmg = calculateVMG(Data.speed, Data.course, target_bearing);
    char vmg_sign[2] = { (vmg < 0 ? '-' : ' '), '\0' };
    float vmg_abs = fabs(vmg);

    // Format speed floats as strings
    char speedStr[8];
    char vmgStr[8];
    char courseStr[8];

    // Format the strings with one decimal place
    snprintf(speedStr, sizeof(speedStr), "%.1f", Data.speed);
    snprintf(vmgStr, sizeof(vmgStr), "%.1f", vmg_abs);    
    snprintf(courseStr, sizeof(courseStr), "%03d", static_cast<int>(round(Data.course)));

    // Show VMG in top
    GUI_DisString_EN(70, 60, vmgStr, &Font96, LCD_BACKGROUND, WHITE);

    // Show the sign left of the VMG, to keep centered
    GUI_DisString_EN(10, 60, vmg_sign, &Font96, LCD_BACKGROUND, WHITE);

    // Show speed over ground
    GUI_DisString_EN(10, 200, speedStr, &Font36, LCD_BACKGROUND, WHITE);

    // Show course over ground
    GUI_DisString_EN(240, 200, courseStr, &Font36, LCD_BACKGROUND, WHITE);
    
    // Show last tack heading if available
    float last_tack = m_tackDetector.getLastTackHeading();
    char tackHeadingStr[8] = "-"; // Default to "N/A"
    if (last_tack > 0.0) {
        snprintf(tackHeadingStr, sizeof(tackHeadingStr), "%03d", static_cast<int>(round(last_tack)));
    }
    GUI_DisString_EN(140, 200, tackHeadingStr, &Font36, BLACK, WHITE);

    // Print timestamp
    char time_str[10];
    
    // If Data.timestamp is 0, report "No GPS fix" to display
    if (Data.timestamp < 1) {
        snprintf(time_str, sizeof(time_str), "No GPS...");
    } else {
        time_from_epoch(Data.timestamp, time_str, sizeof(time_str));
    }
    GUI_DisString_EN(0, 0, time_str, &Font24, BLACK, WHITE);
    
    // Update battery display
    updateBatteryDisplay();
    
    // Always add data points when they arrive (to maintain data accuracy)
    uint32_t lastPlotUpdate = m_timeSeries->getLastUpdateTime();
    if (Data.timestamp != lastPlotUpdate && Data.timestamp > 0) {
        // Only add a data point if not in simulation mode (simulation already added it)
        if (!m_simulation->isActive()) {
            m_timeSeries->addDataPoint(vmg, Data.speed, Data.timestamp);
        }
        
        // Only update the visual display when it's time to update based on the configured interval
        bool shouldUpdateNow = m_timeSeries->shouldUpdate(Data.timestamp);
        
        if (shouldUpdateNow) {
            // Only clear the data area, not the axes and labels
            m_timeSeries->clearPlotArea();
            
            // Draw the plot
            m_timeSeries->drawPlot();
            
            // Update the last visual update timestamp
            m_timeSeries->updateLastVisualTimestamp(Data.timestamp);
        }
    }
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

// Update the target display
void NavigationGUI::updateTarget() {
    // Format the target mark text
    char markStr[20];
    snprintf(markStr, sizeof(markStr), "VMG (kt) -> %s", current_target.name);
    int mark_str_len = 18 * strlen(markStr);
    
    // Clear the text area
    LCD_SetArealColor(0, 40, 480, 70, LCD_BACKGROUND);
    
    // Draw the new text
    GUI_DisString_EN(320 - mark_str_len, 40, markStr, &Font24, BLACK, WHITE);
}

// Update the battery percentage display in the top right corner
void NavigationGUI::updateBatteryDisplay() {
    // Get battery percentage and current
    float battery_percentage = m_batteryMonitor.getBatteryPercentage();
    float current = m_batteryMonitor.getCurrent_mA();
    
    // Get display dimensions from sLCD_DIS
    extern LCD_DIS sLCD_DIS;  // Declare the external LCD_DIS structure
    
    // Clear the battery display area
    LCD_SetArealColor(215, 0, 320, 24, LCD_BACKGROUND);
    
    // Check if battery is charging (indicated by special value -1)
    if (battery_percentage < 0) {
        // Display "charging" instead of percentage
        GUI_DisString_EN(220, 0, "charging", &Font16, BLACK, WHITE);
    } else {
        // Format as 2-digit whole integer with leading zero
        char batteryStr[8];
        snprintf(batteryStr, sizeof(batteryStr), "%02.0f%%", battery_percentage);
        
        // Draw the battery percentage
        GUI_DisString_EN(265, 0, batteryStr, &Font24, BLACK, WHITE);
    }
    
    // // Debug output
    // if (battery_percentage < 0) {
    //     printf("Battery: Charging, Current: %.2f mA\n", current);
    // } else {
    //     printf("Battery: %.0f%%, Current: %.2f mA\n", battery_percentage, current);
    // }
}

// Cycle to the next target mark
void NavigationGUI::cycleToNextTarget() {
    // Use a more reliable approach with a timeout to prevent button lockup
    static uint32_t last_cycle_time = 0;
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    // Allow cycling if at least 100ms has passed since the last cycle
    // This is more reliable than a boolean flag that could get stuck
    if (current_time - last_cycle_time < 100) {
        printf("Cycling too fast, ignoring request\n");
        return;
    }
    
    // Update the last cycle time
    last_cycle_time = current_time;
    printf("Cycling to next target mark\n");
    
    // Find the current index in the marks array
    int current_index = 0;
    for (size_t i = 0; i < Navigation::MARKS.size(); i++) {
        if (strcmp(current_target.name, Navigation::MARKS[i].name) == 0) {
            current_index = i;
            break;
        }
    }
    
    // Increment index and wrap around if needed
    int next_index = (current_index + 1) % Navigation::MARKS.size();
    
    // Update the current target
    current_target = Navigation::MARKS[next_index];
    printf("New target: %s\n", current_target.name);
    
    // Recalculate the bearing to the new target if we have valid GPS data
    if (Data.status) {
        target_bearing = calculateBearing(
            Data.lat, Data.lon,
            current_target.lat, current_target.lon
        );
        printf("New bearing: %.1f degrees\n", target_bearing);
    }
    
    // Update the target display
    updateTarget();
}