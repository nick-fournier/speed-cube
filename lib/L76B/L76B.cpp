#include "L76B.h"
#include <cstring>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/irq.h"
#include "hardware/uart.h"

#define UART_ID uart0
#define UART_TX_PIN 0  // Replace with the appropriate TX pin for your setup
#define UART_RX_PIN 1  // Replace with the appropriate RX pin for your setup
#define BAUD_RATE_DEFAULT 9600
#define BAUD_RATE 115200

#define PMTK_CMD_COLD_START "$PMTK103*30\r\n"
#define PMTK_CMD_FULL_COLD_START "$PMTK104*37\r\n"

#define PMTK_CMD_BAUDRATE "$PMTK251,115200*1F\r\n"  // Set baud rate to 115200
#define PMTK_SET_NMEA_UPDATERATE "$PMTK220,200\r\n" // Set update rate to 5Hz (200ms)


// Constructor
L76B::L76B() {}

unsigned long systime() {
    return to_ms_since_boot(get_absolute_time());
}

void L76B::init() {
    // Initialize UART
    uart_init(UART_ID, BAUD_RATE_DEFAULT);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    // Full cold start
    // uart_puts(UART_ID, PMTK_CMD_COLD_START);

    // Set NMEA update rate
    uart_puts(UART_ID, PMTK_SET_NMEA_UPDATERATE);

    // Set baud rate to 115200
    uart_puts(UART_ID, PMTK_CMD_BAUDRATE);
    sleep_ms(100);  // Wait for the command to take effect

    // Reinitialize UART with the new baud rate
    uart_deinit(UART_ID);
    uart_init(UART_ID, BAUD_RATE);

    // Enable UART RX interrupt
    irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);


    printf("GPS initialized\n");
}

void L76B::on_uart_rx() {

    // Track start time for timeout
    unsigned long startTime = systime();

    // Read characters from UART until a full NMEA sentence is received or timeout
    while (uart_is_readable(UART_ID)) {
        // Read a character
        char c = uart_getc(UART_ID);
       
        // Start of new sentence
        if (c == '$') {
            buffer_index = 0;  // Reset index
        }

        // Add to buffer if there's space
        if (buffer_index < sizeof(rx_buffer) - 1) {
            rx_buffer[buffer_index++] = c;
        }

        // Check for end of sentence
        if (c == '\n' || c == '\r') {
            rx_buffer[buffer_index] = '\0';  // Null-terminate the string

            if (strncmp(rx_buffer, "$GNRMC", 6) == 0) {
                parse(rx_buffer);
            }

            buffer_index = 0;  // Reset for the next sentence
        }

    }

    // printf("Buffer: %s\n", buffer);
}

bool L76B::parse(const char* buffer) {
    // Example of GNRMC Sentence: 
    // $GNRMC,092204.999,A,5321.6802,N,00630.3372,W,0.06,31.66,280511,,,A*43
    // <0> $GNRMC
    // <1> UTC time, the format is hhmmss.sss
    // <2> Positioning status, A=effective positioning, V=invalid positioning
    // <3> Latitude, the format is ddmm.mmmmmmm
    // <4> Latitude hemisphere, N or S (north latitude or south latitude)
    // <5> Longitude, the format is dddmm.mmmmmmm
    // <6> Longitude hemisphere, E or W (east longitude or west longitude)
    // <7> Ground speed
    // <8> Ground heading (take true north as the reference datum)
    // <9> UTC date, the format is ddmmyy (day, month, year)
    // <10> Magnetic declination (000.0~180.0 degrees)
    // <11> Magnetic declination direction, E (east) or W (west)
    // <12> Mode indication (A=autonomous positioning, D=differential, E=estimation, N=invalid data)
    // * Statement end marker
    // XX XOR check value of all bytes starting from $ to *
    // <CR> Carriage return, end tag
    // <LF> line feed, end tag

    char lat_hem, lon_hem;

    // Parse NMEA RMC sentence
    int matched = sscanf(
        buffer,
        "$GNRMC,%f,%c,%f,%c,%f,%c,%f,%f,%6s",
        &Data.time, 
        &status,
        &Data.latitude, &lat_hem,
        &Data.longitude, &lon_hem,
        &Data.speed,
        &Data.heading,
        Data.date
    );

    // Ensure all expected values were extracted
    if (matched < 9 || status != 'A') {  // 'A' means valid fix
        return false;  // Invalid data
    }

    // Convert latitude and longitude to decimal degrees
    Data.latitude = toDecimalDegrees(Data.latitude, lat_hem);
    Data.longitude = toDecimalDegrees(Data.longitude, lon_hem);

    // Set status to true indicating valid data received
    status = true;
    
    return true;

}

GPSData L76B::getData() const {
    return Data;
}

float L76B::toDecimalDegrees(
    const float& coordinate,
    const char& hemisphere
) {
    // Coordinate format: ddmm.mmmm
    // Hemisphere format: N/S/E/W
    // Example: 12218.6633, W == -122.3110555

    // Extract degrees and minutes
    int dd = (int)(coordinate / 100);
    float mm = coordinate - (dd * 100);

    // Convert to decimal degrees
    float decimalDegrees = dd + (mm / 60);

    // Adjust sign based on hemisphere
    return (hemisphere == 'S' || hemisphere == 'W') ? -decimalDegrees : decimalDegrees;

}

float L76B::Time() const { return Data.time; }
float L76B::Latitude() const { return Data.latitude; }
float L76B::Longitude() const { return Data.longitude; }
float L76B::Speed() const { return Data.speed; }
float L76B::Heading() const { return Data.heading; }
bool L76B::Status() const { return status; }

