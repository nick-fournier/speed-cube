#include <stdio.h>
#include <string.h>
#include <time.h>
#include "gps_logger.h"
#include "gps_datetime.h"  // Correct path to gps_datetime.h

// Include SD card functions with C linkage
extern "C" {
    #include "MMC_SD.h"  // For SD card initialization
}

// Forward declare the SPI function we need
extern "C" unsigned char SPI4W_Write_Byte(unsigned char value);

// Function to send dummy bytes to stabilize SD card
void send_dummy_bytes(int count) {
    for (int i = 0; i < count; i++) {
        SPI4W_Write_Byte(0xFF);
    }
}

bool GPSLogger::init(const char* filename) {
    FRESULT res;
    static FATFS fs;
    
    // Reset initialization flag
    initialized = false;
    
    // Initialize SD card with a simplified approach
    printf("Initializing SD card...\n");
    
    // Add a brief delay to ensure hardware is ready
    printf("Waiting for SD card hardware to stabilize...\n");
    sleep_ms(1000); // 1 second initial delay
    
    // Send a few dummy bytes to stabilize the interface
    send_dummy_bytes(10); // Send 10 dummy bytes
    
    // Configure SPI for SD card
    SD_SPI_SpeedLow();
    
    // Try to initialize the SD card with limited retries
    uint8_t sd_init_result = 0;
    const int max_retries = 3; // Reduced retry count as per user request
    bool sd_initialized = false;
    
    for (int retry = 0; retry < max_retries; retry++) {
        printf("SD card initialization attempt %d of %d...\n", retry + 1, max_retries);
        
        // Try initialization
        sd_init_result = SD_Initialize();
        if (sd_init_result == 0) {
            printf("SD card initialized successfully\n");
            sd_initialized = true;
            break;
        }
        
        // Report specific error based on error code
        if (sd_init_result == 0x01) {
            printf("SD card in idle state (error code: %d), retrying...\n", sd_init_result);
            sleep_ms(1000); // Brief delay before retry
        } else {
            printf("Error: Failed to initialize SD card (error code: %d), retrying...\n", sd_init_result);
        }
    }
    
    // If all retries failed, provide a hardware-focused error message
    if (!sd_initialized) {
        printf("Error: Failed to initialize SD card (error code: %d).\n", sd_init_result);
        
        // Provide specific error messages based on error code
        switch (sd_init_result) {
            case 0x01: // MSD_IN_IDLE_STATE
                printf("SD card is stuck in idle state. This may indicate a hardware issue with the SD card.\n");
                printf("Possible solutions:\n");
                printf("1. Check if the SD card is properly inserted\n");
                printf("2. Try a different SD card\n");
                printf("3. Verify that the SD card is formatted correctly (FAT32)\n");
                printf("4. Check for physical damage to the SD card or SD card slot\n");
                break;
                
            case 0x0D: // MSD_DATA_WRITE_ERROR
                printf("SD card write error. This may indicate the card is write-protected or damaged.\n");
                printf("Possible solutions:\n");
                printf("1. Check if the SD card is write-protected (slide the lock switch)\n");
                printf("2. Try a different SD card\n");
                printf("3. Format the SD card using a computer\n");
                printf("4. Check for physical damage to the SD card\n");
                break;
                
            case 0x04: // MSD_ILLEGAL_COMMAND
                printf("SD card rejected the command as illegal. This may indicate an incompatible card type.\n");
                printf("Try using a different type or brand of SD card.\n");
                break;
                
            case 0x08: // MSD_COM_CRC_ERROR
                printf("SD card communication CRC error. This may indicate interference or connection issues.\n");
                printf("Check the connections and try again.\n");
                break;
                
            default:
                printf("This may indicate a hardware issue with the SD card or SD card interface.\n");
                printf("Possible solutions:\n");
                printf("1. Try a different SD card\n");
                printf("2. Check the SD card connections\n");
                printf("3. Verify that the SD card is compatible with this device\n");
                break;
        }
        
        return false;
    }
    
    // Switch to high speed for normal operation
    SD_SPI_SpeedHigh();
    
    // Mount file system
    printf("Mounting file system...\n");
    res = f_mount(&fs, "", 1);  // Mount default drive
    if (res != FR_OK) {
        printf("Error: Failed to mount file system (error code: %d)\n", res);
        return false;
    }
    printf("File system mounted successfully\n");
    
    // Validate that a filename was provided
    if (filename == NULL || filename[0] == '\0') {
        printf("Error: Filename is required for GPS logger initialization\n");
        return false;
    }
    
    // Ensure filename follows FatFS path format
    char fatfs_path[256];
    
    // Extract the base filename (remove any path separators)
    const char* base_filename = filename;
    const char* last_slash = strrchr(filename, '/');
    if (last_slash != NULL) {
        base_filename = last_slash + 1;
    }
    
    // Ensure the filename has the correct "0:" prefix and sanitize it
    snprintf(fatfs_path, sizeof(fatfs_path), "0:%s", base_filename);
    
    printf("Creating log file: %s\n", fatfs_path);
    
    // Try to open the file for writing (create if it doesn't exist)
    res = f_open(&file, fatfs_path, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        printf("Error: Failed to open log file %s (error code: %d)\n", fatfs_path, res);
        
        // Provide more detailed error information
        switch (res) {
            case FR_DISK_ERR:
                printf("A hard error occurred in the low level disk I/O layer\n");
                break;
            case FR_INT_ERR:
                printf("Assertion failed\n");
                break;
            case FR_NOT_READY:
                printf("The physical drive cannot work\n");
                break;
            case FR_NO_FILE:
                printf("Could not find the file\n");
                break;
            case FR_NO_PATH:
                printf("Could not find the path\n");
                break;
            case FR_INVALID_NAME:
                printf("The path name format is invalid\n");
                break;
            case FR_DENIED:
                printf("Access denied due to prohibited access or directory full\n");
                break;
            case FR_EXIST:
                printf("Access denied due to prohibited access\n");
                break;
            case FR_INVALID_OBJECT:
                printf("The file/directory object is invalid\n");
                break;
            case FR_WRITE_PROTECTED:
                printf("The physical drive is write protected\n");
                break;
            case FR_INVALID_DRIVE:
                printf("The logical drive number is invalid\n");
                break;
            case FR_NOT_ENABLED:
                printf("The volume has no work area\n");
                break;
            case FR_NO_FILESYSTEM:
                printf("There is no valid FAT volume\n");
                break;
            case FR_MKFS_ABORTED:
                printf("The f_mkfs() aborted due to any problem\n");
                break;
            case FR_TIMEOUT:
                printf("Could not get a grant to access the volume within defined period\n");
                break;
            case FR_LOCKED:
                printf("The operation is rejected according to the file sharing policy\n");
                break;
            case FR_NOT_ENOUGH_CORE:
                printf("LFN working buffer could not be allocated\n");
                break;
            case FR_TOO_MANY_OPEN_FILES:
                printf("Number of open files > FF_FS_LOCK\n");
                break;
            case FR_INVALID_PARAMETER:
                printf("Given parameter is invalid\n");
                break;
            default:
                printf("Unknown error code\n");
                break;
        }
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
    printf("GPS logger initialized with file: %s\n", fatfs_path);
    
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