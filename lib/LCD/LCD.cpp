#include "LCD.h"
#include "font.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

// SPI Configuration
#define SPI_PORT spi1
#define SPI_SPEED 10000000 // 10 - 60 MHz
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
    // Initialize SPI at a lower speed initially for stability
    spi_init(SPI_PORT, 1000000); // Start at 1 MHz for initialization
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
    
    // Initial state
    gpio_put(PIN_CS, 1);  // Deselect display
    gpio_put(PIN_DC, 1);  // Data mode
    gpio_put(PIN_BL, 0);  // Backlight off initially
    
    // More robust reset sequence
    sleep_ms(50);         // Wait for power stabilization
    gpio_put(PIN_RST, 1);
    sleep_ms(10);
    gpio_put(PIN_RST, 0); // Reset low
    sleep_ms(20);         // Longer reset pulse
    gpio_put(PIN_RST, 1); // Reset high
    sleep_ms(150);        // Longer wait after reset
    
    // Initialize display with more robust sequence
    lcd_write_command(ILI9488_SWRESET);    // Software reset
    sleep_ms(150);                         // Longer delay after reset
    
    lcd_write_command(ILI9488_SLPOUT);     // Sleep out
    sleep_ms(150);                         // Longer delay after sleep out
    
    // Power control settings
    lcd_write_command(0xC0);               // Power Control 1
    lcd_write_data(0x0E);
    lcd_write_data(0x0E);
    
    lcd_write_command(0xC1);               // Power Control 2
    lcd_write_data(0x41);
    lcd_write_data(0x00);
    
    lcd_write_command(0xC5);               // VCOM Control
    lcd_write_data(0x00);
    lcd_write_data(0x22);
    lcd_write_data(0x80);
    
    // Enhanced color and contrast settings
    lcd_write_command(0xE0);               // Positive Gamma Control
    lcd_write_data(0x00);
    lcd_write_data(0x07);
    lcd_write_data(0x0F);
    lcd_write_data(0x0D);
    lcd_write_data(0x1B);
    lcd_write_data(0x0A);
    lcd_write_data(0x3C);
    lcd_write_data(0x78);
    lcd_write_data(0x4A);
    lcd_write_data(0x07);
    lcd_write_data(0x0E);
    lcd_write_data(0x09);
    lcd_write_data(0x1B);
    lcd_write_data(0x1E);
    lcd_write_data(0x0F);
    
    lcd_write_command(0xE1);               // Negative Gamma Control
    lcd_write_data(0x00);
    lcd_write_data(0x22);
    lcd_write_data(0x24);
    lcd_write_data(0x06);
    lcd_write_data(0x12);
    lcd_write_data(0x07);
    lcd_write_data(0x36);
    lcd_write_data(0x47);
    lcd_write_data(0x47);
    lcd_write_data(0x06);
    lcd_write_data(0x0A);
    lcd_write_data(0x07);
    lcd_write_data(0x30);
    lcd_write_data(0x37);
    lcd_write_data(0x0F);
    
    // Set default rotation (0 degrees)
    lcd_set_rotation(LCD_ROTATION_0);
    
    // Set default contrast
    lcd_set_contrast(0xC0);  // Higher contrast for better blacks
    
    lcd_write_command(ILI9488_COLMOD);     // Interface Pixel Format
    lcd_write_data(0x55);                  // 16 bits per pixel
    
    // Additional settings for stability
    lcd_write_command(0xB0);               // Interface Mode Control
    lcd_write_data(0x00);
    
    lcd_write_command(0xB1);               // Frame Rate Control
    lcd_write_data(0xA0);                  // 60Hz
    
    lcd_write_command(0xB4);               // Display Inversion Control
    lcd_write_data(0x02);                  // 2-dot inversion
    
    lcd_write_command(0xB6);               // Display Function Control
    lcd_write_data(0x02);
    lcd_write_data(0x02);
    lcd_write_data(0x3B);
    
    lcd_write_command(ILI9488_DISPON);     // Display On
    sleep_ms(150);                         // Longer delay after display on
    
    // Now increase SPI speed to operational speed
    spi_set_baudrate(SPI_PORT, SPI_SPEED);
    
    // Turn on backlight only after display is fully initialized
    sleep_ms(50);
    gpio_put(PIN_BL, 1);
}

// Set display contrast and gamma
void lcd_set_contrast(uint8_t contrast) {
    // Set VCOM control for contrast
    lcd_write_command(0xC5);               // VCOM Control
    lcd_write_data(0x00);                  // First parameter
    lcd_write_data(contrast);              // Contrast value (higher = more contrast)
    lcd_write_data(0x80);                  // Third parameter
    
    // Adjust gamma for better black levels
    lcd_write_command(0xF2);               // Enable Gamma
    lcd_write_data(0x01);                  // Enable gamma adjustment
    
    // Set display inversion for better color reproduction
    lcd_write_command(0xB4);               // Display Inversion Control
    lcd_write_data(0x01);                  // 1-dot inversion for better blacks
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