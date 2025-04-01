#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "LCD.h"
#include "L76B.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"


L76B gps;


int main()
{
    // Initialize standard library
    stdio_init_all();
    
    // Initialize the LCD
    lcd_init();
    
    // Set display rotation (0 degrees - normal orientation)
    lcd_set_rotation(LCD_ROTATION_180);
    
    // Set higher contrast for better black levels
    lcd_set_contrast(0x40);  // Lower contrast
    // lcd_set_contrast(0x80);  // Medium contrast
    // lcd_set_contrast(0xA0);  // Slightly higher contrast
    // lcd_set_contrast(0xD0);  // Higher contrast value for deeper blacks
    // lcd_set_contrast(0xFF);  // Maximum contrast

    // Clear the screen with true black background
    lcd_clear(COLOR_BLACK);
    
    // Display "Hello, World!" text with larger font (scale factor 3)
    // Center the text on the display
    lcd_draw_string_scaled(20, 100, "Hello, World!", COLOR_WHITE, COLOR_BLACK, 3);
    
    // Main loop
    while (true) {
        // Keep the program running
        printf("Hello, World!\n");

        // Write a counter to the display
        static int counter = 0;
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "Counter: %d", counter);
        lcd_draw_string_scaled(20, 200, buffer, COLOR_WHITE, COLOR_BLACK, 2);
        counter++;

        sleep_ms(1000);
    }
    
    return 0;
}