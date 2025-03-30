#ifndef L76B_H
#define L76B_H

#include <string>

class L76B {
public:
    // Constructor
    L76B();

    // Function to initialize the module
    void init();

    // Function to read data from UART and process NMEA sentences
    // timeout_ms: maximum time to spend reading in milliseconds (0 = no timeout)
    void readNMEA(unsigned long timeout_ms = 100);

    // Function to process NMEA sentence
    bool processNMEA(const std::string& nmea);

    // Getters
    float getTime() const;
    float getLatitude() const;
    float getLongitude() const;
    float getSpeed() const;
    float getHeading() const;
    bool getStatus() const;


private:
    float time;
    float latitude;
    float longitude;
    float speed;   // Speed in knots
    float heading; // Heading in degrees
    bool status; // False means no data received from module

    // Helper function to parse $GNRMC NMEA sentence
    void parseGNRMC(const std::string& nmea);

    // Helper function to convert NMEA coordinates to decimal degrees
    float convertToDecimalDegrees(const std::string& coordinate, const std::string& direction);
};

#endif // L76B_H
