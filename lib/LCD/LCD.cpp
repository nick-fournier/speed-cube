#include "LCD.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

// SPI Configuration
#define SPI_PORT spi1
#define SPI_SPEED 50000000 // 10 - 60 MHz
#define PIN_SCK  10
#define PIN_MOSI 11
#define PIN_MISO 12
#define PIN_CS   9
#define PIN_DC   8
#define PIN_RST  15
#define PIN_BL   13

// ILI9488 Commands
#define ILI9488_SWRESET    0x01
#define ILI9488_SLPOUT     0x11
#define ILI9488_DISPON     0x29
#define ILI9488_CASET      0x2A
#define ILI9488_PASET      0x2B
#define ILI9488_RAMWR      0x2C
#define ILI9488_MADCTL     0x36
#define ILI9488_COLMOD     0x3A

// MADCTL values for different rotations
#define MADCTL_MX  0x40  // Row Address Order
#define MADCTL_MY  0x80  // Column Address Order
#define MADCTL_MV  0x20  // Row / Column Exchange
#define MADCTL_ML  0x10  // Vertical Refresh Order
#define MADCTL_RGB 0x00  // RGB order
#define MADCTL_BGR 0x08  // BGR order

// Current rotation
static uint8_t current_rotation = 0;

// Simple 5x7 font
const unsigned char font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // Space
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x08, 0x2A, 0x1C, 0x2A, 0x08, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x00, 0x08, 0x14, 0x22, 0x41, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x41, 0x22, 0x14, 0x08, 0x00, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x01, 0x01, // F
    0x3E, 0x41, 0x41, 0x49, 0x7A, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x04, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x7F, 0x20, 0x18, 0x20, 0x7F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x03, 0x04, 0x78, 0x04, 0x03, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x00, 0x7F, 0x41, 0x41, // [
    0x02, 0x04, 0x08, 0x10, 0x20, // "\"
    0x41, 0x41, 0x7F, 0x00, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x01, 0x02, 0x04, 0x00, // `
    0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x08, 0x14, 0x54, 0x54, 0x3C, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x20, 0x40, 0x44, 0x3D, 0x00, // j
    0x00, 0x7F, 0x10, 0x28, 0x44, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x7C, 0x14, 0x14, 0x14, 0x08, // p
    0x08, 0x14, 0x14, 0x18, 0x7C, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x0C, 0x50, 0x50, 0x50, 0x3C, // y
    0x44, 0x64, 0x54, 0x4C, 0x44, // z
    0x00, 0x08, 0x36, 0x41, 0x00, // {
    0x00, 0x00, 0x7F, 0x00, 0x00, // |
    0x00, 0x41, 0x36, 0x08, 0x00, // }
    0x08, 0x08, 0x2A, 0x1C, 0x08, // ->
    0x08, 0x1C, 0x2A, 0x08, 0x08  // <-
};

// Function prototypes for internal use
static void lcd_write_command(uint8_t cmd);
static void lcd_write_data(uint8_t data);
static void lcd_write_data_16bit(uint16_t data);
static void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
static void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
static void lcd_draw_char(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color);
static void lcd_draw_char_scaled(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color, uint8_t scale);

// Initialize the LCD
void lcd_init() {
    // Initialize SPI
    spi_init(SPI_PORT, SPI_SPEED); // 10 MHz
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // Initialize control pins
    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_RST);
    gpio_init(PIN_BL);
    
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    gpio_set_dir(PIN_BL, GPIO_OUT);
    
    // Reset sequence
    gpio_put(PIN_RST, 1);
    sleep_ms(10);
    gpio_put(PIN_RST, 0);
    sleep_ms(10);
    gpio_put(PIN_RST, 1);
    sleep_ms(120);
    
    // Turn on backlight
    gpio_put(PIN_BL, 1);
    
    // Initialize display
    lcd_write_command(ILI9488_SWRESET);    // Software reset
    sleep_ms(120);
    
    lcd_write_command(ILI9488_SLPOUT);     // Sleep out
    sleep_ms(120);
    
    // Set default rotation (0 degrees)
    lcd_set_rotation(LCD_ROTATION_0);
    
    lcd_write_command(ILI9488_COLMOD);     // Interface Pixel Format
    lcd_write_data(0x55);                  // 16 bits per pixel
    
    lcd_write_command(ILI9488_DISPON);     // Display On
    sleep_ms(120);
}

// Set the display rotation
void lcd_set_rotation(uint8_t rotation) {
    current_rotation = rotation % 4;
    
    lcd_write_command(ILI9488_MADCTL);     // Memory Access Control
    
    switch (current_rotation) {
        case LCD_ROTATION_0:
            // Normal orientation (0 degrees)
            lcd_write_data(MADCTL_MX | MADCTL_BGR);  // Add MX to fix mirroring
            break;
        case LCD_ROTATION_90:
            // 90 degrees clockwise
            lcd_write_data(MADCTL_MV | MADCTL_MY | MADCTL_BGR);
            break;
        case LCD_ROTATION_180:
            // 180 degrees
            lcd_write_data(MADCTL_MY | MADCTL_BGR);
            break;
        case LCD_ROTATION_270:
            // 270 degrees clockwise
            lcd_write_data(MADCTL_MV | MADCTL_MX | MADCTL_BGR);
            break;
    }
}

