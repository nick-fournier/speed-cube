#ifndef LCD_H
#define LCD_H

#include <stdint.h>

// Display configuration for Waveshare Pico-ResTouch-LCD-3.5
// Display uses ILI9488 controller with SPI interface

// Display dimensions
#define LCD_WIDTH  320
#define LCD_HEIGHT 480

// Rotation values
#define LCD_ROTATION_0   0
#define LCD_ROTATION_90  1
#define LCD_ROTATION_180 2
#define LCD_ROTATION_270 3

// Colors (RGB565 format)
// True black (0,0,0)
#define COLOR_BLACK   0xFFFF
// Deep blue (0,0,31)
#define COLOR_BLUE    0x001F
// Pure red (31,0,0)
#define COLOR_RED     0xF800
// Pure green (0,63,0)
#define COLOR_GREEN   0x07E0
// Pure white (31,63,31)
#define COLOR_WHITE   0x0000
// Yellow (31,63,0)
#define COLOR_YELLOW  0xFFE0
// Gray (16,32,16)
#define COLOR_GRAY    0x8410
// Cyan (0,63,31)
#define COLOR_CYAN    0x07FF
// Magenta (31,0,31)
#define COLOR_MAGENTA 0xF81F

class LCD {
public:
    // Constructor
    LCD();
    
    // Initialize the LCD display
    void init();
    
    // Set the display rotation
    void setRotation(uint8_t rotation);
    
    // Set display contrast and gamma
    void setContrast(uint8_t contrast);
    
    // Clear the screen with specified color
    void clear(uint16_t color);
        
    // Draw a string at the specified position with scaling
    void drawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t scale);

    // Draw a character at the specified position with scaling
    void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

private:
    // SPI Configuration
    static const uint32_t SPI_PORT = 1;  // spi1
    // static const uint32_t SPI_SPEED = 10000000; // 10 MHz
    static const uint32_t SPI_SPEED = 50000000; // 50 MHz
    static const uint8_t PIN_SCK = 10;
    static const uint8_t PIN_MOSI = 11;
    static const uint8_t PIN_MISO = 12;
    static const uint8_t PIN_CS = 9;
    static const uint8_t PIN_DC = 8;
    static const uint8_t PIN_RST = 15;
    static const uint8_t PIN_BL = 13;
    
    // ILI9488 Commands
    static const uint8_t ILI9488_SWRESET = 0x01;
    static const uint8_t ILI9488_SLPOUT = 0x11;
    static const uint8_t ILI9488_DISPON = 0x29;
    static const uint8_t ILI9488_CASET = 0x2A;
    static const uint8_t ILI9488_PASET = 0x2B;
    static const uint8_t ILI9488_RAMWR = 0x2C;
    static const uint8_t ILI9488_MADCTL = 0x36;
    static const uint8_t ILI9488_COLMOD = 0x3A;
    
    // MADCTL values for different rotations
    static const uint8_t MADCTL_MX = 0x40;  // Row Address Order
    static const uint8_t MADCTL_MY = 0x80;  // Column Address Order
    static const uint8_t MADCTL_MV = 0x20;  // Row / Column Exchange
    static const uint8_t MADCTL_ML = 0x10;  // Vertical Refresh Order
    static const uint8_t MADCTL_RGB = 0x00; // RGB order
    static const uint8_t MADCTL_BGR = 0x08; // BGR order
    
    // Current rotation
    uint8_t currentRotation;
    
    // Internal methods
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void writeData16Bit(uint16_t data);
    void setWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void drawChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color, uint8_t scale);
};

#endif // LCD_H