#pragma once

// Command structure for function pointer table
typedef struct {
    const char* name;
    void (*function)(void);
    const char* description;
} command_t;

// Forward declarations
void backlight(void);
void battery(void);
void beep(void);
void box(void);
void bye(void);
void clearscreen(void);
void play(void);
void run_command(const char *command);
void show_command_library(void);
void test(void);

// SD card commands
void sd_dir(void);
void sd_free(void);
void sd_more(void);
void sd_read_filename(const char *filename);
void sd_status(void);

