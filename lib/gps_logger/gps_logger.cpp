#include <stdio.h>
#include <string.h>
#include "gps_logger.h"
#include "../L76B/gps_datetime.h"  // Correct path to gps_datetime.h

bool GPSLogger::init(const char* filename) {
    FRESULT res;
    
    // Reset initialization flag
    initialized = false;
    
    // Try to open the file for writing (create if it doesn't exist)
    res = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        printf("Error: Failed to open log file %s (error code: %d)\n", filename, res);
        return false;
    }
    
    // Write CSV header
    const char* header = "timestamp,date_time,"
                         "raw_lat,raw_lon,raw_speed,raw_course,"
                         "filtered_lat,filtered_lon,filtered_speed,filtered_course\n";
    
    UINT bytesWritten;
    res = f_write(&file, header, strlen(header), &bytesWritten);
    if (res != FR_OK || bytesWritten != strlen(header)) {
        printf("Error: Failed to write header to log file (error code: %d)\n", res);
        f_close(&file);
        return false;
    }
    
    // Flush the file to ensure header is written
    f_sync(&file);
    
    // Set initialization flag
    initialized = true;
    printf("GPS logger initialized with file: %s\n", filename);
    
    return true;
}

bool GPSLogger::logData(const GPSFix& raw_data, const GPSFix& filtered_data) {
    if (!initialized) {
        printf("Error: GPS logger not initialized\n");
        return false;
    }
    
    // Format date and time from timestamp
    char date_str[20];
    char time_str[20];
    char datetime_str[40];
    
    // Convert epoch timestamp to date and time strings
    date_from_epoch(raw_data.timestamp, date_str, sizeof(date_str));
    time_from_epoch(raw_data.timestamp, time_str, sizeof(time_str));
    
    // Combine date and time strings
    snprintf(datetime_str, sizeof(datetime_str), "%s %s", date_str, time_str);
    
    // Format CSV line with both raw and filtered data
    snprintf(csv_buffer, sizeof(csv_buffer), 
             "%u,%s,"
             "%.6f,%.6f,%.2f,%.2f,"
             "%.6f,%.6f,%.2f,%.2f\n",
             raw_data.timestamp, datetime_str,
             raw_data.lat, raw_data.lon, raw_data.speed, raw_data.course,
             filtered_data.lat, filtered_data.lon, filtered_data.speed, filtered_data.course);
    
    // Write CSV line to file
    UINT bytesWritten;
    FRESULT res = f_write(&file, csv_buffer, strlen(csv_buffer), &bytesWritten);
    if (res != FR_OK || bytesWritten != strlen(csv_buffer)) {
        printf("Error: Failed to write data to log file (error code: %d)\n", res);
        return false;
    }
    
    // Flush the file every write to ensure data is saved even if power is lost
    f_sync(&file);
    
    return true;
}

void GPSLogger::close() {
    if (initialized) {
        f_close(&file);
        initialized = false;
        printf("GPS logger closed\n");
    }
}