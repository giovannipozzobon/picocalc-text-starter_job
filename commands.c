#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#include "pico/bootrom.h"
#include "pico/float.h"
#include "pico/util/datetime.h"
#include "pico/time.h"

#include "drivers/southbridge.h"
#include "drivers/audio.h"
#include "drivers/sdcard.h"
#include "drivers/fat32.h"
#include "drivers/lcd.h"
#include "drivers/keyboard.h"
#include "drivers/ds3231.h"
#include "songs.h"
#include "tests.h"
#include "commands.h"
#include "gfx.h"
#include "sprites.h"
#include "tiles.h"

#define STEP_Y 2
#define STEP_X 2

static gfx_sprite_t s;

volatile bool user_interrupt = false;
extern void readline(char *buffer, size_t size);
uint8_t columns = 40;

// Command table - map of command names to functions
static const command_t commands[] = {
    {"backlight", backlight, "Show/set the backlight"},
    {"battery", battery, "Show the battery level"},
    {"beep", beep, "Play a simple beep sound"},
    {"box", box, "Draw a box on the screen"},
    {"bye", bye, "Reboot into BOOTSEL mode"},
    {"cls", clearscreen, "Clear the screen"},
    {"cd", cd, "Change directory ('/' path sep.)"},
    {"dir", dir, "List files on the SD card"},
    {"free", sd_free, "Show free space on the SD card"},
    {"hexdump", hexdump, "Show hex dump of a file"},
    {"mkdir", sd_mkdir, "Create a new directory"},
    {"mkfile", sd_mkfile, "Create a new file"},
    {"mv", sd_mv, "Move or rename a file/directory"},
    {"more", sd_more, "Page through a file"},
    {"play", play, "Play a song"},
    {"poweroff", power_off, "Power off the device"},
    {"pwd", sd_pwd, "Print working directory"},
    {"reset", reset, "Reset the device"},
    {"rm", sd_rm, "Remove a file"},
    {"rmdir", sd_rmdir, "Remove a directory"},
    {"sdcard", sd_status, "Show SD card status"},
    {"showimg", showimg, "Display image from SD card"},
    {"songs", show_song_library, "Show song library"},
    {"test", test, "Run a test"},
    {"tests", show_test_library, "Show test library"},
    {"time", rtc_time, "Show/set DS3231 RTC time"},
    {"viewtext", viewtext, "View text file with scrolling"},
    {"width", width, "Set number of columns"},
    {"help", show_command_library, "Show this help message"},
    {"sprite", show_sprite, "Show the Sprite test"},
    {NULL, NULL, NULL} // Sentinel to mark end of array
};

char *strechr(const char *s, int c)
{
    do
    {
        if (*s == '\0')
        {
            return NULL;
        }
        if (*s == '\\')
        {
            s++; // Skip escaped character
            if (*s == '\0')
            {
                return NULL; // End of string after escape
            }
        }
        s++;
    } while (*s && *s != (char)c);

    return (char *)s;
}

char *condense(char *s)
{
    char *src = (char *)s;
    char *dst = (char *)s;

    while (*src)
    {
        if (*src != '\\')
        {
            *dst++ = *src++;
        }
        else
        {
            src++;
            if (*src == '\0')
            {
                break; // End of string after escape
            }
            *dst++ = *src++; // Copy the escaped character
        }
    }
    *dst = '\0';
    return s;
}

char *basename(const char *path)
{
    const char *last_slash = strrchr(path, '/');
    if (last_slash)
    {
        return (char *)(last_slash + 1); // Return after the last slash
    }
    return (char *)path; // No slashes, return the whole path
}

// Extended song command that takes a parameter
static void play_named_song(const char *song_name)
{
    const audio_song_t *song = find_song(song_name);
    if (!song)
    {
        printf("Song '%s' not found.\n", song_name);
        printf("Use 'songs' command to see available\nsongs.\n");
        return;
    }

    printf("\nNow playing:\n%s\n\n", song->description);
    printf("Press BREAK key to stop...\n");

    // Reset user interrupt flag
    user_interrupt = false;

    audio_play_song_blocking(song);

    if (user_interrupt)
    {
        printf("\nPlayback interrupted by user.\n");
    }
    else
    {
        printf("\nSong finished!\n");
    }
}

static void run_named_test(const char *test_name)
{
    const test_t *test = find_test(test_name);
    if (!test)
    {
        printf("Test '%s' not found.\n", test_name);
        printf("Use 'tests' command to see available\ntests.\n");
        return;
    }

    printf("Running test: %s\n", test->name);
    printf("Press BREAK key to stop...\n");

    // Reset user interrupt flag
    user_interrupt = false;
    test->function(); // Call the test function

    if (user_interrupt)
    {
        printf("\nTest interrupted by user.\n");
    }
    else
    {
        printf("\nTest finished!\n");
    }
}

