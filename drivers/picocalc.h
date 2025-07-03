#pragma once  

#include "pico/stdlib.h"

typedef void (*led_callback_t)(uint8_t);

// Function prototypes
void picocalc_init(led_callback_t led_callback);
