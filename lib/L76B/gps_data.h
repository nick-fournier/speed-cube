#pragma once
#include "pico/sync.h"

#define GPS_BUFFER 100

// Unified GPS data structure
struct GPSFix {
    char date[7];    // Date in ddmmyy format
    double lat;      // Latitude in decimal degrees
    double lon;      // Longitude in decimal degrees
    float speed;     // Speed in knots
    float course;    // Course in degrees
    float time;      // UTC time (hhmmss.sss)
    bool status;     // Status flag (true if valid fix)
};

// Array of GPS data
extern GPSFix gps_buffer[GPS_BUFFER];
extern int gps_buffer_head;
extern int gps_buffer_count;
extern mutex_t gps_buffer_mutex;

// Shared raw fix data from NMEA parser (in L76B)
extern GPSFix raw_data;
extern mutex_t raw_data_mutex;

// Shared filtered fix data from Kalman filter
extern GPSFix filtered_data;
extern mutex_t filtered_data_mutex;
