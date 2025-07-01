//
//  PicoCalc Songs Library
//
//  This file contains popular songs arranged for the PicoCalc audio driver.
//  Songs are stored as arrays of notes with frequencies and durations.
//

#include <stdio.h>
#include <string.h>
#include "pico/time.h"
#include "drivers/audio.h"
#include "songs.h"

// "Mary Had a Little Lamb" - Simple test song
const audio_note_t song_mary_lamb[] = {
    {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER},
    {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_HALF},
    {TONE_D4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER}, {TONE_D4, NOTE_HALF},
    {TONE_E4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER}, {TONE_G4, NOTE_HALF},
    
    {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER},
    {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER},
    {TONE_D4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER},
    {TONE_C4, NOTE_WHOLE},
    
    // End marker
    {TONE_SILENCE, 0}
};

// "Happy Birthday" - Classic celebration song
const audio_note_t song_happy_birthday[] = {
    {TONE_C4, NOTE_DOTTED_EIGHTH}, {TONE_C4, NOTE_SIXTEENTH}, {TONE_D4, NOTE_QUARTER},
    {TONE_C4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER}, {TONE_E4, NOTE_HALF},
    
    {TONE_C4, NOTE_DOTTED_EIGHTH}, {TONE_C4, NOTE_SIXTEENTH}, {TONE_D4, NOTE_QUARTER},
    {TONE_C4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER}, {TONE_F4, NOTE_HALF},
    
    {TONE_C4, NOTE_DOTTED_EIGHTH}, {TONE_C4, NOTE_SIXTEENTH}, {TONE_C5, NOTE_QUARTER},
    {TONE_A4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER},
    
    {TONE_AS4, NOTE_DOTTED_EIGHTH}, {TONE_AS4, NOTE_SIXTEENTH}, {TONE_A4, NOTE_QUARTER},
    {TONE_F4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER}, {TONE_F4, NOTE_HALF},
    
    // End marker
    {TONE_SILENCE, 0}
};

// "Twinkle Twinkle Little Star" - Simple children's song
const audio_note_t song_twinkle[] = {
    {TONE_C4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER},
    {TONE_A4, NOTE_QUARTER}, {TONE_A4, NOTE_QUARTER}, {TONE_G4, NOTE_HALF},
    
    {TONE_F4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER},
    {TONE_D4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER}, {TONE_C4, NOTE_HALF},
    
    {TONE_G4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER},
    {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_HALF},
    
    {TONE_G4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER},
    {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_HALF},
    
    {TONE_C4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER},
    {TONE_A4, NOTE_QUARTER}, {TONE_A4, NOTE_QUARTER}, {TONE_G4, NOTE_HALF},
    
    {TONE_F4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER},
    {TONE_D4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER}, {TONE_C4, NOTE_HALF},
    
    // End marker
    {TONE_SILENCE, 0}
};

// "Baa Baa Black Sheep" - Traditional nursery rhyme
const audio_note_t song_baa_baa[] = {
    // "Baa baa black sheep, have you any wool?"
    {TONE_C4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER},
    {TONE_A4, NOTE_QUARTER}, {TONE_A4, NOTE_QUARTER}, {TONE_G4, NOTE_HALF},
    
    // "Yes sir, yes sir, three bags full"
    {TONE_F4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER},
    {TONE_D4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER}, {TONE_C4, NOTE_HALF},
    
    // "One for the master, one for the dame"
    {TONE_G4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER},
    {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_HALF},
    
    // "And one for the little boy who lives down the lane"
    {TONE_G4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER},
    {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_HALF},
    
    // "Baa baa black sheep, have you any wool?" (repeat)
    {TONE_C4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER},
    {TONE_A4, NOTE_QUARTER}, {TONE_A4, NOTE_QUARTER}, {TONE_G4, NOTE_HALF},
    
    // "Yes sir, yes sir, three bags full" (repeat)
    {TONE_F4, NOTE_QUARTER}, {TONE_F4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER},
    {TONE_D4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER}, {TONE_C4, NOTE_HALF},
    
    // End marker
    {TONE_SILENCE, 0}
};

