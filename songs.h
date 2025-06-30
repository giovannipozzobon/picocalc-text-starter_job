#pragma once

#include "drivers/audio.h"
#include <string.h>

// Structure to hold song information
typedef struct {
    const char* name;           // Short name for command reference
    const audio_note_t* song;   // Pointer to the song data
    const char* description;    // Full song title and artist
} song_info_t;

// Song function prototypes
void play_song(const audio_note_t *song);
const audio_note_t* find_song(const char* song_name);
void list_songs(void);

// External song arrays
extern const audio_note_t song_mary_lamb[];
extern const audio_note_t song_happy_birthday[];
extern const audio_note_t song_twinkle[];

// Song list
extern const song_info_t song_list[];
