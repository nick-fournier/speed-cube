#ifndef L76B_H
#define L76B_H

#include <string>

// Data structure to hold GPS data
struct GPSData {
    float time;             // Time in UTC (hhmmss.sss)
    char date[7];          // Date in UTC (ddmmyy), string of length 6
    float latitude;         // Latitude in decimal degrees
    float longitude;  // Longitude in decimal degrees
    float speed;      // Speed in knots
    float heading;    // Heading in degrees
};


class L76B {
public:

    // Constructor
    L76B();
    // Function to initialize the module
    void init();
    GPSData getData() const;  // Get latest GPS data

    // Getters
    float Time() const;
    float Latitude() const;
    float Longitude() const;
    float Speed() const;
    float Heading() const;
    bool Status() const;

private:
    // Function to read data from UART and process NMEA sentences
    static void on_uart_rx();

    // Helper function to parse $GNRMC NMEA sentence
    static bool parse(const char* nmea);

    // Helper function to convert NMEA coordinates to decimal degrees
    static float toDecimalDegrees(const float& coordinate, const char& hemisphere);

    // Buffer for NMEA sentence
    static inline char rx_buffer[83];
    static inline size_t buffer_index = 0;
    static inline GPSData Data;
    static inline bool status;
};

#endif // L76B_H
