#pragma once

#include "pico/stdlib.h"

// Channels
#define LEFT_CHANNEL (0)
#define RIGHT_CHANNEL (1)

// GPIO pins
#define AUDIO_LEFT_PIN (26)
#define AUDIO_RIGHT_PIN (27)


// TONES and their frequencies (in Hz)

// Octave 3 (Low notes)
#define TONE_C3  (131)
#define TONE_CS3 (139)
#define TONE_D3  (147)
#define TONE_DS3 (156)
#define TONE_E3  (165)
#define TONE_F3  (175)
#define TONE_FS3 (185)
#define TONE_G3  (196)
#define TONE_GS3 (208)
#define TONE_A3  (220)
#define TONE_AS3 (233)
#define TONE_B3  (247)

// Octave 4 (Middle notes)
#define TONE_C4  (262)
#define TONE_CS4 (277)
#define TONE_D4  (294)
#define TONE_DS4 (311)
#define TONE_E4  (330)
#define TONE_F4  (349)
#define TONE_FS4 (370)
#define TONE_G4  (392)
#define TONE_GS4 (415)
#define TONE_A4  (440)  // A440 - Concert pitch
#define TONE_AS4 (466)
#define TONE_B4  (494)

// Octave 5 (High notes)
#define TONE_C5  (523)
#define TONE_CS5 (554)
#define TONE_D5  (587)
#define TONE_DS5 (622)
#define TONE_E5  (659)
#define TONE_F5  (698)
#define TONE_FS5 (740)
#define TONE_G5  (784)
#define TONE_GS5 (831)
#define TONE_A5  (880)
#define TONE_AS5 (932)
#define TONE_B5  (988)

// Octave 6 (Very high notes)
#define TONE_C6  (1047)
#define TONE_CS6 (1109)
#define TONE_D6  (1175)
#define TONE_DS6 (1245)
#define TONE_E6  (1319)
#define TONE_F6  (1397)
#define TONE_FS6 (1480)
#define TONE_G6  (1568)
#define TONE_GS6 (1661)
#define TONE_A6  (1760)
#define TONE_AS6 (1865)
#define TONE_B6  (1976)

// Special tones
#define TONE_SILENCE (0)
#define TONE_LOW_BEEP (100)
#define TONE_HIGH_BEEP (2000)

// Common chord frequencies (for convenience)
#define CHORD_C_MAJOR TONE_C4, TONE_E4, TONE_G4
#define CHORD_G_MAJOR TONE_G4, TONE_B4, TONE_D5
#define CHORD_F_MAJOR TONE_F4, TONE_A4, TONE_C5

// Note lengths in milliseconds
#define NOTE_WHOLE     (2000)    // Whole note - 2 seconds
#define NOTE_HALF      (1000)    // Half note - 1 second
#define NOTE_QUARTER   (500)     // Quarter note - 0.5 seconds
#define NOTE_EIGHTH    (250)     // Eighth note - 0.25 seconds
#define NOTE_SIXTEENTH (125)     // Sixteenth note - 0.125 seconds
#define NOTE_THIRTYSECOND (62)   // Thirty-second note - 0.062 seconds

// Common note length variations
#define NOTE_DOTTED_HALF      (1500)    // Dotted half note (1.5x half note)
#define NOTE_DOTTED_QUARTER   (750)     // Dotted quarter note (1.5x quarter note)
#define NOTE_DOTTED_EIGHTH    (375)     // Dotted eighth note (1.5x eighth note)

// Tempo-based note lengths (120 BPM as default)
#define NOTE_WHOLE_120BPM     (2000)    // Whole note at 120 BPM
#define NOTE_HALF_120BPM      (1000)    // Half note at 120 BPM
#define NOTE_QUARTER_120BPM   (500)     // Quarter note at 120 BPM
#define NOTE_EIGHTH_120BPM    (250)     // Eighth note at 120 BPM

// Fast tempo note lengths (140 BPM)
#define NOTE_WHOLE_140BPM     (1714)    // Whole note at 140 BPM
#define NOTE_HALF_140BPM      (857)     // Half note at 140 BPM
#define NOTE_QUARTER_140BPM   (429)     // Quarter note at 140 BPM
#define NOTE_EIGHTH_140BPM    (214)     // Eighth note at 140 BPM

// Slow tempo note lengths (80 BPM)
#define NOTE_WHOLE_80BPM      (3000)    // Whole note at 80 BPM
#define NOTE_HALF_80BPM       (1500)    // Half note at 80 BPM
#define NOTE_QUARTER_80BPM    (750)     // Quarter note at 80 BPM
#define NOTE_EIGHTH_80BPM     (375)     // Eighth note at 80 BPM

typedef struct {
    uint16_t frequency;  // Frequency in Hz
    uint32_t duration_ms; // Duration in milliseconds
} audio_note_t;


// Audio driver function prototypes
void audio_init(void);
void audio_play_tone(uint16_t frequency, uint32_t duration_ms);
void audio_play_tone_async(uint16_t frequency);
void audio_stop(void);
void audio_set_volume(uint8_t volume);  // Volume: 0-100
bool audio_is_playing(void);

