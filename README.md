# picocalc-text-starter

With this starter project, you will be able to get started on the PicoCalc using the Pico-Series C/C++ SDK. You can create the [text-based user interface](https://en.wikipedia.org/wiki/Text-based_user_interface) experience of the 1980's that are well suited for a mouseless system.

This starter contains code to write text to the LCD display and read input from the keyboard using the C stdio <stdio.h> library (printf, scanf, getchar, putchar, ...).

*This starter is not intended for getting started with only a Raspberry Pi Pico as you should already have, or will need to gain, knowledge with the Pico-series C/C++ SDK to use this starter project.*

This starter provides source code for accessing the peripherals of the PicoCalc device so you can concentrate on creating your project.

> This is a starter project. Feel free to take bits and pieces and modify what is here to suit YOUR project.

## Display

The driver for the LCD display is driven by a high-performance library optimised for displaying text and controlled through ANSI escape sequences. For performance, the display is operated in 65K colour depth (RGB565) and any colour from this colour depth can be chosen and used. 

A frame buffer on the Pico not used; this driver uses very little RAM leaving more for your projects.

Colours default to plain ASCII (black and phosphor), or you can use ANSI 16-colour, 256-colour palette (216 colors + 16 ANSI + 24 gray) and 24-bit truecolor (approximate to 65K colours). You can chose a between white, green or amber presets as your default phosphor colour, or chose your own.

The [Special Graphics Characters](https://vt100.net/docs/vt100-ug/chapter3.html#T3-9) of the VT100 are also supported.

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

## Keyboard

The keyboard driver operates with a timer loop that polls the PicoCalc's southbridge for key presses. Unfortunately, the southbridge cannot notify the Pico when a key is pressed.

The purpose of this implementation was support:

- type ahead
- keyboard user interrupts

The type ahead buffer allows users to type even while your project is processing. When Brk (Shift-Esc) is pressed, a flag is set allowing your project to monitor and stop processing, if desired. 


## Examples

The main function implements a simple REPL to demonstrate different cababilities of this starter project:

- **battery** – Displays the battery level and status
- **box** – Draws a yellow box using special graphics characters
- **bye** – Reboots the device into BOOTSEL mode
- **clear** – Clears the display
- **speedtest** – Display driver speed test
- **help** – Lists the available commands


## Speed Test Notes

The speed test is written to attempt to provide real-world metrics. The scrolling test is scrolling full lines of text using printf() to display the row number and changing the colour of each line.

Likewise, the characters per second test is positioning the cursor, changing the colour and displaying the character.

