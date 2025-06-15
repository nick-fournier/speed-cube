#ifndef TACK_DETECTOR_H
#define TACK_DETECTOR_H

#include "pico/stdlib.h"
#include "gps_data.h"
#include <cmath>

class TackDetector {
public:
    // Constructor
    TackDetector();
    
    // Update method - call this with each new heading
    void update(float heading, float speed, uint32_t timestamp);
    
    // Accessors
    float getLastTackHeading() const { return last_tack_heading; }
    bool isOnStarboardTack() const { return is_on_starboard_tack; }
    
    // Configuration methods
    void setWindDirection(float direction) { WIND_DIRECTION = direction; }
    void setMinimumSpeed(float speed) { MINIMUM_SPEED_FOR_TACK = speed; }
    void setDebounceTime(uint32_t time_ms) { TACK_DEBOUNCE_TIME = time_ms; }
    void setAngleThreshold(float angle) { TACK_ANGLE_THRESHOLD = angle; }
    
private:
    // Tack tracking variables
    float last_tack_heading;     // Heading from before the last tack
    float previous_heading;      // Previous heading for detecting changes
    bool is_on_starboard_tack;   // Track which tack the boat is on
    uint32_t last_tack_time;     // Time of the last tack for debouncing
    
    // Tack detection parameters
    float WIND_DIRECTION;            // Assumed upwind direction in degrees
    float MINIMUM_SPEED_FOR_TACK;    // Minimum speed in knots for tack detection
    uint32_t TACK_DEBOUNCE_TIME;     // Minimum time between tacks in ms
    float TACK_ANGLE_THRESHOLD;      // Minimum angle change to detect a tack
    
    // Helper method to normalize angle to 0-360 range
    float normalizeAngle(float angle) const;
};

#endif // TACK_DETECTOR_H