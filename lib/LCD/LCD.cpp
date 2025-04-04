#include "LCD.h"
#include "font5x7.h"
#include "font10x14.h"
#include "math.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"


// Constructor
LCD::LCD() : currentRotation(0) {
    // Initialize member variables
}

// Initialize the LCD
void LCD::init() {
    // Initialize SPI at a lower speed initially for stability
    spi_init(SPI_PORT == 0 ? spi0 : spi1, 1000000); // Start at 1 MHz for initialization
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
    writeCommand(ILI9488_SWRESET);    // Software reset
    sleep_ms(150);                    // Longer delay after reset
    
    writeCommand(ILI9488_SLPOUT);     // Sleep out
    sleep_ms(150);                    // Longer delay after sleep out
    
    // Power control settings
    writeCommand(0xC0);               // Power Control 1
    writeData(0x0E);
    writeData(0x0E);
    
    writeCommand(0xC1);               // Power Control 2
    writeData(0x41);
    writeData(0x00);
    
    writeCommand(0xC5);               // VCOM Control
    writeData(0x00);
    writeData(0x22);
    writeData(0x80);
    
    // Adjusted gamma settings to reduce purple tint
    writeCommand(0xE0);               // Positive Gamma Control
    writeData(0x00);
    writeData(0x09);                  // Increased from 0x07 to reduce purple
    writeData(0x0F);
    writeData(0x0D);
    writeData(0x1B);
    writeData(0x0A);
    writeData(0x3C);
    writeData(0x78);
    writeData(0x4A);
    writeData(0x07);
    writeData(0x0E);
    writeData(0x09);
    writeData(0x1B);
    writeData(0x1E);
    writeData(0x0F);
    
    writeCommand(0xE1);               // Negative Gamma Control
    writeData(0x00);
    writeData(0x22);
    writeData(0x24);
    writeData(0x06);
    writeData(0x12);
    writeData(0x07);
    writeData(0x36);
    writeData(0x47);
    writeData(0x47);
    writeData(0x06);
    writeData(0x0A);
    writeData(0x07);
    writeData(0x30);
    writeData(0x37);
    writeData(0x0F);
    
    // Set default rotation (0 degrees)
    setRotation(LCD_ROTATION_0);
    
    // Set default contrast
    setContrast(0xC0);  // Higher contrast for better blacks
    
    writeCommand(ILI9488_COLMOD);     // Interface Pixel Format
    writeData(0x55);                  // 16 bits per pixel
    
    // Additional settings for stability
    writeCommand(0xB0);               // Interface Mode Control
    writeData(0x00);
    
    writeCommand(0xB1);               // Frame Rate Control
    writeData(0xA0);                  // 60Hz
    
    writeCommand(0xB4);               // Display Inversion Control
    writeData(0x02);                  // 2-dot inversion
    
    writeCommand(0xB6);               // Display Function Control
    writeData(0x02);
    writeData(0x02);
    writeData(0x3B);
    
    writeCommand(ILI9488_DISPON);     // Display On
    sleep_ms(150);                    // Longer delay after display on
    
    // Now increase SPI speed to operational speed
    spi_set_baudrate(SPI_PORT == 0 ? spi0 : spi1, SPI_SPEED);
    
    // Turn on backlight only after display is fully initialized
    sleep_ms(50);
    gpio_put(PIN_BL, 1);
}

// Set display contrast and gamma
void LCD::setContrast(uint8_t contrast) {
    // Set VCOM control for contrast
    writeCommand(0xC5);               // VCOM Control
    writeData(0x00);                  // First parameter
    writeData(contrast);              // Contrast value (higher = more contrast)
    writeData(0x80);                  // Third parameter
    
    // Adjust gamma for better black levels and reduced purple tint
    writeCommand(0xF2);               // Enable Gamma
    writeData(0x01);                  // Enable gamma adjustment
    
    // Set display inversion for better color reproduction
    writeCommand(0xB4);               // Display Inversion Control
    writeData(0x01);                  // 1-dot inversion for better blacks
    
    // Additional settings to reduce purple tint and shadow
    writeCommand(0xC7);               // VCOM Offset Control
    writeData(0xC0);                  // Adjusted to reduce purple tint
}

