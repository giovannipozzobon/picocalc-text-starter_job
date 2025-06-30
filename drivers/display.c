//
//  PicoCalc LCD display driver
//
//  This driver implements a simple VT100 terminal interface for the PicoCalc LCD display
//  using the ST7789P LCD controller.
//
//  It is optimised for a character-based display with a fixed-width, 8-pixel wide font
//  and 65K colours in the RGB565 format. This driver requires little memory as it
//  uses the frame memory on the controller directly.
//
//  NOTE: Some code below is written to respect timing constraints of the ST7789P controller.
//        For instance, you can usually get away with a short chip select high pulse widths, but
//        writing to the display RAM requires the minimum chip select high pulse width of 40ns.
//

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/spi.h"

#include "string.h"

#include "display.h"

//
//  LCD Driver
//
//  This section contains the definitions and variables used for handling
//  the LCD display.
//

// Cursor positioning and scrolling
uint8_t cursor_x = 0;      // cursor x position for drawing
uint8_t cursor_y = 0;      // cursor y position for drawing
uint16_t lcd_y_offset = 0; // offset for vertical scrolling

uint16_t foreground = FOREGROUND; // default foreground colour (white phosphor)
uint16_t background = BACKGROUND; // default background colour (black)

bool underscore = false;    // underscore state (not implemented)
bool reverse = false;       // reverse video state (not implemented)
bool cursor_enabled = true; // cursor visibility state

// Text drawing
extern uint8_t font[];
uint16_t char_buffer[8 * GLYPH_HEIGHT] __attribute__((aligned(4)));

// Background processing
semaphore_t lcd_sem;
static repeating_timer_t cursor_timer;

// Xterm 256-colour palette (RGB565 format)
//
// That is 5-bits for red and blue, and 6-bits for green. The display is configured to map to
// its native 18-bit RGB using the LSB of green for the LSB of red and blue, and the green
// component is unmodified.

// The RGB() macro can be used to create colors from this colour depth using 8-bits per
// channel values.

const uint16_t palette[256] = {
    // Standard 16 colors (0-15)
    0x0000, 0x8000, 0x0400, 0x8400, 0x0010, 0x8010, 0x0410, 0xC618,
    0x8410, 0xF800, 0x07E0, 0xFFE0, 0x001F, 0xF81F, 0x07FF, 0xFFFF,

    // 216 colors in 6×6×6 RGB cube (16-231)
    0x0000, 0x0010, 0x0015, 0x001F, 0x0014, 0x001F, 0x0400, 0x0410, 0x0415, 0x041F, 0x0414, 0x041F,
    0x0500, 0x0510, 0x0515, 0x051F, 0x0514, 0x051F, 0x07E0, 0x07F0, 0x07F5, 0x07FF, 0x07F4, 0x07FF,
    0x0600, 0x0610, 0x0615, 0x061F, 0x0614, 0x061F, 0x07E0, 0x07F0, 0x07F5, 0x07FF, 0x07F4, 0x07FF,
    0x8000, 0x8010, 0x8015, 0x801F, 0x8014, 0x801F, 0x8400, 0x8410, 0x8415, 0x841F, 0x8414, 0x841F,
    0x8500, 0x8510, 0x8515, 0x851F, 0x8514, 0x851F, 0x87E0, 0x87F0, 0x87F5, 0x87FF, 0x87F4, 0x87FF,
    0x8600, 0x8610, 0x8615, 0x861F, 0x8614, 0x861F, 0x87E0, 0x87F0, 0x87F5, 0x87FF, 0x87F4, 0x87FF,
    0xA000, 0xA010, 0xA015, 0xA01F, 0xA014, 0xA01F, 0xA400, 0xA410, 0xA415, 0xA41F, 0xA414, 0xA41F,
    0xA500, 0xA510, 0xA515, 0xA51F, 0xA514, 0xA51F, 0xA7E0, 0xA7F0, 0xA7F5, 0xA7FF, 0xA7F4, 0xA7FF,
    0xA600, 0xA610, 0xA615, 0xA61F, 0xA614, 0xA61F, 0xA7E0, 0xA7F0, 0xA7F5, 0xA7FF, 0xA7F4, 0xA7FF,
    0xF800, 0xF810, 0xF815, 0xF81F, 0xF814, 0xF81F, 0xFC00, 0xFC10, 0xFC15, 0xFC1F, 0xFC14, 0xFC1F,
    0xFD00, 0xFD10, 0xFD15, 0xFD1F, 0xFD14, 0xFD1F, 0xFFE0, 0xFFF0, 0xFFF5, 0xFFFF, 0xFFF4, 0xFFFF,
    0xFE00, 0xFE10, 0xFE15, 0xFE1F, 0xFE14, 0xFE1F, 0xFFE0, 0xFFF0, 0xFFF5, 0xFFFF, 0xFFF4, 0xFFFF,
    0xC000, 0xC010, 0xC015, 0xC01F, 0xC014, 0xC01F, 0xC400, 0xC410, 0xC415, 0xC41F, 0xC414, 0xC41F,
    0xC500, 0xC510, 0xC515, 0xC51F, 0xC514, 0xC51F, 0xC7E0, 0xC7F0, 0xC7F5, 0xC7FF, 0xC7F4, 0xC7FF,
    0xC600, 0xC610, 0xC615, 0xC61F, 0xC614, 0xC61F, 0xC7E0, 0xC7F0, 0xC7F5, 0xC7FF, 0xC7F4, 0xC7FF,
    0xE000, 0xE010, 0xE015, 0xE01F, 0xE014, 0xE01F, 0xE400, 0xE410, 0xE415, 0xE41F, 0xE414, 0xE41F,
    0xE500, 0xE510, 0xE515, 0xE51F, 0xE514, 0xE51F, 0xE7E0, 0xE7F0, 0xE7F5, 0xE7FF, 0xE7F4, 0xE7FF,
    0xE600, 0xE610, 0xE615, 0xE61F, 0xE614, 0xE61F, 0xE7E0, 0xE7F0, 0xE7F5, 0xE7FF, 0xE7F4, 0xE7FF,

    // 24 grayscale colors (232-255)
    0x0000, 0x1082, 0x2104, 0x3186, 0x4208, 0x528A, 0x630C, 0x738E,
    0x8410, 0x9492, 0xA514, 0xB596, 0xC618, 0xD69A, 0xE71C, 0xF79E,
    0x0841, 0x18C3, 0x2945, 0x39C7, 0x4A49, 0x5ACB, 0x6B4D, 0x7BCF};

