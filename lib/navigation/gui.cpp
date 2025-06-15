#include "gui.h"
#include "simulation.h"
#include "timeseries.h"
#include "pointers.h"

NavigationGUI::NavigationGUI() {
    // Create component objects
    m_timeSeries = new TimeSeriesPlot(this);
    m_simulation = new Simulation(this, m_timeSeries);
    m_pointers = new Pointers(this);
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

    // Initialize SD card
    SD_Init();

    // Initialize LCD
    LCD_SCAN_DIR lcd_scan_dir = SCAN_DIR_DFT;
    LCD_Init(lcd_scan_dir, 800);
    GUI_Clear(LCD_BACKGROUND);

    // Draw labels
    GUI_DisString_EN(100, 40, "VMG (kt)", &Font20, BLACK, WHITE);
    GUI_DisString_EN(10, 175, "SOG", &Font20, BLACK, WHITE);
    GUI_DisString_EN(260, 175, "COG", &Font20, BLACK, WHITE);

    // Initialize plot area and draw initial plot
    m_timeSeries->clearPlotArea();
    m_timeSeries->drawPlot();
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

    // Calculate VMG and store the sign as a character
    float vmg = calculateVMG(Data.speed, Data.course, target_bearing);
    char vmg_sign[2] = { (vmg < 0 ? '-' : ' '), '\0' };
    float vmg_abs = fabs(vmg);

    // Format speed floats as strings
    char speedStr[8];
    char vmgStr[8];
    char courseStr[8];
    char markStr[20];

    // Store the current target mark name as string with prefix
    snprintf(markStr, sizeof(markStr), "->%s", current_target.name);

    // Get length of string
    int mark_str_len = 18 * strlen(markStr);

    // Format the strings with one decimal place
    snprintf(speedStr, sizeof(speedStr), "%.1f", Data.speed);
    snprintf(vmgStr, sizeof(vmgStr), "%.1f", vmg_abs);    
    snprintf(courseStr, sizeof(courseStr), "%03d", static_cast<int>(round(Data.course)));

    // Show the current target mark
    GUI_DisString_EN(320 - mark_str_len, 0, markStr, &Font24, BLACK, WHITE);

    // Show VMG in top
    GUI_DisString_EN(70, 60, vmgStr, &Font96, LCD_BACKGROUND, WHITE);

    // Show the sign left of the VMG, to keep centered
    GUI_DisString_EN(10, 60, vmg_sign, &Font96, LCD_BACKGROUND, WHITE);

    // Show speed over ground
    GUI_DisString_EN(10, 200, speedStr, &Font48, LCD_BACKGROUND, WHITE);

    // Show course over ground
    GUI_DisString_EN(220, 200, courseStr, &Font48, LCD_BACKGROUND, WHITE);

    // Print timestamp
    char time_str[10];
    
    // If Data.timestamp is 0, report "No GPS fix" to display
    if (Data.timestamp == 0) {
        snprintf(time_str, sizeof(time_str), "No GPS...");
    } else {
        time_from_epoch(Data.timestamp, time_str, sizeof(time_str));
    }
    GUI_DisString_EN(0, 0, time_str, &Font24, BLACK, WHITE);
    
    // Update plot data and redraw plot (once per second)
    uint32_t lastPlotUpdate = m_timeSeries->getLastUpdateTime();
    if (Data.timestamp != lastPlotUpdate && Data.timestamp > 0) {
        // Only add a data point if not in simulation mode (simulation already added it)
        if (!m_simulation->isActive()) {
            m_timeSeries->addDataPoint(vmg, Data.speed, Data.timestamp);
        }
        
        // Only clear the data area, not the axes and labels
        m_timeSeries->clearPlotArea();
        
        // Draw the plot
        m_timeSeries->drawPlot();
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