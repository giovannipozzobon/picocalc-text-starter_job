#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drivers/audio.h"
#include "tests.h"

extern volatile bool user_interrupt;

// Helper function names corrected
void play_stereo_melody_demo()
{
    printf("Playing stereo melody demo...\n");
    printf("Listen for the melody bouncing between\n");
    printf("left and right channels!\n\n");

    // Define a simple melody with stereo panning
    struct
    {
        uint16_t left_freq;
        uint16_t right_freq;
        uint32_t duration;
        const char *note_name;
    } stereo_melody[] = {
        // Twinkle Twinkle Little Star with stereo panning
        {PITCH_C4, SILENCE, NOTE_QUARTER, "C4 (Left)"},
        {SILENCE, PITCH_C4, NOTE_QUARTER, "C4 (Right)"},
        {PITCH_G4, SILENCE, NOTE_QUARTER, "G4 (Left)"},
        {SILENCE, PITCH_G4, NOTE_QUARTER, "G4 (Right)"},
        {PITCH_A4, SILENCE, NOTE_QUARTER, "A4 (Left)"},
        {SILENCE, PITCH_A4, NOTE_QUARTER, "A4 (Right)"},
        {PITCH_G4, PITCH_G4, NOTE_HALF, "G4 (Both)"}, // Both channels for emphasis

        // Second phrase
        {SILENCE, PITCH_F4, NOTE_QUARTER, "F4 (Right)"},
        {PITCH_F4, SILENCE, NOTE_QUARTER, "F4 (Left)"},
        {SILENCE, PITCH_E4, NOTE_QUARTER, "E4 (Right)"},
        {PITCH_E4, SILENCE, NOTE_QUARTER, "E4 (Left)"},
        {SILENCE, PITCH_D4, NOTE_QUARTER, "D4 (Right)"},
        {PITCH_D4, SILENCE, NOTE_QUARTER, "D4 (Left)"},
        {PITCH_C4, PITCH_C4, NOTE_HALF, "C4 (Both)"}, // Both channels for ending

        // End marker
        {SILENCE, SILENCE, 0, "End"}};

    int i = 0;
    while (stereo_melody[i].duration > 0)
    {
        if (user_interrupt)
        {
            printf("\nUser interrupt detected.\n");
            audio_stop();
            return;
        }

        printf("  %s\n", stereo_melody[i].note_name);
        audio_play_sound_blocking(
            stereo_melody[i].left_freq,
            stereo_melody[i].right_freq,
            stereo_melody[i].duration);
        sleep_ms(50); // Small gap between notes
        i++;
    }

    printf("\nStereo melody demo complete!\n");
}

void play_stereo_harmony_demo()
{
    printf("Playing stereo harmony demo...\n");
    printf("Listen for harmonious intervals played\n");
    printf("simultaneously in both channels!\n\n");

    // Define chord progression with harmony
    struct
    {
        uint16_t left_freq;
        uint16_t right_freq;
        uint32_t duration;
        const char *chord_name;
    } harmony_progression[] = {
        // C Major chord progression
        {PITCH_C4, PITCH_E4, NOTE_WHOLE, "C Major (C4-E4)"},
        {PITCH_F4, PITCH_A4, NOTE_WHOLE, "F Major (F4-A4)"},
        {PITCH_G4, PITCH_B4, NOTE_WHOLE, "G Major (G4-B4)"},
        {PITCH_C4, PITCH_E4, NOTE_WHOLE, "C Major (C4-E4)"},

        // With bass notes
        {PITCH_C3, PITCH_C4, NOTE_WHOLE, "C Octave (C3-C4)"},
        {PITCH_F3, PITCH_F4, NOTE_WHOLE, "F Octave (F3-F4)"},
        {PITCH_G3, PITCH_G4, NOTE_WHOLE, "G Octave (G3-G4)"},
        {PITCH_C3, PITCH_C4, NOTE_WHOLE, "C Octave (C3-C4)"},

        // End marker
        {SILENCE, SILENCE, 0, "End"}};

    int i = 0;
    while (harmony_progression[i].duration > 0)
    {
        if (user_interrupt)
        {
            printf("\nUser interrupt detected.\n");
            audio_stop();
            return;
        }

        printf("  %s\n", harmony_progression[i].chord_name);
        audio_play_sound_blocking(
            harmony_progression[i].left_freq,
            harmony_progression[i].right_freq,
            harmony_progression[i].duration);
        sleep_ms(200); // Pause between chords
        i++;
    }

    printf("\nStereo harmony demo complete!\n");
}


