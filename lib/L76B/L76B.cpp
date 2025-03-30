#include "L76B.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"

#define BUFFER_SIZE 83
#define UART_ID uart0
#define BAUD_RATE_DEFAULT 9600
#define BAUD_RATE 115200
#define UART_TX_PIN 0  // Replace with the appropriate TX pin for your setup
#define UART_RX_PIN 1  // Replace with the appropriate RX pin for your setup
#define PMTK_CMD_COLD_START "$PMTK103*30\r\n"
#define PMTK_CMD_FULL_COLD_START "$PMTK104*37\r\n"
#define PMTK_SET_NMEA_UPDATERATE "$PMTK220,100\r\n"


L76B::L76B() : latitude(0.0), longitude(0.0), speed(0.0), heading(0.0) {
    
}

unsigned long systime() {
    return to_ms_since_boot(get_absolute_time());
}

void L76B::init() {
    // Initialize UART
    uart_init(UART_ID, BAUD_RATE_DEFAULT);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Set UART format
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    // Update baud rate
    // uart_set_baudrate(UART_ID, BAUD_RATE);

    // Full cold start
    // uart_puts(UART_ID, PMTK_CMD_COLD_START);

    // Set NMEA update rate
    // uart_puts(UART_ID, PMTK_SET_NMEA_UPDATERATE);
    printf("GPS initialized\n");

}

void L76B::readNMEA(unsigned long timeout_ms) {
    char buffer[BUFFER_SIZE];  // Buffer to store NMEA sentence
    size_t index = 0;
    
    // Track start time for timeout
    unsigned long startTime = systime();

    // Read characters from UART until a full NMEA sentence is received or timeout
    while (true) {
        // Check for timeout first
        if (timeout_ms != 0 && systime() - startTime > timeout_ms) {
            printf("Timeout reached\n");
            break;  // Exit if timeout is reached
        }
        
        // Check if data is available
        if (!uart_is_readable(UART_ID)) {
            sleep_ms(1000);  // Small delay to prevent CPU hogging when no data
            printf("No data available\n");

            // Reinitalize GPS
            // init();

            // continue;
            break;
        }
        
        // Read a character
        char c = uart_getc(UART_ID);
        
        if (c == '$') {
            // Start of new sentence
            index = 0;  // Reset index
            buffer[index++] = c;
        } else if (c == '\n' || c == '\r') {
            // End of sentence
            if (index < BUFFER_SIZE) {
                buffer[index] = '\0';  // Null-terminate the string
                // Check if it's the NMEA sentence we're looking for
                std::string nmea(buffer);
                if (nmea.find("$GNRMC") != std::string::npos) {
                    parseGNRMC(nmea);
                    break;
                }
            }
        } else if (index < BUFFER_SIZE - 1) {
            // Add character to buffer if there's space
            buffer[index++] = c;
        }
    }

    // printf("Buffer: %s\n", buffer);
}

void L76B::parseGNRMC(const std::string& nmea) {
    // Example of GNRMC Sentence: 
    // $GNRMC,092204.999,A,5321.6802,N,00630.3372,W,0.06,31.66,280511,,,A*43

    size_t start = 0, pos;
    int fieldIndex = 0;
    std::string delimiter = ",";

    // Check if the sentence is GNRMC
    if (nmea.find("$GNRMC") == std::string::npos) {
        return;  // Exit without updating variables
    }   

    while ((pos = nmea.find(delimiter, start)) != std::string::npos) {
        std::string token = nmea.substr(start, pos - start);  // Extract field
        
        switch (fieldIndex) {
            case 0: // Sentence identifier
                break;
            case 1: // Time
                time = std::stod(token);
                break;
            case 3: // Latitude
                latitude = convertToDecimalDegrees(token, nmea.substr(pos + 1, 1));
                break;
            case 5: // Longitude
                longitude = convertToDecimalDegrees(token, nmea.substr(pos + 1, 1));
                break;
            case 7: // Speed in knots
                speed = std::stod(token);
                break;
            case 8: // Heading in degrees
                heading = std::stod(token);
                break;
            default:
                break;
        }

        start = pos + 1;  // Move to the next field
        fieldIndex++;
    }

}

float L76B::convertToDecimalDegrees(const std::string& coordinate, const std::string& direction) {
    // Coordinate format: ddmm.mmmm
    // Direction format: N/S/E/W
    // Example: 12218.6633, W == -122.3110555

    // Extract degrees as integer
    int dd = (int)(std::stod(coordinate) / 100);

    // Extract minutes as float
    float mm = std::stod(coordinate) - (dd * 100);

    // Convert to decimal degrees
    float decimalDegrees = dd + (mm / 60);
    if (direction == "S" || direction == "W") {
        decimalDegrees *= -1;
    }
    return decimalDegrees;
}

float L76B::getTime() const { return time; }
float L76B::getLatitude() const { return latitude; }
float L76B::getLongitude() const { return longitude; }
float L76B::getSpeed() const { return speed; }
float L76B::getHeading() const { return heading; }
bool L76B::getStatus() const { return status; }

