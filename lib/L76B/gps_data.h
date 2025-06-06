#pragma once
#include "pico/sync.h"

#define GPS_BUFFER_SIZE 100

// Unified GPS data structure
struct GPSFix {
    uint32_t timestamp; // UTC time in seconds since epoch
    float lat;      // Latitude in decimal degrees
    float lon;      // Longitude in decimal degrees
    float speed;     // Speed in knots
    float course;    // Course in degrees
    bool status;     // Status flag (true if valid fix)
};

// Dual buffer of raw and filtered data
struct GPSBuffer {
    uint32_t timestamp; // Timestamp in seconds since epoch
    struct {
        float lat;      // Latitude in decimal degrees
        float lon;      // Longitude in decimal degrees
        float speed;     // Speed in knots
        float course;    // Course in degrees
    } filtered; // Filtered data from Kalman filter
    struct {
        float lat;      // Latitude in decimal degrees
        float lon;      // Longitude in decimal degrees
        float speed;     // Speed in knots
        float course;    // Course in degrees
    } raw;      // Raw data from NMEA parser
};

// Array of GPS data
extern GPSBuffer gps_buffer[GPS_BUFFER_SIZE];
extern size_t gps_buffer_index;
extern size_t gps_buffer_count;
extern mutex_t gps_buffer_mutex;

// Shared raw fix data from NMEA parser (in L76B)
extern GPSFix raw_data;
extern mutex_t raw_data_mutex;

// Shared filtered fix data from Kalman filter
extern GPSFix filtered_data;
extern mutex_t filtered_data_mutex;
