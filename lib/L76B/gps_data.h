#pragma once
#include "pico/sync.h"

// Unified GPS data structure
struct GPSFix {
    char date[7];    // Date in ddmmyy format
    double lat;      // Latitude in decimal degrees
    double lon;      // Longitude in decimal degrees
    float speed;     // Speed in knots
    float course;    // Course in degrees
    float time;      // UTC time (hhmmss.sss)
    bool status;     // Valid fix (only meaningful for raw data)
};

// Shared raw fix data from NMEA parser (in L76B)
extern GPSFix raw_data;
extern mutex_t raw_data_mutex;

// Shared filtered fix data from Kalman filter
extern GPSFix filtered_data;
extern mutex_t filtered_data_mutex;