// Set foreground colour
static void lcd_set_foreground(uint16_t color)
{
    if (reverse)
    {
        background = color; // if reverse is enabled, set background to the new foreground color
    }
    else
    {
        foreground = color;
    }
}

// Set background colour
static void lcd_set_background(uint16_t color)
{
    if (reverse)
    {
        foreground = color; // if reverse is enabled, set foreground to the new background color
    }
    else
    {
        background = color;
    }
}

static void lcd_set_reverse(bool reverse_on)
{
    // swap foreground and background colors if reverse is "reversed"
    if (reverse && !reverse_on || !reverse && reverse_on)
    {
        uint16_t temp = foreground;
        lcd_set_foreground(background);
        lcd_set_background(temp);
    }
    reverse = reverse_on;
}

static void lcd_set_underscore(bool underscore_on)
{
    // Underscore is not implemented, but we can toggle the state
    underscore = underscore_on;
}

static void lcd_set_cursor(bool cursor_on)
{
    // Cursor visibility is not implemented, but we can toggle the state
    cursor_enabled = cursor_on;
}

static bool lcd_cursor_enabled()
{
    // Return the current cursor visibility state
    return cursor_enabled;
}

// Reset the LCD display
static void lcd_reset()
{
    // Blip the reset pin to reset the LCD controller
    gpio_put(LCD_RST, 0);
    sleep_us(20); // 20µs reset pulse (10µs minimum)

    gpio_put(LCD_RST, 1);
    sleep_ms(120); // 5ms required after reset, but 120ms needed before sleep out command
}

static bool lcd_available()
{
    // Check if the semaphore is available for LCD access
    return sem_available(&lcd_sem);
}

// Protect the SPI bus with a semaphore
static void lcd_acquire()
{
    sem_acquire_blocking(&lcd_sem);
}

// Release the SPI bus
static void lcd_release()
{
    sem_release(&lcd_sem);
}

//
// Low-level SPI functions
//

// Send a command
static void lcd_write_cmd(uint8_t cmd)
{
    gpio_put(LCD_DCX, 0); // Command
    gpio_put(LCD_CSX, 0);
    spi_write_blocking(LCD_SPI, &cmd, 1);
    gpio_put(LCD_CSX, 1);
}

