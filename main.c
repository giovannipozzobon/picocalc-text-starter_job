#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "drivers/picocalc.h"
#include "drivers/display.h"
#include "drivers/keyboard.h"
#include "drivers/onboard_led.h"
#include "drivers/lcd.h"
#include "drivers/ds3231.h"

#include "commands.h"
#include "wifi.h"
#include "tiles.h"

bool power_off_requested = false;

// Command history
#define HISTORY_SIZE 10
#define HISTORY_BUFFER_SIZE 40
static char history[HISTORY_SIZE][HISTORY_BUFFER_SIZE];
static int history_count = 0;
static int history_index = 0;

void set_onboard_led(uint8_t led)
{
    led_set(led & 0x01);
}

#include <ctype.h>

void str_to_lower(char *s) {
    while (*s) {
        *s = tolower((unsigned char)*s);
        s++;
    }
}

void readline(char *buffer, size_t size)
{
    size_t index = 0;
    int history_pos = -1; // -1 significa "nessuna navigazione history"
    char temp_buffer[HISTORY_BUFFER_SIZE] = {0}; // Buffer temporaneo per il comando corrente

    while (true)
    {
        char ch = getchar();
        if (ch == 0x04) // Ctrl+D to debug
        {
            printf("Entering debug mode...\n");
            __breakpoint();
        }
        else if (ch == '\n' || ch == '\r')
        {
            printf("\n");
            break; // End of line
        }
        else if (ch == KEY_UP) // Freccia SU - naviga history verso l'alto
        {
            if (history_count == 0) continue;

            // Salva il comando corrente la prima volta
            if (history_pos == -1)
            {
                strncpy(temp_buffer, buffer, HISTORY_BUFFER_SIZE - 1);
                history_pos = history_count;
            }

            if (history_pos > 0)
            {
                history_pos--;

                // Cancella la riga corrente
                while (index > 0)
                {
                    printf("\b \b");
                    index--;
                }

                // Copia e mostra il comando dalla history
                strncpy(buffer, history[history_pos], size - 1);
                buffer[size - 1] = '\0';
                index = strlen(buffer);
                printf("%s", buffer);
            }
        }
        else if (ch == KEY_DOWN) // Freccia GIÙ - naviga history verso il basso
        {
            if (history_pos == -1) continue;

            // Cancella la riga corrente
            while (index > 0)
            {
                printf("\b \b");
                index--;
            }

            if (history_pos < history_count - 1)
            {
                history_pos++;
                strncpy(buffer, history[history_pos], size - 1);
            }
            else
            {
                // Torna al comando che stava scrivendo
                history_pos = -1;
                strncpy(buffer, temp_buffer, size - 1);
            }

            buffer[size - 1] = '\0';
            index = strlen(buffer);
            printf("%s", buffer);
        }
        else if ((ch == 0x08 || ch == 0x7F) && index > 0) // Backspace or Delete
        {
            index--;
            buffer[index] = '\0'; // Remove last character
            printf("\b \b"); // Erase the last character
        }
        else if (ch >= 0x20 && ch < 0x7F && index < size - 1) // Printable characters
        {
            buffer[index++] = ch;
            putchar(ch);
        }
    }
    buffer[index] = '\0'; // Null-terminate the string

    // Aggiungi il comando alla history se non è vuoto e diverso dall'ultimo
    if (index > 0 && (history_count == 0 || strcmp(buffer, history[history_count - 1]) != 0))
    {
        // Se la history è piena, sposta tutti gli elementi indietro
        if (history_count >= HISTORY_SIZE)
        {
            for (int i = 0; i < HISTORY_SIZE - 1; i++)
            {
                strncpy(history[i], history[i + 1], HISTORY_BUFFER_SIZE - 1);
            }
            history_count = HISTORY_SIZE - 1;
        }

        // Aggiungi il nuovo comando
        strncpy(history[history_count], buffer, HISTORY_BUFFER_SIZE - 1);
        history[history_count][HISTORY_BUFFER_SIZE - 1] = '\0';
        history_count++;
    }
}

int main()
{
    char buffer[40];

    // Initialize the LED driver and set the LED callback
    // If the LED driver fails to initialize, we can still run the text starter
    // without LED support, so we pass NULL to picocalc_init.
    int led_init_result = led_init();

    stdio_init_all();
    picocalc_init();
    if (led_init_result == 0) {
        display_set_led_callback(set_onboard_led);
    }

    printf("\033c\033[1m\n Hello from the PicoCalc Text Starter!\033[0m\n\n");
    printf("      Contributed to the community\n");
    printf("            by Blair Leduc.\n\n");
    printf("Type \033[4mhelp\033[0m for a list of commands.\n\n");

    // Inizializza il DS3231 RTC
    if (ds3231_init()) {
        printf("DS3231 RTC ready on I2C1 (GP6/GP7)\n\n");
    } else {
        printf("Warning: DS3231 RTC not detected\n\n");
    }

    //JOBOND 
    //test_wifi();

    // A very simple REPL
    printf("\033[qReady.\n");
    while (true)
    {
        readline(buffer, sizeof(buffer));
        if (strlen(buffer) == 0)
        {
            continue; // Skip empty input
        }

        printf("\033[1q\n"); // Turn on the LED so the user knows input is being processed

        // Convert the input to lowercase for case-insensitive command matching
        str_to_lower(buffer);
        
        run_command(buffer); // Call the command handler

        printf("\033[q\nReady. %s\n", power_off_requested ? "(power off requested)" : ""); // Turn off the LED and prompt for input again
        power_off_requested = false;
    }
}
