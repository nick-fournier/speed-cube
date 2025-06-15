#include "tack_detector.h"
#include "pico/stdlib.h"
#include <stdio.h>

TackDetector::TackDetector() {
    // Initialize tack tracking variables
    last_tack_heading = 0.0;
    previous_heading = 0.0;
    is_on_starboard_tack = false;
    last_tack_time = 0;
    
    // Initialize tack detection parameters with default values
    WIND_DIRECTION = 270.0;              // Default upwind direction is 0 degrees
    MINIMUM_SPEED_FOR_TACK = 1.0;      // Default minimum speed is 1.0 knots
    TACK_DEBOUNCE_TIME = 10000;        // Default debounce time is 10 seconds
    TACK_ANGLE_THRESHOLD = 45.0;       // Default angle threshold is 45 degrees
}

void TackDetector::update(float heading, float speed, uint32_t timestamp) {
    // Only process if we have valid data and sufficient speed
    if (speed >= MINIMUM_SPEED_FOR_TACK) {
        // Normalize heading to 0-360 range
        float normalized_heading = normalizeAngle(heading);
        
        // Determine which tack the boat is currently on
        // Starboard tack: Wind is coming from the right side (heading between WIND_DIRECTION and WIND_DIRECTION+180)
        // Port tack: Wind is coming from the left side (heading outside that range)
        bool on_starboard_now = false;
        
        // Calculate the range for starboard tack
        float start_angle = WIND_DIRECTION;
        float end_angle = normalizeAngle(WIND_DIRECTION + 180.0);
        
        // Check if the current heading is within the starboard tack range
        if (start_angle < end_angle) {
            // Normal case (e.g., 0 to 180)
            on_starboard_now = (normalized_heading >= start_angle && normalized_heading <= end_angle);
        } else {
            // Wrapped case (e.g., 330 to 150)
            on_starboard_now = (normalized_heading >= start_angle || normalized_heading <= end_angle);
        }
        
        // Check if we've changed tacks
        if (on_starboard_now != is_on_starboard_tack) {
            // We've changed tacks, but make sure it's not just noise
            // Check if enough time has passed since the last tack (debouncing)
            if (timestamp - last_tack_time > TACK_DEBOUNCE_TIME) {
                // Check if the heading change is significant enough
                float heading_change = fabs(normalized_heading - previous_heading);
                if (heading_change > 180.0) {
                    heading_change = 360.0 - heading_change; // Handle wrap-around
                }
                
                if (heading_change >= TACK_ANGLE_THRESHOLD) {
                    // This is a valid tack - store the previous heading
                    last_tack_heading = previous_heading;
                    last_tack_time = timestamp;
                    is_on_starboard_tack = on_starboard_now;
                    
                    // Log the tack for debugging
                    printf("Tack detected! Changed to %s tack. Last heading: %.1f\n", 
                           is_on_starboard_tack ? "starboard" : "port", last_tack_heading);
                }
            }
        }
        
        // Update previous heading for next iteration
        previous_heading = normalized_heading;
    }
}

float TackDetector::normalizeAngle(float angle) const {
    // Normalize angle to 0-360 range
    return fmod(angle + 360.0, 360.0);
}