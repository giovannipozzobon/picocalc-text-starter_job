# PicoCalc

This pseudo driver configures the southbridge, display and keyboard drivers. The display and keyboard are connected to the  C stdio <stdio.h> library (printf, scanf, getchar, putchar, ...).

## picocalc_init

`void picocalc_init(led_callback_t led_callback)`

Initialise the southbridge, display and keyboard. Connects the C stdio functions to the display and keyboard.

### Parameters

- led_callback – notifies when an LED state was modified by the display driver

