#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "drivers/picocalc.h"
#include "drivers/keyboard.h"
#include "drivers/onboard_led.h"

#include "commands.h"

int main()
{
    char buffer[40];
    uint8_t index = 0;

    // Initialize the LED driver and set the LED callback
    // If the LED driver fails to initialize, we can still run the text starter
    // without LED support, so we pass NULL to picocalc_init.
    int led_init_result = led_init();

    stdio_init_all();
    picocalc_init(led_init_result == 0 ? led_set : NULL);

    printf(" Hello from the PicoCalc Text Starter!\n\n");
    printf("      Contributed to the community\n");
    printf("            by Blair Leduc.\n\n");
    printf("Type 'help' for a list of commands.\n\n");

    // A very simple REPL
    printf("\033[qReady.\n");
    while (true)
    {
        char ch = getchar();
        if (ch == 0x04) // Ctrl+D to debug
        {
            printf("Entering debug mode...\n");
            __breakpoint();
        }
        else if ((ch == 0x08 || ch == 0x7F) && index > 0) // Backspace or Delete
        {
            printf("\b \b"); // Erase the last character
            index--;
        }
        else if (ch >= 0x20 && ch < 0x7F && index < sizeof(buffer) - 1) // Printable characters
        {
            buffer[index++] = ch;
            putchar(ch);
        }
        else if (ch == 0x0D) // Enter key
        {
            printf("\033[1q\n"); // Turn on the LED so the user knows input is being processed
            buffer[index] = '\0'; // Null-terminate the string

            run_command(buffer); // Call the command handler

            printf("\033[q\nReady.\n"); // Turn off the LED and prompt for input again
            index = 0;
        }
    }
}
