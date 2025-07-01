#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "pico/bootrom.h"
#include "pico/float.h"
#include "pico/util/datetime.h"
#include "pico/time.h"

#include "drivers/southbridge.h"
#include "drivers/audio.h"
#include "songs.h"
#include "commands.h"

volatile bool user_interrupt = false;

// Command table - map of command names to functions
static const command_t commands[] = {
    {"audiotest", audiotest, "Test the audio driver"},
    {"backlight", backlight, "Show the backlight levels"},
    {"battery", battery, "Show the battery level"},
    {"beep", beep, "Play a simple beep sound"},
    {"box", box, "Draw a box on the screen"},
    {"bye", bye, "Reboot into BOOTSEL mode"},
    {"cls", clearscreen, "Clear the screen"},
    {"play", play, "Play a song"},
    {"songs", songs, "List all available songs"},
    {"speedtest", speedtest, "Run a speed test"},
    {"help", help, "Show this help message"},
    {NULL, NULL} // Sentinel to mark end of array
};

// Extended song command that takes a parameter
static void play_named_song(const char* song_name)
{
    const audio_note_t* selected_song = find_song(song_name);
    if (!selected_song) {
        printf("Song '%s' not found.\n", song_name);
        printf("Use 'songs' command to see available songs.\n");
        return;
    }
    
    // Find the song info for display
    const char* song_title = "Unknown";
    for (int i = 0; song_list[i].name != NULL; i++) {
        if (strcmp(song_list[i].name, song_name) == 0) {
            song_title = song_list[i].description;
            break;
        }
    }
    
    printf("Now playing: %s\n", song_title);
    printf("Press BREAK key to stop...\n");
    
    audio_set_volume(75); // Set a good volume level
    
    // Reset user interrupt flag
    user_interrupt = false;
    
    play_song(selected_song);
    
    if (user_interrupt) {
        printf("\nPlayback interrupted by user.\n");
    } else {
        printf("\nSong finished!\n");
    }
}

void run_command(const char *command)
{
    bool command_found = false; // Flag to check if command was found
    
    // Make a copy of the command string for parsing
    char cmd_copy[256];
    strncpy(cmd_copy, command, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';
    
    // Parse command and arguments
    char *cmd_name = strtok(cmd_copy, " ");
    char *cmd_arg = strtok(NULL, " ");
    
    if (!cmd_name) {
        return; // Empty command
    }

    // Search for command in the table
    for (int i = 0; commands[i].name != NULL; i++)
    {
        if (strcmp(cmd_name, commands[i].name) == 0)
        {
            // Special handling for song command with argument
            if (strcmp(cmd_name, "play") == 0 && cmd_arg != NULL) {
                play_named_song(cmd_arg);
            } else {
                commands[i].function(); // Call the command function
            }
            command_found = true;   // Command found and executed
            break;                  // Exit the loop
        }
    }

    if (!command_found)
    {
        printf("%s ?\nType 'help' for a list of commands.\n", cmd_name);
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

void backlight()
{
    uint8_t lcd_backlight = sb_read_lcd_backlight();
    uint8_t keyboard_backlight = sb_read_keyboard_backlight();

    printf("LCD BackLight: %.0f%%\n", lcd_backlight / 2.55); // Convert to percentage
    printf("Keyboard BackLight: %.0f%%\n", keyboard_backlight / 2.55); // Convert to percentage
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

void beep()
{
    printf("Playing beep...\n");
    audio_play_tone(TONE_HIGH_BEEP, NOTE_QUARTER);
    printf("Beep complete.\n");
}

void audiotest()
{
    printf("Audio Driver Test\n");
    
    // Set volume to 75%
    audio_set_volume(75);
    printf("Volume set to 75%%.\n");
    
    printf("\nPlaying musical scale (C4 to C5):\n");
    
    // Play a simple C major scale
    uint16_t scale[] = {
        TONE_C4, TONE_D4, TONE_E4, TONE_F4, 
        TONE_G4, TONE_A4, TONE_B4, TONE_C5
    };
    
    const char* note_names[] = {
        "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5"
    };
    
    for (int i = 0; i < 8; i++) {
        if (user_interrupt) {
            printf("\nUser interrupt detected.\nStopping audio test.\n");
            break;
        }
        
        printf("Playing %s (%d Hz)...\n", note_names[i], scale[i]);
        audio_play_tone(scale[i], NOTE_HALF);
        sleep_ms(100); // Small gap between notes
    }
    
    printf("\nPlaying chord progression:\n");
    
    // Play some chords (just root notes for simplicity since we're mono)
    uint16_t chord_roots[] = {TONE_C4, TONE_F4, TONE_G4, TONE_C4};
    const char* chord_names[] = {"C Major", "F Major", "G Major", "C Major"};
    
    for (int i = 0; i < 4; i++) {
        if (user_interrupt) {
            printf("\nUser interrupt detected.\nStopping audio test.\n");
            break;
        }
        
        printf("Playing %s (%d Hz)...\n", chord_names[i], chord_roots[i]);
        audio_play_tone(chord_roots[i], NOTE_WHOLE);
        sleep_ms(200);
    }
    
    printf("\nTesting different volume levels:\n");
    
    for (int vol = 25; vol <= 100; vol += 25) {
        if (user_interrupt) {
            printf("\nUser interrupt detected.\nStopping audio test.\n");
            break;
        }
        
        audio_set_volume(vol);
        printf("Volume %d%% - Playing A4 (440 Hz)...\n", vol);
        audio_play_tone(TONE_A4, NOTE_HALF);
        sleep_ms(200);
    }
    
    printf("\nTesting frequency range:\n");
    
    // Test low to high frequency sweep
    uint16_t test_freqs[] = {
        TONE_LOW_BEEP, TONE_C3, TONE_C4, TONE_C5, TONE_C6, TONE_HIGH_BEEP
    };
    
    const char* freq_names[] = {
        "Low Beep (100 Hz)", "C3 (131 Hz)", "C4 (262 Hz)", 
        "C5 (523 Hz)", "C6 (1047 Hz)", "High Beep (2000 Hz)"
    };
    
    for (int i = 0; i < 6; i++) {
        if (user_interrupt) {
            printf("\nUser interrupt detected.\nStopping audio test.\n");
            break;
        }
        
        printf("Playing %s...\n", freq_names[i]);
        audio_play_tone(test_freqs[i], NOTE_QUARTER);
        sleep_ms(300);
    }
    
    // Test async playback
    printf("\nTesting asynchronous playback:\n");
    printf("Playing continuous tone for 3 seconds:\n");
    
    audio_play_tone_async(TONE_A4);
    
    for (int i = 3; i > 0; i--) {
        if (user_interrupt) {
            printf("\nUser interrupt detected. Stopping audio test.\n");
            break;
        }
        printf("%d...\n", i);
        sleep_ms(1000);
    }
    
    audio_stop();
    printf("Audio stopped.\n");
    
    printf("\nAudio test complete!\n");
    printf("Press BREAK key anytime during audio\nplayback to interrupt.\n");
}

void songs()
{
    printf("PicoCalc Song Library\n");
    list_songs();
    printf("\nUsage: song <name>\n");
    printf("Example: song mary\n");
    printf("\nPress BREAK key during playback to stop a song.\n");
}

void play()
{
    printf("Error: No song specified.\n");
    printf("Usage: song <name>\n");
    printf("Use 'songs' command to see available\nsongs.\n");
}