// Send 8-bit data (byte)
static void lcd_write_data(uint8_t len, ...)
{
    va_list args;
    va_start(args, len);
    gpio_put(LCD_DCX, 1); // Data
    gpio_put(LCD_CSX, 0);
    for (uint8_t i = 0; i < len; i++)
    {
        uint8_t data = va_arg(args, int); // get the next byte of data
        spi_write_blocking(LCD_SPI, &data, 1);
    }
    gpio_put(LCD_CSX, 1);
    va_end(args);
}

// Send 16-bit data (half-word)
static void lcd_write16_data(uint8_t len, ...)
{
    va_list args;

    // DO NOT MOVE THE spi_set_format() OR THE gpio_put(LCD_DCX) CALLS!
    // They are placed before the gpio_put(LCD_CSX) to ensure that a minimum
    // chip select high pulse width is achieved (at least 40ns)
    spi_set_format(LCD_SPI, 16, 0, 0, SPI_MSB_FIRST);

    va_start(args, len);
    gpio_put(LCD_DCX, 1); // Data
    gpio_put(LCD_CSX, 0);
    for (uint8_t i = 0; i < len; i++)
    {
        uint16_t data = va_arg(args, int); // get the next half-word of data
        spi_write16_blocking(LCD_SPI, &data, 1);
    }
    gpio_put(LCD_CSX, 1);
    va_end(args);

    spi_set_format(LCD_SPI, 8, 0, 0, SPI_MSB_FIRST);
}

// Send a buffer of 16-bit data (half-words)
static void lcd_write16_buf(const uint16_t *buffer, size_t len)
{
    // DO NOT MOVE THE spi_set_format() OR THE gpio_put(LCD_DCX) CALLS!
    // They are placed before the gpio_put(LCD_CSX) to ensure that a minimum
    // chip select high pulse width is achieved (at least 40ns)
    spi_set_format(LCD_SPI, 16, 0, 0, SPI_MSB_FIRST);

    gpio_put(LCD_DCX, 1); // Data
    gpio_put(LCD_CSX, 0);
    spi_write16_blocking(LCD_SPI, buffer, len);
    gpio_put(LCD_CSX, 1);

    spi_set_format(LCD_SPI, 8, 0, 0, SPI_MSB_FIRST);
}

//
//  ST7365P LCD controller functions
//

// Select the target of the pixel data in the display RAM that will follow
static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // lcd_acquire() and lcd_release() are not needed here, as this function
    // is only called from lcd_blit() which already acquires the semaphore

    // Set column address (X)
    lcd_write_cmd(LCD_CMD_CASET);
    lcd_write_data(4,
                   UPPER8(x0), LOWER8(x0),
                   UPPER8(x1), LOWER8(x1));

    // Set row address (Y)
    lcd_write_cmd(LCD_CMD_RASET);
    lcd_write_data(4,
                   UPPER8(y0), LOWER8(y0),
                   UPPER8(y1), LOWER8(y1));

    // Prepare to write to RAM
    lcd_write_cmd(LCD_CMD_RAMWR);
}

//
//  Send pixel data to the display
//
//  All display RAM updates come through this function. This function is responsible for
//  setting the correct window in the display RAM and writing the pixel data to it. It also
//  handles the vertical scrolling by adjusting the y-coordinate based on the current scroll
//  offset (lcd_y_offset).
//
//  The pixel data is expected to be in RGB565 format, which is a 16-bit value with the
//  red component in the upper 5 bits, the green component in the middle 6 bits, and the
//  blue component in the lower 5 bits.

static void lcd_blit(uint16_t *pixels, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    // Adjust y for vertical scroll offset and wrap within memory height
    uint16_t y_virtual = (y + lcd_y_offset) % FRAME_HEIGHT;

    lcd_acquire();
    lcd_set_window(x, y_virtual, x + width - 1, y_virtual + height - 1);
    lcd_write16_buf((uint16_t *)pixels, width * height);
    lcd_release();
}

// Draw a solid rectangle on the display
static void lcd_solid_rectangle(uint16_t color, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    static uint16_t pixels[WIDTH];

    for (uint16_t row = 0; row < height; row++)
    {
        for (uint16_t i = 0; i < width; i++)
        {
            pixels[i] = color;
        }
        lcd_blit(pixels, x, y + row, width, 1);
    }
}

