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
#define COLOR_BLACK   0xFFFF
#define COLOR_BLUE    0x001F
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_WHITE   0x0000
#define COLOR_YELLOW  0xFFE0

// Initialize the LCD display
void lcd_init();

// Set the display rotation
void lcd_set_rotation(uint8_t rotation);

// Clear the screen with specified color
void lcd_clear(uint16_t color);

// Draw a string at the specified position
void lcd_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color);

// Draw a string at the specified position with scaling
void lcd_draw_string_scaled(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, uint8_t scale);

#endif // LCD_H