void audiotest()
{
    printf("Comprehensive Audio Driver Test\n");

    printf("\n1. Playing musical scale (C4 to C5):\n");

    // Play a simple C major scale
    uint16_t scale[] = {
        PITCH_C4, PITCH_D4, PITCH_E4, PITCH_F4,
        PITCH_G4, PITCH_A4, PITCH_B4, PITCH_C5};

    const char *note_names[] = {
        "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5"};

    for (int i = 0; i < 8; i++)
    {
        if (user_interrupt)
        {
            printf("\nUser interrupt detected.\nStopping audio test.\n");
            return;
        }

        printf("Playing %s (%d Hz)...\n", note_names[i], scale[i]);
        audio_play_sound_blocking(scale[i], scale[i], NOTE_HALF);
        sleep_ms(100); // Small gap between notes
    }

    printf("\n2. Testing stereo channel separation:\n");

    // Test left channel only
    printf("Left channel only (C4 - 262 Hz)...\n");
    audio_play_sound_blocking(PITCH_C4, SILENCE, NOTE_WHOLE);
    if (user_interrupt)
        return;
    sleep_ms(200);

    // Test right channel only
    printf("Right channel only (E4 - 330 Hz)...\n");
    audio_play_sound_blocking(SILENCE, PITCH_E4, NOTE_WHOLE);
    if (user_interrupt)
        return;
    sleep_ms(200);

    // Test both channels with different frequencies
    printf("Both channels (Left: G4, Right: C5)...\n");
    audio_play_sound_blocking(PITCH_G4, PITCH_C5, NOTE_WHOLE);
    if (user_interrupt)
        return;
    sleep_ms(200);

    printf("\n3. Harmony Test:\n");
    printf("Playing musical intervals...\n");
    
    // Musical intervals for stereo harmony
    struct
    {
        uint16_t left;
        uint16_t right;
        const char *interval;
        const char *description;
    } harmonies[] = {
        {PITCH_C4, PITCH_C4, "Unison", "Same note both channels"},
        {PITCH_C4, PITCH_E4, "Major 3rd", "C4 + E4"},
        {PITCH_C4, PITCH_G4, "Perfect 5th", "C4 + G4"},
        {PITCH_C4, PITCH_C5, "Octave", "C4 + C5"},
        {PITCH_F4, PITCH_A4, "Major 3rd", "F4 + A4"},
        {PITCH_G4, PITCH_D5, "Perfect 5th", "G4 + D5"},
        {PITCH_A3, PITCH_CS4, "Major 3rd", "A3 + C#4"},
        {PITCH_E4, PITCH_B4, "Perfect 5th", "E4 + B4"}};

    for (int i = 0; i < 8; i++)
    {
        if (user_interrupt)
        {
            printf("\nUser interrupt detected.\n");
            return;
        }

        printf("  %s: %s\n", harmonies[i].interval, harmonies[i].description);
        audio_play_sound_blocking(harmonies[i].left, harmonies[i].right, NOTE_HALF);
        sleep_ms(100);
    }

    printf("\n4. Beat Frequency Test:\n");
    printf("Creating beat effects with detuned\nfrequencies...\n");

    // Beat frequency tests - slightly detuned frequencies create beating effects
    struct
    {
        uint16_t left;
        uint16_t right;
        const char *description;
    } beats[] = {
        {440, 442, "A4 vs A4+2Hz (slow beat)"},
        {440, 444, "A4 vs A4+4Hz (medium beat)"},
        {440, 448, "A4 vs A4+8Hz (fast beat)"},
        {523, 527, "C5 vs C5+4Hz (medium beat)"}};

    for (int i = 0; i < 4; i++)
    {
        if (user_interrupt)
        {
            printf("\nUser interrupt detected.\n");
            return;
        }

        printf("  %s\n", beats[i].description);
        audio_play_sound_blocking(beats[i].left, beats[i].right, NOTE_WHOLE + NOTE_HALF);
        sleep_ms(300);
    }

    printf("\n5. Stereo Sweep Test:\n");
    printf("Frequency sweep in stereo...\n");

    // Sweep both channels with different patterns
    printf("  Parallel sweep (both channels rising)\n");
    for (int freq = 200; freq <= 1000; freq += 100)
    {
        if (user_interrupt)
        {
            printf("\nUser interrupt detected.\n");
            return;
        }

        audio_play_sound_blocking(freq, freq, NOTE_EIGHTH);
        sleep_ms(25);
    }

    printf("  Counter sweep (left up, right down)\n");
    for (int i = 0; i < 9; i++)
    {
        if (user_interrupt)
        {
            printf("\nUser interrupt detected.\n");
            return;
        }

        uint16_t left_freq = 200 + (i * 100);
        uint16_t right_freq = 1000 - (i * 100);
        audio_play_sound_blocking(left_freq, right_freq, NOTE_EIGHTH);
        sleep_ms(25);
    }

    // === FREQUENCY RANGE TESTS ===
    printf("\n6. Testing frequency range (stereo):\n");

    // Test low to high frequency sweep in stereo
    uint16_t test_freqs[] = {
        LOW_BEEP, PITCH_C3, PITCH_C4, PITCH_C5, PITCH_C6, HIGH_BEEP};

    const char *freq_names[] = {
        "Low Beep (100 Hz)", "C3 (131 Hz)", "C4 (262 Hz)",
        "C5 (523 Hz)", "C6 (1047 Hz)", "High Beep (2000 Hz)"};

    for (int i = 0; i < 6; i++)
    {
        if (user_interrupt)
        {
            printf("\nUser interrupt detected.\nStopping audio test.\n");
            return;
        }

        printf("Playing %s (stereo)...\n", freq_names[i]);
        audio_play_sound_blocking(test_freqs[i], test_freqs[i], NOTE_QUARTER);
        sleep_ms(300);
    }

    // Test async stereo playback
    printf("\n7. Testing async stereo playback:\n");
    printf("Playing continuous stereo harmony\n");
    printf("for 3 seconds (C4 left, E4 right):\n");

    audio_play_sound(PITCH_C4, PITCH_E4);

    for (int i = 3; i > 0; i--)
    {
        if (user_interrupt)
        {
            printf("\nUser interrupt detected.\nStopping audio test.\n");
            break;
        }
        printf("%d...\n", i);
        sleep_ms(1000);
    }

    audio_stop();
    printf("Audio stopped.\n");

    printf("\n8. Stereo Phase Test:\n");
    printf("Playing identical frequencies to test\nphase alignment...\n");

    // Test phase alignment with identical frequencies
    uint16_t test_tones[] = {PITCH_A3, PITCH_A4, PITCH_A5};
    const char *PITCH_names[] = {"A3 (220 Hz)", "A4 (440 Hz)", "A5 (880 Hz)"};

    for (int i = 0; i < 3; i++)
    {
        if (user_interrupt)
        {
            printf("\nUser interrupt detected.\n");
            return;
        }

        printf("  %s on both channels...\n", PITCH_names[i]);
        audio_play_sound_blocking(test_tones[i], test_tones[i], NOTE_WHOLE);
        sleep_ms(200);
    }

    printf("\nDemo 1: Stereo Melody\n");
    play_stereo_melody_demo();

    if (user_interrupt)
    {
        printf("Demo interrupted.\n");
        return;
    }

    printf("\nDemo 2: Stereo Harmony\n");
    play_stereo_harmony_demo();

    if (user_interrupt)
    {
        printf("Demo interrupted.\n");
        return;
    }

    printf("\nComprehensive audio test complete!\n");
    printf("Your stereo audio system is working\n");
    printf("properly if you heard distinct\n");
    printf("left/right separation, melodies\n");
    printf("bouncing between channels, and\n");
    printf("harmonious intervals.\n\n");
    printf("Press BREAK key anytime during audio\nplayback to interrupt.\n");
}


