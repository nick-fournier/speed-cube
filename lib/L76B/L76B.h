#ifndef L76B_H
#define L76B_H

#include <string>
#include "kalman.h"
#include "gps_data.h"

class L76B {
public:

    // Constructor
    L76B();

    // Function to initialize the module
    void init();

    // Getters
    GPSFix getData() const;  // Get latest GPS data
    float Time() const;
    float Latitude() const;
    float Longitude() const;
    float Speed() const;
    float Course() const;
    bool Status() const;

private:
    // Add kalman filter
    KalmanFilter kf;  // Uncomment if Kalman filter is used

    // ISR stub
    static void on_uart_rx();

    // Instance-based UART handler
    // Function to read data from UART and process NMEA sentences
    void handle_uart();

    // Helper function to parse $GNRMC NMEA sentence
    bool parse(const char* nmea);

    // Helper function to convert NMEA coordinates to decimal degrees
    float toDecimalDegrees(const float& coordinate, const char& hemisphere);

    // Buffer for NMEA sentence
    static inline char rx_buffer[83];
    static inline size_t buffer_index = 0;

    // GPSFix Data;  // Raw GPS data
    static inline GPSFix working_data;

    // Pointer to this instance
    static inline L76B* instance = nullptr;
};

#endif // L76B_H