//
//  Set the scrolling area of the display
//
//  This forum post provides a good explanation of how scrolling on the ST7789P display works:
//      https://forum.arduino.cc/t/st7735s-scrolling/564506
//
//  These functions (lcd_define_scrolling, lcd_scroll_up, and lcd_scroll_down) configure and
//  set the vertical scrolling area of the display, but it is the responsibility of lcd_blit()
//  to ensure that the pixel data is written to the correct location in the display RAM.
//

static void lcd_define_scrolling(uint16_t top_fixed_area, uint16_t bottom_fixed_area)
{
    uint16_t scroll_area = HEIGHT - (top_fixed_area + bottom_fixed_area);

    lcd_acquire();
    lcd_write_cmd(LCD_CMD_VSCRDEF);
    lcd_write_data(6,
                   UPPER8(top_fixed_area),
                   LOWER8(top_fixed_area),
                   UPPER8(scroll_area),
                   LOWER8(scroll_area),
                   UPPER8(bottom_fixed_area),
                   LOWER8(bottom_fixed_area));
    lcd_release();
}

// Scroll the screen up one line (make space at the bottom)
static void lcd_scroll_up()
{
    // The will rotate the content in the scroll area up by one line
    lcd_y_offset = (lcd_y_offset + GLYPH_HEIGHT) % FRAME_HEIGHT;
    lcd_acquire();
    lcd_write_cmd(LCD_CMD_VSCSAD); // Sets where in display RAM the scroll area starts
    lcd_write_data(2, UPPER8(lcd_y_offset), LOWER8(lcd_y_offset));
    lcd_release();

    // Clear the new line at the bottom
    lcd_solid_rectangle(background, 0, HEIGHT - GLYPH_HEIGHT, WIDTH, GLYPH_HEIGHT);
}

// Scroll the screen down one line (making space at the top)
static void lcd_scroll_down()
{
    // This will rotate the content in the scroll area down by one line
    lcd_y_offset = (lcd_y_offset - GLYPH_HEIGHT + FRAME_HEIGHT) % FRAME_HEIGHT;
    lcd_acquire();
    lcd_write_cmd(LCD_CMD_VSCSAD); // Sets where in display RAM the scroll area starts
    lcd_write_data(2, UPPER8(lcd_y_offset), LOWER8(lcd_y_offset));
    lcd_release();

    // Clear the new line at the top
    lcd_solid_rectangle(background, 0, 0, WIDTH, GLYPH_HEIGHT);
}

// Clear the entire screen
static void lcd_clear_screen()
{
    lcd_solid_rectangle(background, 0, 0, WIDTH, FRAME_HEIGHT);
}

// Draw a character at the specified position
static void lcd_putc(uint8_t x, uint8_t y, uint8_t c)
{
    uint8_t *glyph = &font[c * GLYPH_HEIGHT];
    uint16_t *buffer = char_buffer;

    for (uint8_t i = 0; i < GLYPH_HEIGHT; i++, glyph++)
    {
        *(buffer++) = (*glyph & 0x80) ? foreground : background;
        *(buffer++) = (*glyph & 0x40) ? foreground : background;
        *(buffer++) = (*glyph & 0x20) ? foreground : background;
        *(buffer++) = (*glyph & 0x10) ? foreground : background;
        *(buffer++) = (*glyph & 0x08) ? foreground : background;
        *(buffer++) = (*glyph & 0x04) ? foreground : background;
        *(buffer++) = (*glyph & 0x02) ? foreground : background;
        *(buffer++) = (*glyph & 0x01) ? foreground : background;
    }

    lcd_blit(char_buffer, x << 3, y * GLYPH_HEIGHT, 8, GLYPH_HEIGHT);
}

//
// The cursor
//
// A performance cheat: The cursor is drawn as a solid line at the bottom of the
// character cell. The cursor is positioned here since the printable glyphs
// do not extend to that row (on purpose). Drawing and erasing the cursor does
// not corrupt the glyphs.
//
// Except for the box drawing glyphs who do extend into that row. Disable the
// cursor when printing these if you want to see the box drawing glyphs
// uncorrupted.

// Draw the cursor at the current position
static void lcd_draw_cursor()
{
    lcd_solid_rectangle(foreground, cursor_x << 3, ((cursor_y + 1) * GLYPH_HEIGHT) - 1, 8, 1);
}

