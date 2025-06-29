#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/float.h"
#include "pico/util/datetime.h"
#include "drivers/picocalc.h"
#include "pico/time.h"

#include "drivers/keyboard.h"
#include "pico/bootrom.h"

volatile bool user_interrupt = false;

int main()
{
    char buffer[40];
    int index = 0;

    stdio_init_all();
    picocalc_init();

    printf(" Hello from the PicoCalc Text Starter!\n");
    printf("      Contributed to the community\n");
    printf("            by Blair Leduc.\n\n");
    printf("Type 'help' for a list of commands.\n");

    printf("Ready.\n");
    while (true) {
        char ch = getchar();
        if (ch == 0x04) { // Ctrl+D to exit
            printf("Exiting...\n");
            break;
        } else if (ch == 0x0D) { // Enter key
            buffer[index] = '\0'; // Null-terminate the string
            printf("\n");
            if (strcmp(buffer, "exit") == 0) {
                printf("Exiting...\n");
                rom_reset_usb_boot(0, 0);
                break;
            } else if (strcmp(buffer, "clear") == 0) {
                printf("\033[2J\033[H"); // ANSI escape code to clear the screen
                index = 0; // Reset index after clearing
                continue;
            } else if (strcmp(buffer, "battery") == 0) {
                int battery_level = southbridge_read_battery();
                bool charging = (battery_level & 0x80) != 0; // Check if charging
                if (charging) {
                    battery_level &= 0x7F; // Mask out the charging bit
                    printf("Battery level: %d%% (charging)\n", battery_level);
                } else {
                    printf("Battery level: %d%%\n", battery_level);
                }
            } else if (strcmp(buffer, "speedtest") == 0) {
                int row = 1;
                printf("Running display speed test...\n\033[?25l");
                
                absolute_time_t start_time = get_absolute_time();

                while(!user_interrupt && row <= 5000) {
                    int color = 16 + (row % 215);
                    printf("\033[38;5;%dmRow: %04d 01234567890ABCDEFGHIJKLMNOPQRS", color, row++);
                }

                absolute_time_t end_time = get_absolute_time();
                uint64_t elapsed_us = absolute_time_diff_us(start_time, end_time);
                float elapsed_seconds = elapsed_us / 1000000.0;
                float rows_per_second = (row - 1) / elapsed_seconds;
                
                printf("\033[?25h\nSpeed test complete.\n");
                printf("Rows processed: %d\n", row - 1);
                printf("Time elapsed: %.2f seconds\n", elapsed_seconds);
                printf("Average rows per second: %.2f\n", rows_per_second);
            } else if (strcmp(buffer, "help") == 0) {
                printf("Available commands:\n");
                printf("  clear - Clear the screen\n");
                printf("  battery - Show the battery level\n");
                printf("  exit - Reboot into BOOTSEL mode\n");
                printf("  speedtest - Run a speed test\n");
                printf("  help - Show this help message\n");
                index = 0; // Reset index after showing help
                continue;
            } else {
                printf("%s ?\nType 'help' for a list of commands.\n", buffer);
            }
            printf("Ready.\n");
            index = 0;
            user_interrupt = false;
        } else if ((ch == 0x08 || ch == 0x7F) && index > 0) { // Backspace or Delete
            printf("\b \b"); // Erase the last character
            index--;
            continue;
        } else if (ch >= 0x20 && ch < 0x7F && index < sizeof(buffer) - 1) { // Printable characters
            buffer[index++] = ch;
            putchar(ch);
            continue;
        }
        putchar(ch); // Echo the character back
    }
}
