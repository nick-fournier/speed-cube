#include <stdio.h>
#include "pico/stdlib.h"
#include "LCD.h"
#include "L76B.h"


L76B gps;

// Setup function
void setup() {
    // Initialize standard library
    stdio_init_all();
    
    // Initialize the LCD
    lcd_init();
    
    // Set display rotation (0 degrees - normal orientation)
    lcd_set_rotation(LCD_ROTATION_180);
    
    // Clear the screen with black background
    lcd_clear(COLOR_BLACK);
    
    // Display "Hello, World!" text with larger font (scale factor 3)
    // Center the text on the display
    lcd_draw_string_scaled(20, 100, "Hello, World!", COLOR_WHITE, COLOR_BLACK, 3);

    // Initialize the GPS module
    gps.init();

}


int main()
{
    // Initialize standard library
    stdio_init_all();
    
    // Initialize the LCD
    lcd_init();
    
    // Set display rotation (0 degrees - normal orientation)
    lcd_set_rotation(LCD_ROTATION_180);
    
    // Clear the screen with black background
    lcd_clear(COLOR_BLACK);
    
    // Display "Hello, World!" text with larger font (scale factor 3)
    // Center the text on the display
    lcd_draw_string_scaled(20, 100, "Hello, World!", COLOR_WHITE, COLOR_BLACK, 3);
    
    // Main loop
    while (true) {
        // Keep the program running
        sleep_ms(1000);
        printf("Hello, World!\n");

        // Write a counter to the display
        static int counter = 0;
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "Counter: %d", counter);
        lcd_draw_string_scaled(20, 200, buffer, COLOR_WHITE, COLOR_BLACK, 2);
        counter++;
    }
    
    return 0;
}