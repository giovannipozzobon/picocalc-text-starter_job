# Keyboard

The keyboard driver operates with a timer loop that polls the PicoCalc's southbridge for key presses. Unfortunately, the southbridge cannot notify the Pico when a key is pressed.

The purpose of this implementation was support:

- type ahead
- keyboard user interrupts

The type ahead buffer allows users to type even while your project is processing. When Brk (Shift-Esc) is pressed, a flag is set allowing your project to monitor and stop processing, if desired. 


## keyboard_init

`void keyboard_init(keyboard_key_available_callback_t key_available_callback)`

Initialises the keyboard.

### Parameters

key_available_callback - called when keys are available


## keyboard_key_available

`bool keyboard_key_available(void)`

Returns true is a key is available.


## keyboard_get_key

`char keyboard_get_key(void)`

Returns a key; blocks if no key is available.