// Erase the cursor at the current position
static void lcd_erase_cursor()
{
    lcd_solid_rectangle(background, cursor_x << 3, ((cursor_y + 1) * GLYPH_HEIGHT) - 1, 8, 1);
}

// Turn on the LCD display
static void lcd_display_on()
{
    lcd_acquire();
    lcd_write_cmd(LCD_CMD_DISPON);
    lcd_release();
}

// Turn off the LCD display
static void lcd_display_off()
{
    lcd_acquire();
    lcd_write_cmd(LCD_CMD_DISPOFF);
    lcd_release();
}

//
//  VT100 Terminal Emulation
//
//  This section contains the definitions and variables used for handling
//  ANSI escape sequences, cursor positioning, and text attributes.
//
//  This implementation is lacking full support for the VT100 terminal.
//
//  Reference: https://vt100.net/docs/vt100-ug/chapter3.html
//

uint8_t state = STATE_NORMAL; // initial state of escape sequence processing
uint8_t x = 0;                // cursor x position
uint8_t y = 0;                // cursor y position

uint8_t parameters[16]; // buffer for selective parameters
uint8_t p_index = 0;    // index into the buffer

uint8_t save_x = 0; // saved cursor x position for DECSC/DECRC
uint8_t save_y = 0; // saved cursor y position for DECSC/DECRC

uint8_t g0_charset = CHARSET_ASCII; // G0 character set (default ASCII)
uint8_t g1_charset = CHARSET_ASCII; // G1 character set (default ASCII)
uint8_t active_charset = 0;         // currently active character set (0=G0, 1=G1)

void (*display_led_callback)(uint8_t) = NULL;

static void display_set_charset(uint8_t charset)
{
    active_charset = charset; // Set the active character set
}

static void display_set_g0_charset(uint8_t charset)
{
    g0_charset = charset;
}

static void display_set_g1_charset(uint8_t charset)
{
    g1_charset = charset;
}

static uint8_t display_get_current_charset()
{
    // Return the currently active character set
    return (active_charset == G0_CHARSET) ? g0_charset : g1_charset;
}

static void display_leds(uint8_t led)
{
    if (display_led_callback)
    {
        display_led_callback(led); // Call the user-defined LED callback
    }
}

bool display_emit_available()
{
    return true; // always available for output in this implementation
}

