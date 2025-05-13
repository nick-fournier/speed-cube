#include "gps_data.h"

// Global shared structs
GPSFix raw_data = {};
mutex_t raw_data_mutex;

GPSFix filtered_data = {};
mutex_t filtered_data_mutex;

GPSFix gps_buffer[GPS_BUFFER_SIZE] = {};
mutex_t gps_buffer_mutex;
