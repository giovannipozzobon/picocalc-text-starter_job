//
//  PicoCalc Audio Driver
//
//  This driver implements a simple mono PWM-based audio output for the PicoCalc.
//  Since the two GPIO pins (26 & 27) are on the same PWM slice, they must operate
//  at the same frequency. This driver outputs the same tone on both channels
//  simultaneously for mono audio output.
//

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/time.h"

#include "audio.h"

static bool audio_initialised = false;
static uint pwm_slice_num;
static uint8_t current_volume = 50;  // Default volume (0-100)
static bool is_playing = false;
static alarm_id_t tone_alarm_id = -1;

// Forward declaration for the alarm callback
static int64_t tone_stop_callback(alarm_id_t id, void *user_data);

// Set volume (0-100)
void audio_set_volume(uint8_t volume)
{
    if (volume > 100) {
        volume = 100;
    }
    current_volume = volume;
}

// Calculate PWM parameters for a given frequency
static void set_pwm_frequency(uint16_t frequency)
{
    if (frequency == 0 || frequency == TONE_SILENCE) {
        // Silence - set duty cycle to 0
        pwm_set_gpio_level(AUDIO_LEFT_PIN, 0);
        pwm_set_gpio_level(AUDIO_RIGHT_PIN, 0);
        is_playing = false;
        return;
    }

    // Calculate clock divider and wrap value for the desired frequency
    uint32_t clock_freq = clock_get_hz(clk_sys);
    
    // We want: PWM_freq = clock_freq / (clkdiv * (wrap + 1))
    // Rearranging: clkdiv * (wrap + 1) = clock_freq / PWM_freq
    
    float clkdiv = 1.0f;
    uint32_t wrap = (clock_freq / frequency) - 1;
    
    // If wrap is too large for 16-bit, increase clock divider
    while (wrap > 65535) {
        clkdiv += 1.0f;
        wrap = (clock_freq / (frequency * clkdiv)) - 1;
    }
    
    // Configure PWM with new parameters
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clkdiv);
    pwm_config_set_wrap(&config, wrap);
    
    pwm_init(pwm_slice_num, &config, false);
    
    // Calculate duty cycle based on volume (50% max for square wave)
    uint32_t duty_cycle = (wrap * current_volume) / 200;  // Divide by 200 for 50% max
    
    // Set the same duty cycle for both channels (mono output)
    pwm_set_gpio_level(AUDIO_LEFT_PIN, duty_cycle);
    pwm_set_gpio_level(AUDIO_RIGHT_PIN, duty_cycle);
    
    // Enable PWM
    pwm_set_enabled(pwm_slice_num, true);
    is_playing = true;
}

// Play a tone asynchronously (continues until stopped)
void audio_play_tone_async(uint16_t frequency)
{
    if (!audio_initialised) {
        return;
    }

    // Cancel any existing tone alarm
    if (tone_alarm_id >= 0) {
        cancel_alarm(tone_alarm_id);
        tone_alarm_id = -1;
    }

    set_pwm_frequency(frequency);
}

// Play a tone for a specific duration (blocking)
void audio_play_tone(uint16_t frequency, uint32_t duration_ms)
{
    if (!audio_initialised) {
        return;
    }

    // Cancel any existing tone alarm
    if (tone_alarm_id >= 0) {
        cancel_alarm(tone_alarm_id);
        tone_alarm_id = -1;
    }

    set_pwm_frequency(frequency);
    
    if (frequency != 0 && frequency != TONE_SILENCE && duration_ms > 0) {
        // Set up alarm to stop the tone after duration
        tone_alarm_id = add_alarm_in_ms(duration_ms, tone_stop_callback, NULL, false);
        
        // Wait for the duration
        sleep_ms(duration_ms);
    }
}

// Stop audio output
void audio_stop(void)
{
    if (!audio_initialised) {
        return;
    }

    // Cancel any existing tone alarm
    if (tone_alarm_id >= 0) {
        cancel_alarm(tone_alarm_id);
        tone_alarm_id = -1;
    }

    // Set duty cycle to 0 for silence
    pwm_set_gpio_level(AUDIO_LEFT_PIN, 0);
    pwm_set_gpio_level(AUDIO_RIGHT_PIN, 0);
    
    // Disable PWM
    pwm_set_enabled(pwm_slice_num, false);
    is_playing = false;
}

// Check if audio is currently playing
bool audio_is_playing(void)
{
    return is_playing;
}

// Alarm callback function to stop tone
static int64_t tone_stop_callback(alarm_id_t id, void *user_data)
{
    audio_stop();
    tone_alarm_id = -1;
    
    return 0; // Don't repeat the alarm
}

// Initialize the audio driver
void audio_init(void)
{
    if (audio_initialised) {
        return; // Already initialized
    }

    // Initialize GPIO pins for PWM
    gpio_set_function(AUDIO_LEFT_PIN, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_RIGHT_PIN, GPIO_FUNC_PWM);

    // Get PWM slice number (both pins should be on the same slice)
    pwm_slice_num = pwm_gpio_to_slice_num(AUDIO_LEFT_PIN);
    
    // Configure PWM
    pwm_config config = pwm_get_default_config();
    
    // Set initial frequency to silence
    pwm_config_set_clkdiv(&config, 1.0f);
    pwm_config_set_wrap(&config, 65535);
    
    pwm_init(pwm_slice_num, &config, true);

    // Set initial duty cycle to 0 (silence)
    pwm_set_gpio_level(AUDIO_LEFT_PIN, 0);
    pwm_set_gpio_level(AUDIO_RIGHT_PIN, 0);

    audio_initialised = true;
}
