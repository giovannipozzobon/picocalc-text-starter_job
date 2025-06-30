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
void clearscreen(void);
void speedtest(void);
void bye(void);
void box(void);
void battery(void);
void backlight(void);