// Set the display rotation
void LCD::setRotation(uint8_t rotation) {
    currentRotation = rotation % 4;
    
    writeCommand(ILI9488_MADCTL);     // Memory Access Control
    
    switch (currentRotation) {
        case LCD_ROTATION_0:
            // Normal orientation (0 degrees)
            writeData(MADCTL_MX | MADCTL_BGR);  // Add MX to fix mirroring
            break;
        case LCD_ROTATION_90:
            // 90 degrees clockwise
            writeData(MADCTL_MV | MADCTL_MY | MADCTL_BGR);
            break;
        case LCD_ROTATION_180:
            // 180 degrees
            writeData(MADCTL_MY | MADCTL_BGR);
            break;
        case LCD_ROTATION_270:
            // 270 degrees clockwise
            writeData(MADCTL_MV | MADCTL_MX | MADCTL_BGR);
            break;
    }
}

// Clear the screen with specified color
void LCD::clear(uint16_t color) {
    setWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    
    gpio_put(PIN_DC, 1);  // Data mode
    gpio_put(PIN_CS, 0);  // Select display
    
    // Fill the screen with the specified color
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        writeData16Bit(color);
    }
    
    gpio_put(PIN_CS, 1);  // Deselect display
}


// Draw a string at the specified position with scaling
void LCD::drawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t scale) {
    if (scale < 1) scale = 1;
    
    uint16_t posX = x;
    
    while (*str) {
        drawChar(posX, y, *str, color, bg_color, scale);
        posX += (5 * scale) + scale; // 5 pixels for character + 1 pixel spacing, all scaled
        str++;
    }
}

// Internal methods

// Send a command to the display
void LCD::writeCommand(uint8_t cmd) {
    gpio_put(PIN_DC, 0);  // Command mode
    gpio_put(PIN_CS, 0);  // Select display
    
    spi_write_blocking(SPI_PORT == 0 ? spi0 : spi1, &cmd, 1);
    
    gpio_put(PIN_CS, 1);  // Deselect display
}

// Send data to the display
void LCD::writeData(uint8_t data) {
    gpio_put(PIN_DC, 1);  // Data mode
    gpio_put(PIN_CS, 0);  // Select display
    
    spi_write_blocking(SPI_PORT == 0 ? spi0 : spi1, &data, 1);
    
    gpio_put(PIN_CS, 1);  // Deselect display
}

// Send 16-bit data to the display
void LCD::writeData16Bit(uint16_t data) {
    uint8_t data_bytes[2];
    data_bytes[0] = (data >> 8) & 0xFF;  // High byte
    data_bytes[1] = data & 0xFF;         // Low byte
    
    gpio_put(PIN_DC, 1);  // Data mode
    gpio_put(PIN_CS, 0);  // Select display
    
    spi_write_blocking(SPI_PORT == 0 ? spi0 : spi1, data_bytes, 2);
    
    gpio_put(PIN_CS, 1);  // Deselect display
}

// Set the drawing window
void LCD::setWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    // Set column address
    writeCommand(ILI9488_CASET);
    writeData(x1 >> 8);
    writeData(x1 & 0xFF);
    writeData(x2 >> 8);
    writeData(x2 & 0xFF);
    
    // Set row address
    writeCommand(ILI9488_PASET);
    writeData(y1 >> 8);
    writeData(y1 & 0xFF);
    writeData(y2 >> 8);
    writeData(y2 & 0xFF);
    
    // Write to RAM
    writeCommand(ILI9488_RAMWR);
}

// Draw a single pixel
void LCD::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    setWindow(x, y, x, y);
    writeData16Bit(color);
}

void LCD::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    // Bresenham's line algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        drawPixel(x1, y1, color);  // Draw pixel

        if (x1 == x2 && y1 == y2) break;  // End of line

        int err2 = err * 2;
        if (err2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (err2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void LCD::drawChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color, uint8_t scale) {
    int width = 5;
    int height = 7;
    const unsigned char *font = font5x7;
    
    if (scale < 1) scale = 1;

    // if (scale  == 2) {
    //     font = font10x14;
    //     width = 10;
    //     height = 14;
    //     scale = 1;
    // }

    // Valid range check: allow up to your custom symbol
    if (ch < ' ' || ch > 127) ch = '?';

    // Get character index in font array
    uint16_t index = (ch - ' ') * width;

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            bool pixel = (font[index + col] >> row) & 0x01;
            uint16_t pixelColor = pixel ? color : bg_color;

            for (int ys = 0; ys < scale; ys++) {
                for (int xs = 0; xs < scale; xs++) {
                    drawPixel(x + col * scale + xs, y + row * scale + ys, pixelColor);
                }
            }
        }
    }
}