// Clear the screen with specified color
void lcd_clear(uint16_t color) {
    lcd_set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    
    gpio_put(PIN_DC, 1);  // Data mode
    gpio_put(PIN_CS, 0);  // Select display
    
    // Fill the screen with the specified color
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        lcd_write_data_16bit(color);
    }
    
    gpio_put(PIN_CS, 1);  // Deselect display
}

// Draw a string at the specified position
void lcd_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color) {
    uint16_t posX = x;
    
    while (*str) {
        lcd_draw_char(posX, y, *str, color, bg_color);
        posX += 6; // 5 pixels for character + 1 pixel spacing
        str++;
    }
}

// Draw a string at the specified position with scaling
void lcd_draw_string_scaled(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t scale) {
    if (scale < 1) scale = 1;
    
    uint16_t posX = x;
    
    while (*str) {
        lcd_draw_char_scaled(posX, y, *str, color, bg_color, scale);
        posX += (5 * scale) + scale; // 5 pixels for character + 1 pixel spacing, all scaled
        str++;
    }
}

// Internal functions

// Send a command to the display
static void lcd_write_command(uint8_t cmd) {
    gpio_put(PIN_DC, 0);  // Command mode
    gpio_put(PIN_CS, 0);  // Select display
    
    spi_write_blocking(SPI_PORT, &cmd, 1);
    
    gpio_put(PIN_CS, 1);  // Deselect display
}

// Send data to the display
static void lcd_write_data(uint8_t data) {
    gpio_put(PIN_DC, 1);  // Data mode
    gpio_put(PIN_CS, 0);  // Select display
    
    spi_write_blocking(SPI_PORT, &data, 1);
    
    gpio_put(PIN_CS, 1);  // Deselect display
}

// Send 16-bit data to the display
static void lcd_write_data_16bit(uint16_t data) {
    uint8_t data_bytes[2];
    data_bytes[0] = (data >> 8) & 0xFF;  // High byte
    data_bytes[1] = data & 0xFF;         // Low byte
    
    gpio_put(PIN_DC, 1);  // Data mode
    gpio_put(PIN_CS, 0);  // Select display
    
    spi_write_blocking(SPI_PORT, data_bytes, 2);
    
    gpio_put(PIN_CS, 1);  // Deselect display
}

// Set the drawing window
static void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    // Set column address
    lcd_write_command(ILI9488_CASET);
    lcd_write_data(x1 >> 8);
    lcd_write_data(x1 & 0xFF);
    lcd_write_data(x2 >> 8);
    lcd_write_data(x2 & 0xFF);
    
    // Set row address
    lcd_write_command(ILI9488_PASET);
    lcd_write_data(y1 >> 8);
    lcd_write_data(y1 & 0xFF);
    lcd_write_data(y2 >> 8);
    lcd_write_data(y2 & 0xFF);
    
    // Write to RAM
    lcd_write_command(ILI9488_RAMWR);
}

// Draw a single pixel
static void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    lcd_set_window(x, y, x, y);
    lcd_write_data_16bit(color);
}

// Draw a character
static void lcd_draw_char(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color) {
    if (ch < ' ' || ch > '~') ch = '?';  // Handle only printable ASCII
    
    // Get the character index in the font array
    ch -= ' ';
    
    // Draw the character pixel by pixel
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 5; col++) {
            uint8_t pixel = font5x7[ch * 5 + col] & (1 << row);
            if (pixel) {
                lcd_draw_pixel(x + col, y + row, color);
            } else {
                lcd_draw_pixel(x + col, y + row, bg_color);
            }
        }
    }
}

// Draw a character with scaling
static void lcd_draw_char_scaled(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color, uint8_t scale) {
    if (ch < ' ' || ch > '~') ch = '?';  // Handle only printable ASCII
    if (scale < 1) scale = 1;
    
    // Get the character index in the font array
    ch -= ' ';
    
    // Draw the character pixel by pixel with scaling
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 5; col++) {
            uint8_t pixel = font5x7[ch * 5 + col] & (1 << row);
            uint16_t pixelColor = pixel ? color : bg_color;
            
            // Draw a scaled pixel (scale x scale square)
            for (int y_scale = 0; y_scale < scale; y_scale++) {
                for (int x_scale = 0; x_scale < scale; x_scale++) {
                    lcd_draw_pixel(x + (col * scale) + x_scale, y + (row * scale) + y_scale, pixelColor);
                }
            }
        }
    }
}