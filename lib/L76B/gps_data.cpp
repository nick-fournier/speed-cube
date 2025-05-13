#include "gps_data.h"

// Global shared structs
GPSFix raw_data = {};
mutex_t raw_data_mutex;

GPSFix filtered_data = {};
mutex_t filtered_data_mutex;

GPSFix gps_buffer[GPS_BUFFER] = {};
int gps_buffer_head = 0;
int gps_buffer_count = 0;
mutex_t gps_buffer_mutex;
