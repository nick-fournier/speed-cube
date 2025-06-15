#ifndef NAVIGATION_H
#define NAVIGATION_H
#include "L76B.h"
#include "gps_data.h"
#include "gps_datetime.h"
#include "math.h"
#include "tack_detector.h"
#include "marks.h"

extern "C" {
    #include "DEV_Config.h"
    #include "LCD_Driver.h"
    #include "LCD_Touch.h"
    #include "LCD_GUI.h"
    #include "LCD_Bmp.h"
}

// Forward declarations
class Simulation;
class TimeSeriesPlot;
class Pointers;

// Define constants for navigation calculations
static constexpr double DEG2RAD = M_PI / 180.0;
static constexpr double RAD2DEG = 180.0 / M_PI;

class NavigationGUI {
    public:
        // Constructor and destructor
        NavigationGUI();
        ~NavigationGUI();

        // Use the Mark struct from marks.h
        using Target = Navigation::Mark;

        // Initialization function
        void init();
        void update(GPSFix data);
        
        // Navigation calculations
        float calculateBearing(float lat1, float lon1, float lat2, float lon2);
        float calculateVMG(float speed, float course, float target_bearing);
        
        // Accessors
        float getTargetBearing() const { return target_bearing; }
        const Target& getCurrentTarget() const { return current_target; }
        float getLastTackHeading() const { return m_tackDetector.getLastTackHeading(); }
        
        // Target selection
        void cycleToNextTarget();
        
        // Configure time series plot
        void setTimeSeriesUpdateInterval(uint32_t seconds);
    
    private:
        // Friend declarations
        friend class Simulation;
        friend class TimeSeriesPlot;
        friend class Pointers;
        
        // Component objects
        Simulation* m_simulation;
        TimeSeriesPlot* m_timeSeries;
        Pointers* m_pointers;
        TackDetector m_tackDetector;  // Tack detection and tracking
        
        GPSFix Data;

        // Display parameters
        int centerX = 160;  // Center X coordinate of the display
        int centerY = 150;  // Center Y coordinate of the display
        int radius = 130;   // Radius of the circle

        // Internal variables
        float target_bearing = 0.0; // Target bearing in degrees
        float tack_bearing = 0.0;   // Opposing tack bearing in degrees
        
        // Current target - using the marks from marks.h
        Target current_target = Navigation::MARKS[0];
};

#endif
