#pragma once

// Command structure for function pointer table
typedef struct {
    const char* name;
    void (*function)(void);
    const char* description;
} command_t;

// Forward declarations
void run_command(const char *command);
void help(void);
void backlight(void);
void battery(void);
void beep(void);
void box(void);
void bye(void);
void clearscreen(void);
void play(void);
void test(void);

void play_stereo_harmony_demo(void);
void play_stereo_melody_demo(void);

