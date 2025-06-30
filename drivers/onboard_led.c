//
//  LED Driver for the Raspberry Pi Pico
//
//  This driver controls the onboard LED on the Raspberry Pi Pico and
//  can also control the LED on the CYW43 Wi-Fi chip if needed.
//

#ifndef PICO_DEFAULT_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#include "onboard_led.h"

// Initialize the LED driver
int led_init()
{
#ifdef PICO_DEFAULT_LED_PIN
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return 0; // Return 0 to indicate success
#else
    return cyw43_arch_init();
#endif
}

// Set the state of the LED
//
// The `led` parameter can be 0 to turn off all LEDs, or the number of the LED
// to light.
//
// Additional LEDs can be added by extending the function. If you have more
// LEDs, you can use 2, 3, etc. to control them.

void led_set(uint8_t led)
{
#ifdef PICO_DEFAULT_LED_PIN
    if (led == 0)
    {
        gpio_put(PICO_DEFAULT_LED_PIN, 0); // Turn off the LED
        // Turn off all the LEDs here
    }
    else if (led == 1)
    {
        gpio_put(PICO_DEFAULT_LED_PIN, 1); // Turn on the LED
    }
    // Add more LEDs here
#else
    if (led == 0)
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); // Turn off the LED
        // Turn off all the LEDs here
    }
    else if (led == 1)
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); // Turn on the LED
    }
    // Add more LEDs here
#endif
}