void displaytest()
{
    int row = 1;
    printf("\033[?25l"); // Hide cursor

    absolute_time_t start_time = get_absolute_time();

    while (!user_interrupt && row <= 2000)
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
    printf("Character stress test:\n\n");
    printf("\033(0");  // Select DEC Special Character Set for G0
    printf("lqqqk\n"); // Top border: ┌───┐
    printf("x   x\n"); // Sides:      │   │
    printf("mqqqj\n"); // Bottom:     └───┘

    char buffer[32];
    uint output_chars = 0;
    start_time = get_absolute_time(); // Reset the start time for the next display
    while (!user_interrupt && chars < 60000)
    {
        int colour = 16 + (chars % 215);

        sprintf(buffer, "\033[4;3H\033[38;5;%dm%c", colour, 'A' + (chars % 26));
        output_chars += strlen(buffer);
        printf("%s", buffer);
        chars++;
    }
    end_time = get_absolute_time();
    uint64_t cps_elapsed_us = absolute_time_diff_us(start_time, end_time);
    float cps_elapsed_seconds = cps_elapsed_us / 1000000.0;
    float chars_per_second = output_chars / cps_elapsed_seconds;

    printf("\n\n\n\033(B\033[m\033[?25h");
    printf("Display stress test complete.\n");
    printf("\nRows processed: %d\n", row - 1);
    printf("Rows time elapsed: %.2f seconds\n", scrolling_elapsed_seconds);
    printf("Average rows per second: %.2f\n", rows_per_second);
    printf("\nCharacters processed: %d\n", output_chars);
    printf("Characters time elapsed: %.2f seconds\n", cps_elapsed_seconds);
    printf("Average characters per second: %.0f\n", chars_per_second);
}


// Song table for easy access
const test_t tests[] = {
    {"audio", audiotest, "Audio Driver Test"},
    {"display", displaytest, "Display Driver Test"},
    {NULL, NULL, NULL} // End marker
};


// Function to find a song by name
const test_t *find_test(const char *test_name)
{
    for (int i = 0; tests[i].name != NULL; i++)
    {
        if (strcmp(tests[i].name, test_name) == 0)
        {
            return &tests[i];
        }
    }
    return NULL; // Song not found
}

// Function to list all available songs
void show_test_library(void)
{
    printf("Test Library:\n\n");

    for (int i = 0; tests[i].name != NULL; i++)
    {
        printf("  %s - %s\n", tests[i].name, tests[i].description);
    }
}
