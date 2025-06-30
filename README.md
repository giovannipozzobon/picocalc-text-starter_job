# picocalc-text-starter

With this starter project, you will be able to get started on the PicoCalc using the Pico-Series C/C++ SDK. You can create the [text-based user interface](https://en.wikipedia.org/wiki/Text-based_user_interface) experience of the 1980's that are well suited for a mouseless system.

This starter contains code to write text to the LCD display and read input from the keyboard using the C stdio <stdio.h> library (printf, scanf, getchar, putchar, ...).

Other drivers are provided for:

- Onboard LED
- Serial port
- Southbridge functions (battery, backlights)

*This starter is not intended for getting started with only a Raspberry Pi Pico as you should already have, or will need to gain, knowledge with the Pico-series C/C++ SDK to use this starter project.*

This starter provides source code for accessing the peripherals of the PicoCalc device so you can concentrate on creating your project.

> This is a starter project. Feel free to take bits and pieces and modify what is here to suit YOUR project.

# High-Level Drivers

## PicoCalc

This pseudo driver configures the southbridge, display and keyboard drivers. The display and keyboard are connected to the  C stdio <stdio.h> library (printf, scanf, getchar, putchar, ...).

### picocalc_init

`void picocalc_init(led_callback_t led_callback)`

Initialise the southbridge, display and keyboard. Connects the C stdio functions to the display and keyboard.

#### Parameters

- led_callback – notifies when an LED state was modified by the display driver


## Display

The display driver emulates an ANSI terminal.  

This driver uses very little RAM leaving more for your project as a frame buffer in the Pico RAM is not used.

Colours default to plain ASCII (black and phosphor), or you can use ANSI 16-colour, 256-colour palette (216 colors + 16 ANSI + 24 gray) and 24-bit truecolor (approximate to 65K colours). You can chose a between white, green or amber presets as your default phosphor colour, or chose your own.

The UK and [Special Graphics](https://vt100.net/docs/vt100-ug/chapter3.html#T3-9) character sets of the VT100 are supported.

The font (8x10) is easily modifyable in source with out any additional tooling. You draw the glyphs using 1's and 0's:

``` C
    // 0x41
    0b00010000,
    0b00101000,
    0b01000100,
    0b10000010,
    0b11111110,
    0b10000010,
    0b10000010,
    0b00000000,
    0b00000000,
    0b00000000,
```

### display_init

`void display_init(led_callback_t led_callback, bell_callback_t bell_callback)`

Initialises the LCD display.

#### Parameters

led_callback – called when the state of the LEDs changes
bell_callback – called when the BEL character is received

### display_emit_available

`bool display_emit_available(void)`

Returns true if the display can accept characters.

### display_emit

`void display_emit(char c)`

Display a character on the display or processes the ANSI escape sequence.

#### Parameters

c – the character to process


## Keyboard

The keyboard driver operates with a timer loop that polls the PicoCalc's southbridge for key presses. Unfortunately, the southbridge cannot notify the Pico when a key is pressed.

The purpose of this implementation was support:

- type ahead
- keyboard user interrupts

The type ahead buffer allows users to type even while your project is processing. When Brk (Shift-Esc) is pressed, a flag is set allowing your project to monitor and stop processing, if desired. 


### keyboard_init

`void keyboard_init(keyboard_key_available_callback_t key_available_callback)`

Initialises the keyboard.

#### Parameters

key_available_callback - called when keys are available


### keyboard_key_available

`bool keyboard_key_available(void)`

Returns true is a key is available.


### keyboard_get_key

`char keyboard_get_key(void)`

Returns a key; blocks if no key is available.



# Low-Level Drivers

## LCD

The driver for the LCD display is optimised for displaying text. For performance, the display is operated in 65K colour depth (RGB565) and any colour from this colour depth can be chosen and used. 

This driver uses very little RAM leaving more for your project as a frame buffer in the Pico RAM is not used.

### lcd_init

`void lcd_init(void)`

Initialise the LCD controller.


### lcd_display_on

`void lcd_display_on(void)`

Turn the display on.


### lcd_display_off

`void lcd_display_off(void)`

Turn the display off.


### lcd_blit

`void lcd_blit(uint16_t *pixels, uint16_t x, uint16_t y, uint16_t width, uint16_t height)`

Writes pixel data to a region of the frame buffer in the display controller and takes into account the scrolled display.

#### Parameters

- pixels – array of pixels (RGB565)
- x – left edge corner of the region in pixels
- y – top edge of the region in pixels
- width – width of the region in pixels
- height - height of the region in pixels


### lcd_solid_rectangle

`void lcd_solid_rectangle(uint16_t colour, uint16_t x, uint16_t y, uint16_t width, uint16_t height)`

Draws a solid rectangle using a single colour.

#### Parameters

- colour – the RGB565 colour
- x – left edge corner of the rectangle in pixels
- y – top edge of the rectangle in pixels
- width – width of the rectangle in pixels
- height - height of the rectangle in pixels


### lcd_define_scrolling

`void lcd_define_scrolling(uint16_t top_fixed_area, uint16_t bottom_fixed_area)`

Define the area that will be scrolled on the display. The scrollable area is between the top fixed area and the bottom fixed area.

#### Parameters

- top_fixed_area – Number of pixel rows fixed at the top of the display
- bottom_fixed_area – Number of pixel rows fixed at the bottom of the display


### lcd_scroll_up

`void lcd_scroll_up(void)`

Scroll the screen up one line adding room for a line of text at the bottom of the scrollable area.


### lcd_scroll_down

`void lcd_scroll_down(void)`

Scroll the screen down one line adding room for a line of text at the top of the scrollable area.


### lcd_clear_screen

`void lcd_clear_screen(void)`

Clear the display.


### lcd_putc

`void lcd_putc(uint8_t column, uint8_t row, uint8_t c)`

Draws a glyph at a location on the display.

#### Parameters

- column - horizontal location to draw
- row – vertical location to draw
- c – glygh to draw (font offset)


### lcd_move_cursor

`void lcd_move_cursor(uint8_t column, uint8_t row)`

Move to cursor to a location.

#### Parameters

- column - horizontal location to draw
- row – vertical location to draw


### lcd_draw_cursor

`void lcd_draw_cursor(void)`

Draws the cursor, if enabled.


### lcd_erase_cursor

`void lcd_erase_cursor(void)`

Erases the cursor, if enabled.


### lcd_enable_cursor

`void lcd_enable_cursor(bool cursor_on)`

Enable or disable the cursor.


### lcd_cursor_enabled

`bool lcd_cursor_enabled(void)`

Determine if the cursor is enabled.



## Onboard LED

Controlls the on-board LED taking in to account the difference with the WiFi enabled Pico.

### led_init

`int led_init(void)`

Initialises the on-board led and returns 0 if no error, or an error code.


### led_set

`void led_set(bool on)`

Lights or unlights the on-board LED.

#### Parameters

- on - true to light the LED



## Serial

The serial port is available throught the USB C port at the top of the PicoCalc or through the header on the left-side.

### serial_init

`void serial_init(uint baudrate, uint databits, uint stopbits, uart_parity_t parity)`

Initial the serial port.

#### Parameters

- baudrate – Baudrate of UART in Hz
- databits – Number of bits of data, 5 through 8
- stopbits – Number of stop bits, 1 or 2
- parity – one of UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD


### serial_char_available

`bool serial_input_available(void)`

Returns true if a serial character is available.


### serial_get_char

`char serial_get_char(void)`

Returns the next character from the serial input buffer; blocks if input is not available.


### serial_output_available

`bool serial_output_available(void)`

Returns true is a character can be emitted.


### serial_put_char

`void serial_put_char(char ch)`

Emits a character through the serial port.

#### Parameters

- ch - the character to emit


## Southbridge

The southbridge is the MPU (STM32F103R8T6) on the mainboard of the PicoCalc. This MPU interfaces the low-speed devices to the Pico.

### sb_read_keyboard

`uint16_t sb_read_keyboard(void)`

Read a key status and code from the keyboard as a 16-bit half-word. The upper byte is the key status, the lower byte is the key code. 


### sb_read_battery

`uint8_t sb_read_battery(void)`

Read the battery status. The MSB of the returned valus is set if the battery is charging.

### sb_read_lcd_backlight

`uint8_t sb_read_lcd_backlight(void)`

Read the current LCD Display backlight brightness, 0 (dark) to 255 (bright).


### sb_write_lcd_backlight

`void sb_write_lcd_backlight(uint8_t brightness)`

Sets the LCD Display backlight brightness.

#### Parameters

- brightness – a value between 0 (dark) and 255 (bright)


### sb_read_keyboard_backlight

`uint8_t sb_read_keyboard_backlight(void)`

Reads the current keyboard backlight brightness, 0 (dark) to 255 (bright).


### sb_write_keyboard_backlight

`void sb_write_keyboard_backlight(uint8_t brightness)`

Sets the keyboard backlight brightness.

#### Parameters

- brightness – a value between 0 (dark) and 255 (bright)



# Examples

The main function implements a simple REPL to demonstrate different cababilities of this starter project:

- **backlight** - Displays the backlight values for the display and keyboard
- **battery** – Displays the battery level and status (graphically)
- **box** – Draws a yellow box using special graphics characters
- **bye** – Reboots the device into BOOTSEL mode
- **clear** – Clears the display
- **speedtest** – Display driver speed test
- **help** – Lists the available commands


## Speed Test Notes

The speed test is a fun excercise that allows one is determine what to expect when using the display driver for your project. The scrolling test is scrolling full lines of text using printf() to display the row number and changing the colour of each line.

Likewise, the characters per second test is positioning the cursor, changing the colour and displaying the character.