void display_emit(char ch)
{
    if (lcd_cursor_enabled())
    {
        lcd_erase_cursor(); // erase the cursor before processing the character
    }

    // State machine for processing incoming characters
    switch (state)
    {
    case STATE_ESCAPE:        // ESC character received, process the next character
        state = STATE_NORMAL; // reset state by default
        switch (ch)
        {
        case CHR_CAN:               // cancel the current escape sequence
        case CHR_SUB:               // same as CAN
            lcd_putc(x++, y, 0x02); // print a error character
            break;
        case CHR_ESC:
            state = STATE_ESCAPE; // stay in escape state
            break;
        case '7': // DECSC – Save Cursor
            save_x = x;
            save_y = y;
            break;
        case '8': // DECRC – Restore Cursor
            x = save_x;
            y = save_y;
            break;
        case 'D': // IND – Index
            y++;
            break;
        case 'E': // NEL – Next Line
            x = 0;
            y++;
            break;
        case 'M': // RI – Reverse Index
            y--;
            break;
        case 'c': // RIS – Reset To Initial State
            x = y = 0;
            lcd_set_reverse(false);
            lcd_set_foreground(FOREGROUND);
            lcd_set_background(BACKGROUND);
            lcd_set_underscore(false);
            lcd_set_cursor(true);
            display_set_g0_charset(CHARSET_ASCII); // reset character set to ASCII
            display_set_g1_charset(CHARSET_ASCII);
            lcd_define_scrolling(0, 0); // no scrolling area defined
            lcd_clear_screen();
            break;
        case '[': // CSI - Control Sequence Introducer
            p_index = 0;
            memset(parameters, 0, sizeof(parameters));
            state = STATE_CS;
            break;
        case '(': // SCS - G0 character set selection
            state = STATE_G0_SET;
            break;
        case ')': // SCS - G1 character set selection
            state = STATE_G1_SET;
            break;
        default:
            // not a valid escape sequence, should we print an error?
            break;
        }
        break;

    case STATE_CS: // in Control Sequence
        if (ch == CHR_ESC)
        {
            state = STATE_ESCAPE;
            break; // reset to escape state
        }
        else if (ch == '?') // DEC private mode
        {
            state = STATE_DEC;
        }
        else if (ch >= '0' && ch <= '9')
        {
            parameters[p_index] *= 10; // accumulate digits
            parameters[p_index] += ch - '0';
        }
        else if (ch == ';') // delimiter
        {
            if (p_index < sizeof(parameters) - 1)
            {
                p_index++;
            }
        }
        else // final character in control sequence
        {
            state = STATE_NORMAL; // reset state after processing the control sequence
            switch (ch)
            {
            case 'A': // CUU – Cursor Up
                y = MAX(0, y - parameters[0]);
                break;
            case 'B': // CUD – Cursor Down
                y = MIN(y + parameters[0], MAX_ROW);
                break;
            case 'C': // CUF – Cursor Forward
                x = MIN(x + parameters[0], MAX_COL);
                break;
            case 'D': // CUB - Cursor Backward
                x = MAX(0, x - parameters[0]);
                break;
            case 'J':               // ED – Erase In Display
                lcd_clear_screen(); // Only support clearing the entire screen (2)
                break;
            case 'm': // SGR – Select Graphic Rendition
                for (uint8_t i = 0; i <= p_index; i++)
                {
                    if (parameters[i] == 0) // attributes off
                    {
                        lcd_set_foreground(FOREGROUND);
                        lcd_set_background(BACKGROUND);
                        lcd_set_underscore(false);
                        lcd_set_reverse(false);
                    }
                    else if (parameters[i] == 1) // bold or increased intensity
                    {
                        lcd_set_foreground(BRIGHT);
                    }
                    else if (parameters[i] == 4) // underscore
                    {
                        lcd_set_underscore(true);
                    }
                    // No support for blink (5)
                    else if (parameters[i] == 7) // negative (reverse) image
                    {
                        lcd_set_reverse(true);
                    }
                    else if (parameters[i] >= 30 && parameters[i] <= 37) // foreground colour
                    {
                        lcd_set_foreground(palette[parameters[i] - 30]);
                    }
                    else if (parameters[i] >= 40 && parameters[i] <= 47) // background colour
                    {
                        lcd_set_background(palette[parameters[i] - 40]);
                    }
                    else if (parameters[i] >= 90 && parameters[i] <= 97) // bright foreground colour
                    {
                        lcd_set_foreground(palette[parameters[i] - 90 + 8]);
                    }
                    else if (parameters[i] >= 100 && parameters[i] <= 107) // bright background colour
                    {
                        lcd_set_background(palette[parameters[i] - 100 + 8]);
                    }
                    // 256-color support: ESC[38;5;Nm (foreground) and ESC[48;5;Nm (background)
                    else if (parameters[i] == 38 && i + 2 <= p_index && parameters[i + 1] == 5) // foreground 256-color
                    {
                        uint8_t color = parameters[i + 2];
                        if (color < 256)
                        {
                            lcd_set_foreground(palette[color]);
                        }
                        i += 2; // Skip the next two parameters (5 and color)
                    }
                    else if (parameters[i] == 48 && i + 2 <= p_index && parameters[i + 1] == 5) // background 256-color
                    {
                        uint8_t color = parameters[i + 2];
                        if (color < 256)
                        {
                            lcd_set_background(palette[color]);
                        }
                        i += 2; // Skip the next two parameters (5 and color)
                    }
                    // 24-bit truecolor support: ESC[38;2;r;g;b;m (foreground) and ESC[48;2;r;g;b;m (background)
                    else if (parameters[i] == 38 && i + 4 <= p_index && parameters[i + 1] == 2) // foreground truecolor
                    {
                        uint8_t r = parameters[i + 2];
                        uint8_t g = parameters[i + 3];
                        uint8_t b = parameters[i + 4];
                        uint16_t color = RGB(r, g, b);
                        lcd_set_foreground(color);
                        i += 4; // Skip the next four parameters (2, r, g, b)
                    }
                    else if (parameters[i] == 48 && i + 4 <= p_index && parameters[i + 1] == 2) // background truecolor
                    {
                        uint8_t r = parameters[i + 2];
                        uint8_t g = parameters[i + 3];
                        uint8_t b = parameters[i + 4];
                        uint16_t color = RGB(r, g, b);
                        lcd_set_background(color);
                        i += 4; // Skip the next four parameters (2, r, g, b)
                    }
                }
                break;
            case 'f': // HVP – Horizontal and Vertical Position
            case 'H': // CUP – Cursor Position
                y = MIN(parameters[0], MAX_ROW) - 1;
                x = MIN(parameters[1], MAX_COL) - 1;
                break;
            case CHR_CAN:               // cancel the current escape sequence
            case CHR_SUB:               // same as CAN
                lcd_putc(x++, y, 0x02); // print a error character
                break;
            case 'q': // DECLL – Load LEDS (DEC Private)
                display_leds(parameters[0]); // Set the LEDs based on the first parameter
                break;
            default:
                break; // ignore unknown sequences
            }
        }
        break;

    case STATE_DEC: // in DEC private mode sequence
        if (ch == CHR_ESC)
        {
            state = STATE_ESCAPE;
            break; // reset to escape state
        }
        else if (ch >= '0' && ch <= '9')
        {
            parameters[p_index] *= 10; // accumulate digits
            parameters[p_index] += ch - '0';
        }
        else if (ch == ';') // delimiter
        {
            if (p_index < sizeof(parameters) - 1)
            {
                p_index++;
            }
        }
        else // final character in DEC private mode sequence
        {
            state = STATE_NORMAL; // reset state after processing
            switch (ch)
            {
            case 'h':                    // DECSET - DEC Private Mode Set
                if (parameters[0] == 25) // DECTCEM - Text Cursor Enable Mode
                {
                    lcd_set_cursor(true);
                }
                break;
            case 'l':                    // DECRST - DEC Private Mode Reset
                if (parameters[0] == 25) // DECTCEM - Text Cursor Enable Mode
                {
                    lcd_set_cursor(false);
                    lcd_erase_cursor(); // immediately hide cursor
                }
                break;
            default:
                break; // ignore unknown DEC private mode sequences
            }
        }
        break;

    case STATE_G0_SET:        // Setting G0 character set
        state = STATE_NORMAL; // return to normal state after processing
        switch (ch)
        {
        case 'B': // ASCII character set
            display_set_g0_charset(CHARSET_ASCII);
            break;
        case '0': // DEC Special Character Set
            display_set_g0_charset(CHARSET_DEC);
            break;
        default:
            // Unknown character set, ignore
            break;
        }
        break;

    case STATE_G1_SET:        // Setting G1 character set
        state = STATE_NORMAL; // return to normal state after processing
        switch (ch)
        {
        case 'B': // ASCII character set
            display_set_g1_charset(CHARSET_ASCII);
            break;
        case '0': // DEC Special Character Set
            display_set_g1_charset(CHARSET_DEC);
            break;
        default:
            // Unknown character set, ignore
            break;
        }
        break;

    case STATE_NORMAL:
    default:
        // Normal/default state, process characters directly
        switch (ch)
        {
        case CHR_BS:
            x = MAX(0, x - 1); // move cursor back one space (but not before the start of the line)
            break;
        case CHR_BEL:
            // No action for bell in this implementation
            break;
        case CHR_HT:
            x += MIN(((x + 4) & ~3), MAX_COL); // move cursor forward by 1 tabstop (but not beyond the end of the line)
            break;
        case CHR_LF:
        case CHR_VT:
        case CHR_FF:
            y++; // move cursor down one line
            break;
        case CHR_CR:
            x = 0; // move cursor to the start of the line
            break;
        case CHR_SO: // Shift Out - select G1 character set
            display_set_charset(G1_CHARSET);
            break;
        case CHR_SI: // Shift In - select G0 character set
            display_set_charset(G0_CHARSET);
            break;
        case CHR_ESC:
            state = STATE_ESCAPE;
            break;
        default:
            if (ch >= 0x20 && ch < 0x7F) // printable characters
            {
                // Translate character based on active character set
                if (display_get_current_charset() == CHARSET_DEC && ch >= 0x5F && ch <= 0x7E)
                {
                    // Maps characters 0x5F - 0x7E to DEC Special Character Set
                    ch -= 0x5F;
                }

                lcd_putc(x++, y, ch);
            }
            break;
        }
        break;
    }

    // Handle wrapping and scrolling
    if (x > MAX_COL) // wrap around at end of the line
    {
        x = 0;
        y++;
    }

    if (y < 0) // scroll at top of the screen
    {
        while (y < 0) // scroll until y is non-negative
        {
            lcd_scroll_down(); // scroll down to make space at the top
            y++;
        }
    }
    if (y > MAX_ROW) // scroll at bottom of the screen
    {
        while (y > MAX_ROW) // scroll until y is within bounds
        {
            lcd_scroll_up(); // scroll up to make space at the bottom
            y--;
        }
    }

    cursor_x = x; // update cursor position for drawing
    cursor_y = y; // update cursor position for drawing
    if (lcd_cursor_enabled())
    {
        lcd_draw_cursor(); // draw the cursor at the new position
    }
}

