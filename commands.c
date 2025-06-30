#include <stdio.h>
#include <string.h>
#include <math.h>

#include "pico/bootrom.h"
#include "pico/float.h"
#include "pico/util/datetime.h"
#include "pico/time.h"

#include "drivers/southbridge.h"
#include "commands.h"

volatile bool user_interrupt = false;

// Command table - map of command names to functions
static const command_t commands[] = {
    {"battery", battery, "Show the battery level"},
    {"box", box, "Draw a box on the screen"},
    {"bye", bye, "Reboot into BOOTSEL mode"},
    {"cls", clearscreen, "Clear the screen"},
    {"speedtest", speedtest, "Run a speed test"},
    {"help", help, "Show this help message"},
    {NULL, NULL} // Sentinel to mark end of array
};

void run_command(const char *command)
{
    bool command_found = false; // Flag to check if command was found

    // Search for command in the table
    for (int i = 0; commands[i].name != NULL; i++)
    {
        if (strcmp(command, commands[i].name) == 0)
        {
            commands[i].function(); // Call the command function
            command_found = true;   // Command found and executed
            break;                  // Exit the loop
        }
    }

    if (!command_found)
    {
        printf("%s ?\nType 'help' for a list of commands.\n", command);
    }
    user_interrupt = false;
}

void help()
{
    printf("Available commands:\n");
    for (int i = 0; commands[i].name != NULL; i++)
    {
        printf("  %s - %s\n", commands[i].name, commands[i].description);
    }
}

void battery()
{
    int raw_level = sb_read_battery();
    int battery_level = raw_level & 0x7F;    // Mask out the charging bit
    bool charging = (raw_level & 0x80) != 0; // Check if charging

    printf("\033[?25l\033(0");
    if (charging)
    {
        printf("\033[93m");
    }
    else
    {
        printf("\033[97m");
    }
    printf("lqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqk\n");
    printf("x ");
    if (battery_level < 10)
    {
        printf("\033[101m"); // Set background colour to red for critical battery
    }
    else if (battery_level < 30)
    {
        printf("\033[103m"); // Set background colour to yellow for low battery
    }
    else
    {
        printf("\033[102m"); // Set background colour to green for sufficient battery
    }
    for (int i = 0; i < battery_level / 3; i++)
    {
        printf(" "); // Print a coloured space for each 3% of battery
    }
    printf("\033[37;40m"); // Reset colour
    for (int i = battery_level / 3; i < 33; i++)
    {
        printf("a"); // Fill the rest of the bar with fuzz
    }
    if (charging)
    {
        printf("\033[93m");
    }
    else
    {
        printf("\033[97m");
    }
    printf(" x\n");
    printf("mqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqj\n");

    // Switch back to ASCII, turn on cursor and reset character attributes
    printf("\033(B\033[?25h\033[m\n");

    if (charging)
    {
        printf("Battery level: %d%% (charging)\n", battery_level);
    }
    else
    {
        printf("Battery level: %d%%\n", battery_level);
    }
}

void box()
{
    printf("A box using the DEC Special Character\nSet:\n\n");
    printf("\033[93m");  // Set foreground colour to bright yellow
    printf("\033[?25l"); // Hide cursor

    // Switch to DEC Special Character Set and draw a box
    // Visual representation of what will be drawn:
    //   ┌──────┬──────┐
    //   │      │      │
    //   ├──────┼──────┤
    //   │      │      │
    //   └──────┴──────┘
    //
    // DEC Special Character mappings:
    // l = ┌ (top-left)     q = ─ (horizontal)   k = ┐ (top-right)
    // x = │ (vertical)     w = ┬ (top-tee)
    // t = ├ (left-tee)     n = ┼ (cross)        u = ┤ (right-tee)
    // m = └ (bottom-left)  v = ┴ (bottom-tee)   j = ┘ (bottom-right)

    printf("\033(0");          // Select DEC Special Character Set for G0
    printf("lqqqqqwqqqqqk\n"); // ┌──────┬──────┐
    printf("x     x     x\n"); // │      │      │
    printf("tqqqqqnqqqqqu\n"); // ├──────┼──────┤
    printf("x     x     x\n"); // │      │      │
    printf("mqqqqqvqqqqqj\n"); // └──────┴──────┘

    // Switch back to ASCII
    printf("\033(B\033[?25h"); // Select ASCII for G0 and show cursor
    printf("\033[0m");         // Reset colors
    printf("\n\nSee source code for the box drawing\ncharacters.\n");
}

void bye()
{
    printf("Exiting...\n");
    rom_reset_usb_boot(0, 0);
}

void clearscreen()
{
    printf("\033[2J\033[H"); // ANSI escape code to clear the screen
}

void speedtest()
{
    int row = 1;
    printf("\033[?25l"); // Hide cursor

    absolute_time_t start_time = get_absolute_time();

    while (!user_interrupt && row <= 5000)
    {
        int colour = 16 + (row % 215);
        printf("\033[38;5;%dmRow: %04d 01234567890ABCDEFGHIJKLMNOPQRS", colour, row++);
    }

    absolute_time_t end_time = get_absolute_time();
    uint64_t scrolling_elapsed_us = absolute_time_diff_us(start_time, end_time);
    float scrolling_elapsed_seconds = scrolling_elapsed_us / 1000000.0;
    float rows_per_second = (row - 1) / scrolling_elapsed_seconds;

    int chars = 0;
    printf("\033[m\033[2J\033[H"); // Reset colors, clear the screen, and move cursor to home position
    printf("Characters per second test:\n\n");
    printf("\033(0");  // Select DEC Special Character Set for G0
    printf("lqqqk\n"); // Top border: ┌───┐
    printf("x   x\n"); // Sides:      │   │
    printf("mqqqj\n"); // Bottom:     └───┘

    start_time = get_absolute_time(); // Reset the start time for the next display

    while (!user_interrupt && chars < 50000)
    {
        int colour = 16 + (chars % 215);
        printf("\033[4;3H\033[38;5;%dm%c", colour, 'A' + (chars % 26));
        chars++;
    }
    end_time = get_absolute_time();
    uint64_t cps_elapsed_us = absolute_time_diff_us(start_time, end_time);
    float cps_elapsed_seconds = cps_elapsed_us / 1000000.0;
    float chars_per_second = chars / cps_elapsed_seconds;

    printf("\n\n\n\033(B\033[m\033[?25h");
    printf("Speed test complete.\n");
    printf("\nRows processed: %d\n", row - 1);
    printf("Rows time elapsed: %.2f seconds\n", scrolling_elapsed_seconds);
    printf("Average rows per second: %.2f\n", rows_per_second);
    printf("\nCharacters processed: %d\n", chars);
    printf("Characters time elapsed: %.2f seconds\n", cps_elapsed_seconds);
    printf("Average characters per second: %.2f\n", chars_per_second);
}
