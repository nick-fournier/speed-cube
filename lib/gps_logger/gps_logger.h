#pragma once

#include "gps_data.h"
#include "ff.h"  // Correct path to ff.h

/**
 * @brief GPS Logger class for logging GPS data to CSV files
 */
class GPSLogger {
public:
    /**
     * @brief Initialize the GPS logger with SD card and file system
     * 
     * @param filename Name of the CSV file to create (required)
     * @return true if initialization was successful, false otherwise
     */
    bool init(const char* filename);

    /**
     * @brief Log GPS data to the CSV file
     * 
     * @param raw_data Raw GPS data
     * @param filtered_data Filtered GPS data
     * @return true if logging was successful, false otherwise
     */
    bool logData(const GPSFix& raw_data, const GPSFix& filtered_data);

    /**
     * @brief Close the GPS logger
     */
    void close();

    /**
     * @brief Check if the logger is initialized
     * 
     * @return true if the logger is initialized, false otherwise
     */
    bool isInitialized() const { return initialized; }

private:
    FIL file;              // File object
    bool initialized;      // Flag to indicate if the logger is initialized
    char csv_buffer[256];  // Buffer for CSV data
};