// "Old MacDonald Had a Farm" - Popular children's song
const audio_note_t song_old_macdonald[] = {
    // "Old MacDonald had a farm, E-I-E-I-O"
    {TONE_C4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_G3, NOTE_QUARTER},
    {TONE_A3, NOTE_QUARTER}, {TONE_A3, NOTE_QUARTER}, {TONE_G3, NOTE_HALF},
    {TONE_G3, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER},
    {TONE_D4, NOTE_QUARTER}, {TONE_C4, NOTE_HALF},
    
    // "And on his farm he had a cow, E-I-E-I-O"
    {TONE_G3, NOTE_QUARTER}, {TONE_G3, NOTE_QUARTER}, {TONE_G3, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER},
    {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_HALF},
    {TONE_G3, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER},
    {TONE_D4, NOTE_QUARTER}, {TONE_C4, NOTE_HALF},
    
    // "With a moo-moo here, and a moo-moo there"
    {TONE_C4, NOTE_EIGHTH}, {TONE_G3, NOTE_EIGHTH}, {TONE_C4, NOTE_EIGHTH}, {TONE_G3, NOTE_EIGHTH},
    {TONE_C4, NOTE_QUARTER}, {TONE_C4, NOTE_EIGHTH}, {TONE_G3, NOTE_EIGHTH}, {TONE_C4, NOTE_EIGHTH}, {TONE_G3, NOTE_EIGHTH},
    {TONE_C4, NOTE_QUARTER},
    
    // "Here a moo, there a moo, everywhere a moo-moo"
    {TONE_C4, NOTE_EIGHTH}, {TONE_G3, NOTE_EIGHTH}, {TONE_C4, NOTE_QUARTER},
    {TONE_C4, NOTE_EIGHTH}, {TONE_G3, NOTE_EIGHTH}, {TONE_C4, NOTE_QUARTER},
    {TONE_G3, NOTE_QUARTER}, {TONE_C4, NOTE_EIGHTH}, {TONE_G3, NOTE_EIGHTH}, {TONE_C4, NOTE_QUARTER},
    
    // "Old MacDonald had a farm, E-I-E-I-O"
    {TONE_C4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_G3, NOTE_QUARTER},
    {TONE_A3, NOTE_QUARTER}, {TONE_A3, NOTE_QUARTER}, {TONE_G3, NOTE_HALF},
    {TONE_G3, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER},
    {TONE_D4, NOTE_QUARTER}, {TONE_C4, NOTE_WHOLE},
    
    // End marker
    {TONE_SILENCE, 0}
};

// "Itsy Bitsy Spider" - Classic children's nursery rhyme
const audio_note_t song_itsy_bitsy_spider[] = {
    // "The itsy bitsy spider climbed up the water spout"
    {TONE_G4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER},
    {TONE_D4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER},
    {TONE_C4, NOTE_HALF}, {TONE_SILENCE, NOTE_QUARTER},
    
    // "Down came the rain and washed the spider out"
    {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER},
    {TONE_D4, NOTE_QUARTER}, {TONE_G3, NOTE_QUARTER}, {TONE_G3, NOTE_QUARTER}, {TONE_G3, NOTE_QUARTER},
    {TONE_C4, NOTE_HALF}, {TONE_SILENCE, NOTE_QUARTER},
    
    // "Out came the sun and dried up all the rain"
    {TONE_G4, NOTE_QUARTER}, {TONE_G4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER},
    {TONE_D4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER},
    {TONE_D4, NOTE_HALF}, {TONE_SILENCE, NOTE_QUARTER},
    
    // "And the itsy bitsy spider climbed up the spout again"
    {TONE_G4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER}, {TONE_C4, NOTE_QUARTER},
    {TONE_D4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_E4, NOTE_QUARTER}, {TONE_D4, NOTE_QUARTER},
    {TONE_C4, NOTE_QUARTER}, {TONE_G3, NOTE_QUARTER}, {TONE_C4, NOTE_WHOLE},
    
    // End marker
    {TONE_SILENCE, 0}
};

// Song table for easy access
const song_info_t song_list[] = {
    {"mary", song_mary_lamb, "Mary Had a Little Lamb"},
    {"birthday", song_happy_birthday, "Happy Birthday"},
    {"twinkle", song_twinkle, "Twinkle Twinkle Little Star"},
    {"baa", song_baa_baa, "Baa Baa Black Sheep"},
    {"macdonald", song_old_macdonald, "Old MacDonald Had a Farm"},
    {"spider", song_itsy_bitsy_spider, "Itsy Bitsy Spider"},
    {NULL, NULL, NULL} // End marker
};

// Function to play a song from the song array
void play_song(const audio_note_t *song)
{
    if (!song) return;
    
    int note_index = 0;
    while (song[note_index].duration_ms != 0) {
        audio_play_tone(song[note_index].frequency, song[note_index].duration_ms);
        
        // Small gap between notes for clarity (except for silence notes)
        if (song[note_index].frequency != TONE_SILENCE) {
            sleep_ms(20);
        }
        
        note_index++;
        
        // Check for user interrupt (BREAK key)
        extern volatile bool user_interrupt;
        if (user_interrupt) {
            audio_stop();
            break;
        }
    }
    
    audio_stop(); // Ensure audio is stopped at the end
}

// Function to find a song by name
const audio_note_t* find_song(const char* song_name)
{
    for (int i = 0; song_list[i].name != NULL; i++) {
        if (strcmp(song_list[i].name, song_name) == 0) {
            return song_list[i].song;
        }
    }
    return NULL; // Song not found
}

// Function to list all available songs
void list_songs(void)
{
    printf("Available songs:\n\n");
    for (int i = 0; song_list[i].name != NULL; i++) {
        printf("  %s - %s\n", song_list[i].name, song_list[i].description);
    }
}