void run_command(const char *command)
{
    bool command_found = false; // Flag to check if command was found

    // Make a copy of the command string for parsing
    char cmd_copy[256];
    char *cmd_args[8] = {NULL};
    strncpy(cmd_copy, command, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';

    // Parse command and arguments
    int arg_count = 0; // Start with one argument (the command name)
    char *cptr = cmd_copy;

    do
    {
        cmd_args[arg_count++] = cptr; // Set cmd_name to the current position
        cptr = strechr(cptr, ' ');
        if (*cptr)
        {
            *cptr++ = '\0'; // Terminate argument at the space
        }
    } while (cptr && *cptr && arg_count < 8);

    if (!cmd_args[0] || *cmd_args[0] == '\0')
    {
        return; // Empty command
    }

    // Search for command in the table
    for (int i = 0; commands[i].name != NULL; i++)
    {
        if (strcmp(cmd_args[0], commands[i].name) == 0)
        {
            // Special handling for song commands with arguments
            if (strcmp(cmd_args[0], "play") == 0 && cmd_args[1] != NULL)
            {
                play_named_song(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "more") == 0 && cmd_args[1] != NULL)
            {
                sd_read_filename(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "test") == 0 && cmd_args[1] != NULL)
            {
                run_named_test(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "dir") == 0 && cmd_args[1] != NULL)
            {
                sd_dir_dirname(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "cd") == 0 && cmd_args[1] != NULL)
            {
                cd_dirname(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "mkfile") == 0 && cmd_args[1] != NULL)
            {
                sd_mkfile_filename(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "mkdir") == 0 && cmd_args[1] != NULL)
            {
                sd_mkdir_filename(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "rm") == 0 && cmd_args[1] != NULL)
            {
                sd_rm_filename(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "rmdir") == 0 && cmd_args[1] != NULL)
            {
                sd_rmdir_dirname(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "mv") == 0 && cmd_args[1] != NULL && cmd_args[2] != NULL)
            {
                sd_mv_filename(condense(cmd_args[1]), condense(cmd_args[2]));
            }
            else if (strcmp(cmd_args[0], "width") == 0 && cmd_args[1] != NULL)
            {
                width_set(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "poweroff") == 0 && cmd_args[1] != NULL)
            {
                power_off_set(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "reset") == 0 && cmd_args[1] != NULL)
            {
                reset_set(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "backlight") == 0 && cmd_args[1] != NULL && cmd_args[2] != NULL)
            {
                backlight_set(condense(cmd_args[1]), condense(cmd_args[2]));
            }
            else if (strcmp(cmd_args[0], "time") == 0 && cmd_args[1] != NULL && cmd_args[2] != NULL)
            {
                rtc_time_set(condense(cmd_args[1]), condense(cmd_args[2]));
            }
            else if (strcmp(cmd_args[0], "hexdump") == 0 && cmd_args[1] != NULL)
            {
                hexdump_filename(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "showimg") == 0 && cmd_args[1] != NULL)
            {
                showimg_filename(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "viewtext") == 0 && cmd_args[1] != NULL)
            {
                viewtext_filename(condense(cmd_args[1]));
            }
            else if (strcmp(cmd_args[0], "show_sprite") == 0 && cmd_args[1] != NULL)
                        {
                show_sprite();
            }
            else
            {
                commands[i].function(); // Call the command function
            }
            command_found = true; // Command found and executed
            break;                // Exit the loop
        }
    }

    if (!command_found)
    {
        printf("%s ?\nType 'help' for a list of commands.\n", cmd_args[0]);
    }
    user_interrupt = false;
}

void show_command_library()
{
    printf("\033[?25l\033[4mCommand Library\033[0m\n\n");
    for (int i = 0; commands[i].name != NULL; i++)
    {
        printf("  \033[1m%s\033[0m - %s\n", commands[i].name, commands[i].description);
    }
    printf("\n\033[?25h");
}

void backlight()
{
    uint8_t lcd_backlight = sb_read_lcd_backlight();
    uint8_t keyboard_backlight = sb_read_keyboard_backlight();

    printf("LCD BackLight: %.0f%%\n", lcd_backlight / PERCENT_TO_BYTE_SCALE);           // Convert to percentage
    printf("Keyboard BackLight: %.0f%%\n", keyboard_backlight / PERCENT_TO_BYTE_SCALE); // Convert to percentage
}

void backlight_set(const char *display_level, const char *keyboard_level)
{
    int lcd_level = atoi(display_level);
    int key_level = atoi(keyboard_level);

    if (lcd_level < 0 || lcd_level > 100 || key_level < 0 || key_level > 100)
    {
        printf("Error: Invalid backlight level. Please enter values between 0 and 100.\n");
        return;
    }

    uint8_t lcd_backlight = (uint8_t)(lcd_level * PERCENT_TO_BYTE_SCALE);
    uint8_t keyboard_backlight = (uint8_t)(key_level * PERCENT_TO_BYTE_SCALE);

    uint8_t lcd_result = sb_write_lcd_backlight(lcd_backlight);
    uint8_t keyboard_result = sb_write_keyboard_backlight(keyboard_backlight);

    printf("LCD BackLight set to: %d, claims %d\n", lcd_backlight, lcd_result);
    printf("Keyboard BackLight set to: %d, claims %d\n", keyboard_backlight, keyboard_result);
}

void battery()
{
    int raw_level = sb_read_battery();
    int battery_level = raw_level & 0x7F;    // Mask out the charging bit
    bool charging = (raw_level & 0x80) != 0; // Check if charging

    printf("\033[?25l\033(0");
    if (charging)
    {
        printf("\033[38;5;220m");
    }
    else
    {
        printf("\033[38;5;231m");
    }
    printf("lqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqk\n");
    printf("x ");
    if (battery_level < 10)
    {
        printf("\033[38;5;196;7m"); // Set background colour to red for critical battery
    }
    else if (battery_level < 30)
    {
        printf("\033[38;5;226;7m"); // Set background colour to yellow for low battery
    }
    else
    {
        printf("\033[38;5;46;7m"); // Set background colour to green for sufficient battery
    }
    for (int i = 0; i < battery_level / 3; i++)
    {
        printf(" "); // Print a coloured space for each 3% of battery
    }
    printf("\033[0;38;5;242m"); // Reset colour
    for (int i = battery_level / 3; i < 33; i++)
    {
        printf("a"); // Fill the rest of the bar with fuzz
    }
    if (charging)
    {
        printf("\033[38;5;220m");
    }
    else
    {
        printf("\033[38;5;231m");
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

void beep()
{
    printf("Playing beep...\n");
    audio_play_sound_blocking(HIGH_BEEP, HIGH_BEEP, NOTE_QUARTER);
    printf("Beep complete.\n");
}

void box()
{
    printf("A box using the DEC Special Character\nSet:\n\n");
    printf("\033[38;5;208m"); // Set foreground colour to orange
    printf("\033[?25l");      // Hide cursor

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

void play()
{
    printf("Error: No song specified.\n");
    printf("Usage: play <name>\n");
    printf("Use 'songs' command to see available\nsongs.\n");
}

void test()
{
    printf("Error: No test specified.\n");
    printf("Usage: test <name>\n");
    printf("Use 'tests' command to see available\ntests.\n");
}

void width(void)
{
    printf("Error: No width specified.\n");
    printf("Usage: width 40|64\n");
    printf("Example: width 40\n");
    printf("Sets the terminal width for text output.\n");
}

void width_set(const char *width)
{
    if (width == NULL || strlen(width) == 0)
    {
        printf("Error: No width specified.\n");
        printf("Usage: width <width>\n");
        return;
    }

    if (strcmp(width, "40") == 0)
    {
        columns = 40;
        lcd_set_font(&font_8x10);
    }
    else if (strcmp(width, "64") == 0)
    {
        columns = 64;
        lcd_set_font(&font_5x10);
    }
    else
    {
        printf("Error: Invalid width '%s'.\n", width);
        printf("Valid widths are 40 or 64 characters.\n");
        return;
    }

    printf("Terminal width set to %s characters.\n", width);
}

void power_off(void)
{
    printf("Error: No delay specified.\n");
    printf("Usage: poweroff <seconds>\n");
    printf("Example: poweroff 10\n");
    printf("Set the poweroff delay.\n");
}

void power_off_set(const char *seconds)
{
    if (sb_is_power_off_supported())
    {
        int delay = atoi(seconds);
        printf("Poweroff delay set to %d seconds.\n", delay);
        sb_write_power_off_delay(delay);
    }
    else
    {
        printf("Poweroff not supported on this device.\n");
    }
}

void reset()
{
    printf("Resetting the device in one second...\n");
    sb_reset(1);
}

void reset_set(const char *seconds)
{
    int delay = atoi(seconds);
    if (delay < 0 || delay > 255)
    {
        printf("Error: Invalid delay '%s'.\n", seconds);
        printf("Delay must be between 0 and 255 seconds.\n");
        return;
    }
    printf("Resetting the device in %d seconds...\n", delay);
    sb_reset((uint8_t)delay);
}


//
// SD Card Commands
//

static void get_str_size(char *buffer, uint32_t buffer_size, uint64_t bytes)
{
    const char *unit = "bytes";
    uint32_t divisor = 1;

    if (bytes >= 1000 * 1000 * 1000)
    {
        divisor = 1000 * 1000 * 1000;
        unit = "GB";
    }
    else if (bytes >= 1000 * 1000)
    {
        divisor = 1000 * 1000;
        unit = "MB";
    }
    else if (bytes >= 1000)
    {
        divisor = 1000;
        unit = "KB";
    }

    if (strcmp(unit, "bytes") == 0 || strcmp(unit, "KB") == 0)
    {
        snprintf(buffer, buffer_size, "%llu %s", (unsigned long long)(bytes / divisor), unit);
    }
    else
    {
        snprintf(buffer, buffer_size, "%.1f %s", ((float)bytes) / divisor, unit);
    }
}

void sd_status()
{
    if (!sd_card_present())
    {
        printf("SD card not inserted\n");
        return;
    }

    fat32_error_t mount_status = fat32_get_status();
    if (mount_status != FAT32_OK)
    {
        printf("SD card inserted, but unreadable.\n");
        printf("Error: %s\n", fat32_error_string(mount_status));
        return;
    }

    uint64_t total_space;
    fat32_error_t result = fat32_get_total_space(&total_space);
    if (result != FAT32_OK)
    {
        printf("SD card inserted, unable to get total space.\n");
        printf("Error: %s\n", fat32_error_string(result));
        return;
    }
    char buffer[32];
    fat32_get_volume_name(buffer, sizeof(buffer));
    printf("SD card inserted, ready to use.\n");
    printf("  Volume name: %s\n", buffer[0] ? buffer : "No volume label");
    get_str_size(buffer, sizeof(buffer), total_space);
    printf("  Capacity: %s\n", buffer);
    bool is_sdhc = sd_is_sdhc();
    printf("  Type: %s\n", is_sdhc ? "SDHC" : "SDSC");
    get_str_size(buffer, sizeof(buffer), fat32_get_cluster_size());
    printf("  Cluster size: %s\n", buffer);
}

void sd_free()
{
    uint64_t free_space;
    sd_error_t result = fat32_get_free_space(&free_space);

    if (result == SD_OK)
    {
        char size_buffer[32];
        get_str_size(size_buffer, sizeof(size_buffer), free_space);
        printf("Free space on SD card: %s\n", size_buffer);
    }
    else
    {
        printf("Error: %s\n", fat32_error_string(result));
    }
}

void cd(void)
{
    cd_dirname("/"); // Default to root directory
}

void cd_dirname(const char *dirname)
{
    if (dirname == NULL || strlen(dirname) == 0)
    {
        printf("Error: No directory specified.\n");
        printf("Usage: cd <dirname>\n");
        printf("Example: cd /mydir\n");
        return;
    }

    sd_error_t result = fat32_set_current_dir(dirname);
    if (result != SD_OK)
    {
        printf("Error: %s\n", fat32_error_string(result));
        return;
    }
}

void sd_pwd()
{
    char current_dir[FAT32_MAX_PATH_LEN];
    sd_error_t result = fat32_get_current_dir(current_dir, sizeof(current_dir));
    if (result != SD_OK)
    {
        printf("Error: %s\n", fat32_error_string(result));
        return;
    }
    printf("%s\n", current_dir);
}

void dir()
{
    sd_dir_dirname("."); // Show root directory
}

void sd_dir_dirname(const char *dirname)
{
    fat32_file_t dir;
    fat32_entry_t dir_entry;

    fat32_error_t result = fat32_open(&dir, dirname);
    if (result != FAT32_OK)
    {
        printf("Error: %s\n", fat32_error_string(result));
        return;
    }

    do
    {
        result = fat32_dir_read(&dir, &dir_entry);
        if (result != FAT32_OK)
        {
            printf("Error: %s\n", fat32_error_string(result));
            return;
        }
        if (dir_entry.filename[0])
        {
            if (dir_entry.attr & (FAT32_ATTR_VOLUME_ID | FAT32_ATTR_HIDDEN | FAT32_ATTR_SYSTEM))
            {
                // It's a volume label, hidden file, or system file, skip it
                continue;
            }
            else if (dir_entry.attr & FAT32_ATTR_DIRECTORY)
            {
                // It's a directory, append '/' to the name
                printf("%s/\n", dir_entry.filename);
            }
            else
            {
                char size_buffer[16];
                get_str_size(size_buffer, sizeof(size_buffer), dir_entry.size);
                printf("%-28s %10s\n", dir_entry.filename, size_buffer);
            }
        }
    } while (dir_entry.filename[0]);

    fat32_close(&dir);
}

void sd_more()
{
    printf("Error: No filename specified.\n");
    printf("Usage: more <filename>\n");
    printf("Example: more readme.txt\n");
}

void sd_read_filename(const char *filename)
{
    if (filename == NULL || strlen(filename) == 0)
    {
        printf("Error: No filename specified.\n");
        printf("Usage: sd_read <filename>\n");
        printf("Example: sd_read readme.txt\n");
        return;
    }

    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Cannot open file '%s':\n%s\n", filename, strerror(errno));
        return;
    }

    // Read and display the file content with pagination
    char buffer[1024]; // Read in larger chunks
    size_t bytes_read;
    uint32_t total_bytes_read = 0;
    int line_count = 0;
    bool user_quit = false;

    printf("\033[2J\033[H");

    while (!user_quit && !feof(fp))
    {
        bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
        if (bytes_read == 0)
        {
            if (ferror(fp))
            {
                printf("Error reading file '%s':\n%s\n", filename, strerror(errno));
            }
            break;
        }
        if (bytes_read < sizeof(buffer) - 1)
        {
            buffer[bytes_read] = '\0'; // Null terminate the string
        }
        else
        {
            buffer[sizeof(buffer) - 1] = '\0'; // Ensure null termination
        }
        total_bytes_read += bytes_read;

        // Display the content line by line
        char *line_start = buffer;
        char *line_end;

        while ((line_end = strchr(line_start, '\n')) != NULL || line_start < buffer + bytes_read)
        {
            if (line_end == NULL)
            {
                // Last line without newline
                printf("%s", line_start);
                break;
            }

            // Print the line (including newline)
            *line_end = '\0';
            printf("%s\n", line_start);
            size_t line_len = strlen(line_start);
            int screen_lines = (int)((line_len + columns - 1) / columns); // Round up
            if (screen_lines == 0)
            {
                screen_lines = 1;
            }
            line_count += screen_lines;
            // Check if we need to pause
            if (line_count > 30)
            {
                printf("More?");
                char ch = getchar();
                if (ch == 'q' || ch == 'Q')
                {
                    user_quit = true;
                    printf("\n");
                    break;
                }
                printf("\033[2J\033[H");
                line_count = 0;
            }

            line_start = line_end + 1;
        }

        if (user_quit)
            break;
    }

    fclose(fp);
}

void sd_mkfile()
{
    printf("Error: No filename specified.\n");
    printf("Usage: mkfile <filename>\n");
    printf("Example: mkfile newfile.txt\n");
}

void sd_mkfile_filename(const char *filename)
{
    if (filename == NULL || strlen(filename) == 0)
    {
        printf("Error: No filename specified.\n");
        printf("Usage: mkfile <filename>\n");
        printf("Example: mkfile newfile.txt\n");
        return;
    }

    FILE *fp;
    fp = fopen(filename, "wx+");
    if (fp == NULL)
    {
        printf("Cannot create file '%s':\n%s\n", filename, strerror(errno));
        return;
    }

    // Get lines of text and write them to the file
    // If a line is only a dot, stop reading, and close the file
    printf("Enter text to write to the file,\nfinish with a single dot:\n");
    char line[38];
    uint32_t total_bytes_written = 0;
    while (true)
    {
        printf("> ");
        readline(line, sizeof(line));
        if (strcmp(line, ".") == 0)
        {
            break; // Stop reading on a single dot
        }
        size_t bytes_written;
        size_t remaining_space = sizeof(line) - strlen(line) - 1;
        strncat(line, "\n", remaining_space);
        bytes_written = fwrite(line, 1, strlen(line), fp);
        if (bytes_written < strlen(line))
        {
            printf("Warning: Not all bytes written!\n");
        }
        total_bytes_written += bytes_written;
    }

    fclose(fp);

    printf("File '%s' created\nwith %lu bytes written.\n", filename, total_bytes_written);
}

void sd_mkdir()
{
    printf("Error: No directory name specified.\n");
    printf("Usage: mkdir <dirname>\n");
    printf("Example: mkdir newdir\n");
}

void sd_mkdir_filename(const char *dirname)
{
    if (dirname == NULL || strlen(dirname) == 0)
    {
        printf("Error: No directory name specified.\n");
        printf("Usage: mkdir <dirname>\n");
        printf("Example: mkdir newdir\n");
        return;
    }

    fat32_file_t dir;
    fat32_error_t result = fat32_dir_create(&dir, dirname);
    if (result != FAT32_OK)
    {
        printf("Error: %s\n", fat32_error_string(result));
        return;
    }

    printf("Directory '%s' created.\n", dirname);
    fat32_close(&dir);
}

void sd_rm()
{
    printf("Error: No filename specified.\n");
    printf("Usage: rm <filename>\n");
    printf("Example: rm oldfile.txt\n");
}

void sd_rm_filename(const char *filename)
{
    if (filename == NULL || strlen(filename) == 0)
    {
        printf("Error: No filename specified.\n");
        printf("Usage: rm <filename>\n");
        printf("Example: rm oldfile.txt\n");
        return;
    }

    fat32_error_t result = fat32_delete(filename);
    if (result != FAT32_OK)
    {
        printf("Error: %s\n", fat32_error_string(result));
        return;
    }

    printf("File '%s' removed.\n", filename);
}

void sd_rmdir()
{
    printf("Error: No directory name specified.\n");
    printf("Usage: rmdir <dirname>\n");
    printf("Example: rmdir olddir\n");
}

void sd_rmdir_dirname(const char *dirname)
{
    if (dirname == NULL || strlen(dirname) == 0)
    {
        printf("Error: No directory name specified.\n");
        printf("Usage: rmdir <dirname>\n");
        printf("Example: rmdir olddir\n");
        return;
    }

    fat32_error_t result = fat32_delete(dirname);
    if (result != FAT32_OK)
    {
        printf("Error: %s\n", fat32_error_string(result));
        return;
    }

    printf("Directory '%s' removed.\n", dirname);
}

void sd_mv(void)
{
    printf("Error: No source or destination specified.\n");
    printf("Usage: mv <oldname> <newname>\n");
    printf("Example: mv oldfile.txt newfile.txt\n");
}

void sd_mv_filename(const char *oldname, const char *newname)
{
    if (oldname == NULL || strlen(oldname) == 0 || newname == NULL || strlen(newname) == 0)
    {
        printf("Error: No source or destination specified.\n");
        printf("Usage: mv <oldname> <newname>\n");
        printf("Example: mv oldfile.txt newfile.txt\n");
        return;
    }

    struct stat st;
    char full_newname[FAT32_MAX_PATH_LEN];

    if (stat(newname, &st) == 0 && S_ISDIR(st.st_mode))
    {
        // newname is a directory, append basename of oldname
        const char *old_basename = basename((char *)oldname);
        size_t len = strlen(newname);
        snprintf(full_newname, sizeof(full_newname), "%s%s%s",
                 newname,
                 (len > 0 && newname[len - 1] != '/') ? "/" : "",
                 old_basename);
        newname = full_newname;
    }

    if (rename(oldname, newname) < 0)
    {
        printf("Cannot move\n'%s'\nto\n'%s':\n%s\n", oldname, newname, strerror(errno));
        return;
    }

    printf("'%s' moved to '%s'.\n", oldname, newname);
}

//
// RTC DS3231 Commands
//

// Array con i nomi dei giorni della settimana
static const char *day_names[] = {
    "???", "Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab"
};

void rtc_time(void)
{
    ds3231_datetime_t dt;

    if (!ds3231_read_time(&dt))
    {
        printf("Error reading DS3231 RTC.\n");
        printf("Check I2C connection.\n");
        return;
    }

    // Display date and time in readable format
    printf("Date: %s %02d/%02d/20%02d\n",
           day_names[dt.day], dt.date, dt.month, dt.year);
    printf("Time: %02d:%02d:%02d\n",
           dt.hours, dt.minutes, dt.seconds);
}

void rtc_time_set(const char *date, const char *time)
{
    if (date == NULL || time == NULL)
    {
        printf("Error: Missing parameters.\n");
        printf("Usage: time <dd/mm/yy> <hh:mm:ss>\n");
        printf("Example: time 15/03/25 14:30:00\n");
        printf("Days: 1=Sun, 2=Mon, 3=Tue, 4=Wed,\n");
        printf("      5=Thu, 6=Fri, 7=Sat\n");
        return;
    }

    ds3231_datetime_t dt;
    int day, month, year;
    int hours, minutes, seconds;

    // Parse date (dd/mm/yy)
    if (sscanf(date, "%d/%d/%d", &day, &month, &year) != 3)
    {
        printf("Error: Invalid date format.\n");
        printf("Use: dd/mm/yy\n");
        printf("Example: 15/03/25\n");
        return;
    }

    // Parse time (hh:mm:ss)
    if (sscanf(time, "%d:%d:%d", &hours, &minutes, &seconds) != 3)
    {
        printf("Error: Invalid time format.\n");
        printf("Use: hh:mm:ss\n");
        printf("Example: 14:30:00\n");
        return;
    }

    // Validate values
    if (day < 1 || day > 31 || month < 1 || month > 12 || year > 99)
    {
        printf("Error: Invalid date.\n");
        printf("Day: 1-31, Month: 1-12, Year: 0-99\n");
        return;
    }

    if (hours > 23 || minutes > 59 || seconds > 59)
    {
        printf("Error: Invalid time.\n");
        printf("Hours: 0-23, Minutes: 0-59, Seconds: 0-59\n");
        return;
    }

    // Calculate day of week using Zeller's algorithm
    // For Gregorian calendar
    int d = day;
    int m = month;
    int y = 2000 + year;

    if (m < 3)
    {
        m += 12;
        y--;
    }

    int dow = (d + (13 * (m + 1)) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
    // Convert: 0=Sat, 1=Sun, 2=Mon, ... -> 1=Sun, 2=Mon, ..., 7=Sat
    dow = (dow == 0) ? 7 : dow;

    // Prepare datetime structure
    dt.seconds = seconds;
    dt.minutes = minutes;
    dt.hours = hours;
    dt.day = dow;
    dt.date = day;
    dt.month = month;
    dt.year = year;

    // Write to DS3231
    if (!ds3231_write_time(&dt))
    {
        printf("Error writing to DS3231 RTC.\n");
        return;
    }

    printf("DS3231 RTC configured:\n");
    printf("Date: %s %02d/%02d/20%02d\n",
           day_names[dt.day], dt.date, dt.month, dt.year);
    printf("Time: %02d:%02d:%02d\n",
           dt.hours, dt.minutes, dt.seconds);
}

//
// File Viewer Commands
//

void hexdump(void)
{
    printf("Error: No file specified.\n");
    printf("Uso: hexdump <filename>\n");
    printf("Esempio: hexdump image.raw\n");
    printf("Mostra il contenuto esadecimale\ndi un file.\n");
}

void hexdump_filename(const char *filename)
{
    if (filename == NULL || strlen(filename) == 0)
    {
        printf("Error: No file specified.\n");
        printf("Uso: hexdump <filename>\n");
        return;
    }

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("Cannot open file '%s':\n%s\n", filename, strerror(errno));
        return;
    }

    // Ottieni la dimensione del file
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    printf("File: %s (%ld bytes)\n\n", filename, file_size);

    uint8_t buffer[6];  // Ridotto a 6 bytes per riga
    size_t bytes_read;
    uint32_t offset = 0;
    int line_count = 0;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        // Stampa offset
        printf("%06lx:", (unsigned long)offset);

        // Stampa hex
        for (size_t i = 0; i < 6; i++)
        {
            if (i < bytes_read)
            {
                printf(" %02x", buffer[i]);
            }
            else
            {
                printf("   ");
            }
        }

        printf(" |");

        // Stampa ASCII
        for (size_t i = 0; i < bytes_read; i++)
        {
            if (buffer[i] >= 32 && buffer[i] < 127)
            {
                printf("%c", buffer[i]);
            }
            else
            {
                printf(".");
            }
        }

        printf("|\n");

        offset += bytes_read;
        line_count++;

        // Pausa ogni 30 righe per evitare scrolling
        if (line_count > 0 && line_count % 30 == 0 && !feof(fp))
        {
            printf("Press any key to continue\n(or 'q' to quit)...");
            char ch = getchar();
            if (ch == 'q' || ch == 'Q')
            {
                printf("\n");
                break;
            }
            printf("\n");
        }
    }

    fclose(fp);
    printf("\nFine file. %ld bytes totali.\n", file_size);
}


//
// Sprite Test
//


void sprite_frame(int16_t *sx, int16_t *velocity) {
    /* in loop: move sprite and present */

    // Update position
    *sx += *velocity;

    // Check borders and reverse direction
    // WIDTH is defined in lcd.h (screen width)
    // Sprite is 16x16 pixels, so it stops at WIDTH - 16
    if (*sx >= WIDTH - 16) {
        *sx = WIDTH - 16;
        *velocity = -1; // reverse direction to left
    }
    else if (*sx <= 0) {
        *sx = 0;
        *velocity = 1; // reverse direction to right
    }

    gfx_move_sprite(s, *sx, 40);
    gfx_present();
}

void show_sprite(void)
{
    // Local variables for sprite position
    int16_t sx = 40;
    int16_t sy = 40;

    // Hide cursor
    lcd_enable_cursor(false);
    /* initialize gfx with tilesheet from tiles.h */
    gfx_init(my_tilesheet, my_tilesheet_count);

    /* prepare map: create a complete scene */
    gfx_clear_backmap(34);  // sky as background

    // Create floor at bottom (rows 18-19, screen 320x320 = 20x20 tiles)
    for (uint16_t x = 0; x < 20; x++) {
        gfx_set_tile(x, 18, 0);  // grass
        gfx_set_tile(x, 19, 0);  // dirt below grass
    }

    // Create central platform with brick wall
    for (uint16_t x = 5; x <= 10; x++) {
        gfx_set_tile(x, 14, 90);  // red bricks
        gfx_set_tile(x, 15, 90);  // red bricks
    }

    // Gray floor on platform
    for (uint16_t x = 5; x <= 10; x++) {
        gfx_set_tile(x, 13, 48);  // gray floor
    }

    // Water on right
    for (uint16_t y = 16; y <= 19; y++) {
        for (uint16_t x = 15; x <= 19; x++) {
            gfx_set_tile(x, y, 6);  // water
        }
    }

    // Sand near water
    for (uint16_t y = 16; y <= 19; y++) {
        gfx_set_tile(14, y, 122);  // sand
    }

    // Stone wall on left
    for (uint16_t y = 15; y <= 19; y++) {
        gfx_set_tile(0, y, 30);  // stone wall
        gfx_set_tile(1, y, 30);  // stone wall
    }

    /* create sprite (w=16,h=16) */
    s = gfx_create_sprite(sprite1_pixels, 16, 16, sx, sy, 0);

    /* present: draw modified tiles and sprite */
    gfx_present();

    // Continuous loop until user presses ESC
    while (true) {
        // Check if a key is pressed from physical keyboard (non-blocking)
        if (keyboard_key_available()){
            int key = keyboard_get_key();
            if (key != -1) {
                // Handle key press
                if (key == KEY_ESC) {  // ESC key
                    break;
                }
                // Arrow keys
                else if (key == KEY_UP) {  // UP arrow
                    sy -= STEP_Y;
                    if (sy < 0) sy = 0;
                }
                else if (key == KEY_DOWN) {  // DOWN arrow
                    sy += STEP_Y;
                    if (sy > HEIGHT - 16) sy = HEIGHT - 16;
                }
                else if (key == KEY_RIGHT) {  // RIGHT arrow
                    sx += STEP_X;
                    if (sx > WIDTH - 16) sx = WIDTH - 16;
                }
                else if (key == KEY_LEFT) {  // LEFT arrow
                    sx -= STEP_X;
                    if (sx < 0) sx = 0;
                }

                // Update sprite position
                gfx_move_sprite(s, sx, sy);
                gfx_present();
            }
        }

        //sleep_ms(16);  // ~60 FPS
    }

    // Destroy sprite before cleanup
    gfx_destroy_sprite(s);

    // Restore text screen and cursor
    lcd_clear_screen();
    lcd_enable_cursor(true);

}


//
// Image Display Commands
//

void showimg(void)
{
    printf("Error: No file specified.\n");
    printf("Uso: showimg <filename>\n");
    printf("Esempio: showimg image.raw\n");
    printf("\nFormato file RAW RGB565:\n");
    printf("  Byte 0-1: Larghezza (16-bit LE)\n");
    printf("  Byte 2-3: Altezza (16-bit LE)\n");
    printf("  Byte 4+:  Pixel RGB565 (2 byte/pixel)\n");
}

void showimg_filename(const char *filename)
{
    if (filename == NULL || strlen(filename) == 0)
    {
        printf("Error: No file specified.\n");
        printf("Uso: showimg <filename>\n");
        return;
    }

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("Cannot open file '%s':\n%s\n", filename, strerror(errno));
        return;
    }

    // Read header (width and height) - little-endian format
    uint16_t img_width, img_height;
    uint8_t header_bytes[4];

    // Read 4 bytes of header
    if (fread(header_bytes, 1, 4, fp) != 4)
    {
        printf("Error: File too small or corrupted.\n");
        fclose(fp);
        return;
    }

    // Convert from little-endian to uint16_t
    img_width = header_bytes[0] | (header_bytes[1] << 8);
    img_height = header_bytes[2] | (header_bytes[3] << 8);

    // Validate dimensions
    if (img_width == 0 || img_height == 0 || img_width > WIDTH || img_height > HEIGHT)
    {
        printf("Error: Invalid dimensions.\n");
        printf("Maximum: %dx%d pixels\n", WIDTH, HEIGHT);
        fclose(fp);
        return;
    }

    // Allocate buffer for one line at a time (to save memory)
    uint16_t *line_buffer = (uint16_t *)malloc(img_width * sizeof(uint16_t));
    if (line_buffer == NULL)
    {
        printf("Error: Insufficient memory.\n");
        fclose(fp);
        return;
    }

    // Center image on screen
    uint16_t x_offset = (WIDTH - img_width) / 2;
    uint16_t y_offset = (HEIGHT - img_height) / 2;

    // Hide cursor
    lcd_enable_cursor(false);

    // Clear screen (black)
    lcd_solid_rectangle(0x0000, 0, 0, WIDTH, HEIGHT);

    // Read and draw line by line
    for (uint16_t y = 0; y < img_height; y++)
    {
        size_t pixels_read = fread(line_buffer, sizeof(uint16_t), img_width, fp);
        if (pixels_read != img_width)
        {
            break;
        }

        // Draw line on screen
        lcd_blit(line_buffer, x_offset, y_offset + y, img_width, 1);
    }

    free(line_buffer);
    fclose(fp);

    // Wait for user input
    getchar();

    // Restore text screen and cursor
    lcd_clear_screen();
    lcd_enable_cursor(true);
}

//
// Text File Viewer Command
//

void viewtext(void)
{
    printf("Error: No file specified.\n");
    printf("Usage: viewtext <filename>\n");
    printf("Example: viewtext readme.txt\n");
}

void viewtext_filename(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Cannot open file '%s':\n%s\n", filename, strerror(errno));
        return;
    }

    // Count lines and characters
    int total_lines = 0;
    long total_chars = 0;
    int ch;

    while ((ch = fgetc(fp)) != EOF)
    {
        total_chars++;
        if (ch == '\n')
            total_lines++;
    }

    // If file doesn't end with newline, count last line
    if (total_chars > 0)
    {
        fseek(fp, -1, SEEK_END);
        if (fgetc(fp) != '\n')
            total_lines++;
    }

    // Reset to beginning
    fseek(fp, 0, SEEK_SET);

    // Read all lines into memory
    char **lines = NULL;
    int line_capacity = 100;
    int line_count = 0;

    lines = (char **)malloc(line_capacity * sizeof(char *));
    if (lines == NULL)
    {
        printf("Error: Insufficient memory.\n");
        fclose(fp);
        return;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        // Expand array if needed
        if (line_count >= line_capacity)
        {
            line_capacity *= 2;
            char **new_lines = (char **)realloc(lines, line_capacity * sizeof(char *));
            if (new_lines == NULL)
            {
                printf("Error: Insufficient memory.\n");
                for (int i = 0; i < line_count; i++)
                    free(lines[i]);
                free(lines);
                fclose(fp);
                return;
            }
            lines = new_lines;
        }

        // Allocate and copy line
        size_t len = strlen(buffer);
        // Remove newline if present
        if (len > 0 && buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';

        lines[line_count] = (char *)malloc(strlen(buffer) + 1);
        if (lines[line_count] == NULL)
        {
            printf("Error: Insufficient memory.\n");
            for (int i = 0; i < line_count; i++)
                free(lines[i]);
            free(lines);
            fclose(fp);
            return;
        }
        strcpy(lines[line_count], buffer);
        line_count++;
    }

    fclose(fp);

    // Now display the file with scrolling
    lcd_clear_screen();
    lcd_enable_cursor(false);

    int scroll_pos = 0;
    int prev_scroll_pos = -1; // Track previous position
    const int status_lines = 2; // Lines for status bar
    const int text_lines = 24 - status_lines; // Available lines for text

    // Draw status bar once (it doesn't change)
    printf("\033[1;1H"); // Move to row 1, column 1
    printf("\033[7m"); // Reverse video
    printf("%-40s", filename);
    printf("\033[0m"); // Reset

    // Second line of status
    printf("\033[2;1H"); // Move to row 2, column 1
    printf("\033[7m");
    char status_line[41];
    snprintf(status_line, sizeof(status_line),
             "Chars:%ld Lines:%d",
             total_chars, total_lines);
    printf("%-40s", status_line);
    printf("\033[0m"); // Reset

    while (true)
    {
        // Only redraw if scroll position changed
        if (scroll_pos != prev_scroll_pos)
        {
            // Display text lines
            for (int i = 0; i < text_lines; i++)
            {
                int line_index = scroll_pos + i;
                int screen_row = i + 3; // Start from row 3 (after status bar)

                printf("\033[%d;1H", screen_row); // Move to specific row, column 1

                if (line_index < line_count)
                {
                    // Truncate line if too long (40 chars max)
                    char display_line[41];
                    strncpy(display_line, lines[line_index], 40);
                    display_line[40] = '\0';
                    printf("%-40s", display_line);
                }
                else
                {
                    // Empty line
                    printf("%-40s", "");
                }
            }

            prev_scroll_pos = scroll_pos;
        }

        // Wait for key
        char key = getchar();

        if (key == KEY_ESC || key == 'q' || key == 'Q')
        {
            break;
        }
        else if (key == KEY_UP && scroll_pos > 0)
        {
            scroll_pos--;
        }
        else if (key == KEY_DOWN && scroll_pos < line_count - text_lines)
        {
            if (scroll_pos < line_count - 1)
                scroll_pos++;
        }
        else if (key == KEY_PAGE_UP)
        {
            scroll_pos -= text_lines;
            if (scroll_pos < 0)
                scroll_pos = 0;
        }
        else if (key == KEY_PAGE_DOWN)
        {
            scroll_pos += text_lines;
            if (scroll_pos > line_count - text_lines)
                scroll_pos = line_count - text_lines;
            if (scroll_pos < 0)
                scroll_pos = 0;
        }
    }

    // Free memory
    for (int i = 0; i < line_count; i++)
        free(lines[i]);
    free(lines);

    // Restore screen
    lcd_clear_screen();
    lcd_enable_cursor(true);
}


