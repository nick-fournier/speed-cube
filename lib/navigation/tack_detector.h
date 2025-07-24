#ifndef TACK_DETECTOR_H
#define TACK_DETECTOR_H

#include "pico/stdlib.h"
#include "gps_data.h"
#include <cmath>
#include <vector>

class TackDetector {
public:
    // Constructor
    TackDetector();
    
    // Update method - call this with each new heading and position
    void update(float heading, float speed, uint32_t timestamp);
    void updatePosition(float lat, float lon);
    
    // Accessors
    float getLastTackHeading() const { return last_tack_heading; }
    bool isOnStarboardTack() const { return is_on_starboard_tack; }
    
    // Configuration methods
    void setWindDirection(float direction) { WIND_DIRECTION = direction; }
    void setMinimumSpeed(float speed) { MINIMUM_SPEED_FOR_TACK = speed; }
    void setDebounceTime(uint32_t time_ms) { TACK_DEBOUNCE_TIME = time_ms; }
    void setAngleThreshold(float angle) { TACK_ANGLE_THRESHOLD = angle; }
    void setMinimumDistance(float meters) { MINIMUM_DISTANCE_FOR_TACK = meters; }
    void setHeadingStabilityThreshold(float degrees) { HEADING_STABILITY_THRESHOLD = degrees; }
    
private:
    // Tack tracking variables
    float last_tack_heading;     // Heading from before the last tack
    float previous_heading;      // Previous heading for detecting changes
    bool is_on_starboard_tack;   // Track which tack the boat is on
    uint32_t last_tack_time;     // Time of the last tack for debouncing
    
    // Position and distance tracking
    float last_lat;              // Latitude at last heading change
    float last_lon;              // Longitude at last heading change
    float current_lat;           // Current latitude
    float current_lon;           // Current longitude
    float distance_traveled;     // Distance traveled since last heading change
    bool position_initialized;   // Flag to indicate if position has been initialized
    
    // Heading stability tracking
    static const int HEADING_BUFFER_SIZE = 10;  // Size of heading buffer for stability analysis
    std::vector<float> heading_buffer;          // Buffer of recent headings
    
    // Tack detection parameters
    float WIND_DIRECTION;                // Assumed upwind direction in degrees
    float MINIMUM_SPEED_FOR_TACK;        // Minimum speed in knots for tack detection
    uint32_t TACK_DEBOUNCE_TIME;         // Minimum time between tacks in ms
    float TACK_ANGLE_THRESHOLD;          // Minimum angle change to detect a tack
    float MINIMUM_DISTANCE_FOR_TACK;     // Minimum distance traveled for tack detection (meters)
    float HEADING_STABILITY_THRESHOLD;   // Maximum heading variation for stability (degrees)
    
    // Helper methods
    float normalizeAngle(float angle) const;
    float calculateDistance(float lat1, float lon1, float lat2, float lon2) const;
    bool isHeadingStable() const;
    void resetDistanceTracking();
};

#endif // TACK_DETECTOR_H