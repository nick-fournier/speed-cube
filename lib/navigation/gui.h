#ifndef NAVIGATION_H
#define NAVIGATION_H
#include "L76B.h"
#include "gps_data.h"
#include "gps_datetime.h"
#include "math.h"

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

        struct Target {
            char name[10];          // Name of the target
            float lat;             // Latitude in decimal degrees
            float lon;             // Longitude in decimal degrees
        };

        // Initialization function
        void init();
        void update(GPSFix data);
        
        // Navigation calculations
        float calculateBearing(float lat1, float lon1, float lat2, float lon2);
        float calculateVMG(float speed, float course, float target_bearing);
        
        // Accessors
        float getTargetBearing() const { return target_bearing; }
        const Target& getCurrentTarget() const { return current_target; }
        
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
        
        GPSFix Data;

        // Display parameters
        int centerX = 160;  // Center X coordinate of the display
        int centerY = 150;  // Center Y coordinate of the display
        int radius = 130;   // Radius of the circle

        // Internal variables
        float target_bearing = 0.0; // Target bearing in degrees
        float tack_bearing = 0.0;   // Opposing tack bearing in degrees

        // Marks coordinates
        Target marks[8] = {
            {"SBYC", 37.77797371, -122.3852661},
            {"SC1",  37.77555,    -122.3658167},
            {"NAS1", 37.77725,    -122.3412833},
            {"NAS2", 37.774183,   -122.3410167},
            {"YB",   37.79951667, -122.3605167},
            {"AS1",  37.771383,   -122.3830167},
            {"34",   37.75835,    -122.3685333},
            {"33",   37.801,      -122.3477333}
        };
        
        // Current target
        Target current_target = marks[0];
};

#endif
