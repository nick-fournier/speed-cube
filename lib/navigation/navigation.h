#ifndef NAVIGATION_H
#define NAVIGATION_H
#include "L76B.h"
#include "math.h"


// Define constants for navigation calculations
static constexpr double DEG2RAD = M_PI / 180.0;
static constexpr double RAD2DEG = 180.0 / M_PI;


class NavigationGUI {
    public:
        // Constructor
        NavigationGUI();

        struct Target {
            char name[10];          // Name of the target
            float lat;             // Latitude in decimal degrees
            float lon;             // Longitude in decimal degrees
        };

        // Initialization function
        void init();
        void update(GPSData data);

    
    private:
        GPSData Data;

        // Params
        int centerX = 160;  // Center X coordinate of the display
        int centerY = 150;  // Center Y coordinate of the display
        int radius = 120;   // Radius of the circle


        // Internal variables
        float prev_tack_deg = -1;  // -1 indicates "no arrow drawn"
        float prev_mark_deg = -1;  // -1 indicates "no arrow drawn"

        float target_bearing = 0.0; // Target bearing in degrees
        float tack_bearing = 0.0;   // Opposing tack bearing in degrees

        float calculateBearing(float lat1, float lon1, float lat2, float lon2);
        float calculateVMG(float speed, float heading, float target_bearing);

        void updateMarkPointer(float bearing_deg);
        void updateTackPointer(float bearing_deg);

        // Marks coordinates
        Target marks[5] = {
            {"Mark1", 37.77818748002346, -122.38120635348626},
            {"Mark2", 37.77818748002346, -122.38120635348626},
            {"Mark3", 37.77818748002346, -122.38120635348626},
            {"Mark4", 37.77818748002346, -122.38120635348626},
            {"Mark5", 37.77818748002346, -122.38120635348626}
        };
        
        // Current target
        Target current_target = marks[0];

    };
    

#endif