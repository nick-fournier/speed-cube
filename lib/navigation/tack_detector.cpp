#include "tack_detector.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <algorithm>

TackDetector::TackDetector() {
    // Initialize tack tracking variables
    last_tack_heading = 0.0;
    previous_heading = 0.0;
    is_on_starboard_tack = false;
    last_tack_time = 0;
    
    // Initialize position and distance tracking
    last_lat = 0.0;
    last_lon = 0.0;
    current_lat = 0.0;
    current_lon = 0.0;
    distance_traveled = 0.0;
    position_initialized = false;
    
    // Initialize heading buffer
    heading_buffer.reserve(HEADING_BUFFER_SIZE);
    
    // Initialize tack detection parameters with default values
    WIND_DIRECTION = 270.0;              // Default upwind direction is 270 degrees
    MINIMUM_SPEED_FOR_TACK = 1.0;        // Default minimum speed is 1.0 knots
    TACK_DEBOUNCE_TIME = 10000;          // Default debounce time is 10 seconds
    TACK_ANGLE_THRESHOLD = 45.0;         // Default angle threshold is 45 degrees
    MINIMUM_DISTANCE_FOR_TACK = 50.0;    // Default minimum distance is 50 meters
    HEADING_STABILITY_THRESHOLD = 10.0;  // Default heading stability threshold is 10 degrees
}

void TackDetector::update(float heading, float speed, uint32_t timestamp) {
    // Only process if we have valid data and sufficient speed
    if (speed >= MINIMUM_SPEED_FOR_TACK && position_initialized) {
        // Normalize heading to 0-360 range
        float normalized_heading = normalizeAngle(heading);
        
        // Update heading buffer for stability analysis
        if (heading_buffer.size() >= HEADING_BUFFER_SIZE) {
            heading_buffer.erase(heading_buffer.begin());
        }
        heading_buffer.push_back(normalized_heading);
        
        // Calculate distance traveled since last heading change
        distance_traveled = calculateDistance(last_lat, last_lon, current_lat, current_lon);
        
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
                
                // Check if we've traveled enough distance with a stable heading
                if (heading_change >= TACK_ANGLE_THRESHOLD && 
                    distance_traveled >= MINIMUM_DISTANCE_FOR_TACK && 
                    isHeadingStable()) {
                    
                    // This is a valid tack - store the previous heading
                    last_tack_heading = previous_heading;
                    last_tack_time = timestamp;
                    is_on_starboard_tack = on_starboard_now;
                    
                    // Reset distance tracking
                    resetDistanceTracking();
                    
                    // Log the tack for debugging
                    printf("Tack detected! Changed to %s tack. Last heading: %.1f, Distance: %.1f m\n", 
                           is_on_starboard_tack ? "starboard" : "port", 
                           last_tack_heading, 
                           distance_traveled);
                }
            }
        }
        
        // Update previous heading for next iteration
        previous_heading = normalized_heading;
    }
}

void TackDetector::updatePosition(float lat, float lon) {
    // Update current position
    current_lat = lat;
    current_lon = lon;
    
    // Initialize position tracking if not already initialized
    if (!position_initialized) {
        last_lat = lat;
        last_lon = lon;
        position_initialized = true;
    }
}

float TackDetector::normalizeAngle(float angle) const {
    // Normalize angle to 0-360 range
    return fmod(angle + 360.0, 360.0);
}

float TackDetector::calculateDistance(float lat1, float lon1, float lat2, float lon2) const {
    // Convert latitude and longitude from degrees to radians
    float lat1_rad = lat1 * M_PI / 180.0;
    float lon1_rad = lon1 * M_PI / 180.0;
    float lat2_rad = lat2 * M_PI / 180.0;
    float lon2_rad = lon2 * M_PI / 180.0;
    
    // Haversine formula to calculate distance between two points on Earth
    float dlon = lon2_rad - lon1_rad;
    float dlat = lat2_rad - lat1_rad;
    float a = sin(dlat/2) * sin(dlat/2) + cos(lat1_rad) * cos(lat2_rad) * sin(dlon/2) * sin(dlon/2);
    float c = 2 * atan2(sqrt(a), sqrt(1-a));
    float distance = 6371000 * c; // Earth radius in meters
    
    return distance;
}

bool TackDetector::isHeadingStable() const {
    // Check if we have enough heading samples
    if (heading_buffer.size() < HEADING_BUFFER_SIZE) {
        return false;
    }
    
    // Calculate the maximum heading variation
    float min_heading = *std::min_element(heading_buffer.begin(), heading_buffer.end());
    float max_heading = *std::max_element(heading_buffer.begin(), heading_buffer.end());
    
    // Handle wrap-around case (e.g., headings near 0/360 boundary)
    float variation = max_heading - min_heading;
    if (variation > 180.0) {
        // Adjust for wrap-around
        for (size_t i = 0; i < heading_buffer.size(); i++) {
            if (heading_buffer[i] > 180.0) {
                min_heading = std::min<float>(min_heading, heading_buffer[i] - 360.0);
            } else {
                max_heading = std::max<float>(max_heading, heading_buffer[i] + 360.0);
            }
        }
        variation = max_heading - min_heading;
    }
    
    // Check if the heading variation is within the stability threshold
    return variation <= HEADING_STABILITY_THRESHOLD;
}

void TackDetector::resetDistanceTracking() {
    // Reset distance tracking after a tack
    last_lat = current_lat;
    last_lon = current_lon;
    distance_traveled = 0.0;
    
    // Clear heading buffer
    heading_buffer.clear();
}