//
//  Background processing
//
//  Handle background tasks such as blinking the cursor
//

// Blink the cursor at regular intervals
bool on_cursor_timer(repeating_timer_t *rt)
{
    static bool cursor_visible = false;

    if (!lcd_available() || !lcd_cursor_enabled())
    {
        return true; // if the SPI bus is not available or cursor is disabled, do not toggle cursor
    }

    if (cursor_visible)
    {
        lcd_erase_cursor();
    }
    else
    {
        lcd_draw_cursor();
    }

    cursor_visible = !cursor_visible; // Toggle cursor visibility
    return true;                      // Keep the timer running
}

//
//  Display Initialization
//

void display_init(led_callback_t led_callback)
{
    display_led_callback = led_callback; // Set the LED callback function

    // initialise GPIO
    gpio_init(LCD_SCL);
    gpio_init(LCD_SDI);
    gpio_init(LCD_SDO);
    gpio_init(LCD_CSX);
    gpio_init(LCD_DCX);
    gpio_init(LCD_RST);

    gpio_set_dir(LCD_SCL, GPIO_OUT);
    gpio_set_dir(LCD_SDI, GPIO_OUT);
    gpio_set_dir(LCD_CSX, GPIO_OUT);
    gpio_set_dir(LCD_DCX, GPIO_OUT);
    gpio_set_dir(LCD_RST, GPIO_OUT);

    // initialise 4-wire SPI
    spi_init(LCD_SPI, LCD_BAUDRATE);
    gpio_set_function(LCD_SCL, GPIO_FUNC_SPI);
    gpio_set_function(LCD_SDI, GPIO_FUNC_SPI);
    gpio_set_function(LCD_SDO, GPIO_FUNC_SPI);

    gpio_put(LCD_CSX, 1);
    gpio_put(LCD_RST, 1);

    lcd_reset(); // reset the LCD controller

    lcd_write_cmd(LCD_CMD_SWRESET); // reset the commands and parameters to their S/W Reset default values
    sleep_ms(10);                   // required to wait at least 5ms

    lcd_write_cmd(LCD_CMD_COLMOD); // pixel format set
    lcd_write_data(1, 0x55);       // 16 bit/pixel (RGB565)

    lcd_write_cmd(LCD_CMD_MADCTL); // memory access control
    lcd_write_data(1, 0x48);       // BGR colour filter panel, top to bottom, left to right

    lcd_write_cmd(LCD_CMD_INVON); // display inversion on

    lcd_write_cmd(LCD_CMD_EMS); // entry mode set
    lcd_write_data(1, 0xC6);    // normal display, 16-bit (RGB) to 18-bit (rgb) color
                                //   conversion: r(0) = b(0) = G(0)

    lcd_write_cmd(LCD_CMD_VSCRDEF); // vertical scroll definition
    lcd_write_data(6,
                   0x00, 0x00, // top fixed area of 0 pixels
                   0x01, 0x40, // scroll area height of 320 pixels
                   0x00, 0x00  // bottom fixed area of 0 pixels
    );

    lcd_write_cmd(LCD_CMD_SLPOUT); // sleep out
    sleep_ms(10);                  // required to wait at least 5ms

    // Prevent the blinking cursor from interfering with other operations
    sem_init(&lcd_sem, 1, 1);

    // Clear the screen
    lcd_clear_screen();

    // Now that the display is initialized, display RAM garbage is cleared,
    // turn on the display
    lcd_display_on();

    // Blink the cursor every second (500 ms on, 500 ms off)
    add_repeating_timer_ms(500, on_cursor_timer, NULL, &cursor_timer);
}