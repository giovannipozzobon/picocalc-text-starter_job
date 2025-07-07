//
//  LED Driver for the Raspberry Pi Pico
//
//  This driver controls the onboard LED on the Raspberry Pi Pico and
//  can also control the LED on the CYW43 Wi-Fi chip if needed.
//

#include "pico/stdlib.h"

#ifndef PICO_DEFAULT_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#include "onboard_led.h"

static bool led_initialised = false; // Flag to indicate if the LED is initialized

// Set the state of the on-board LED
void led_set(bool led)
{
#ifdef PICO_DEFAULT_LED_PIN
    gpio_put(PICO_DEFAULT_LED_PIN, led ? 1 : 0);
#else
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led ? 1 : 0);
#endif
}

// Initialize the LED driver
int led_init()
{
    if (led_initialised) {
        return 0; // Already initialized, return success
    }

#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return 0; // Return 0 to indicate success
#else
    // If you have a wifi driver, call the initialiser function instead

    // This will initialize the CYW43 Wi-Fi chip and its LED
    return cyw43_arch_init();
#endif

    led_initialised = true; // Set the initialized flag
}

