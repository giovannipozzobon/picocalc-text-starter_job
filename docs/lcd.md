# LCD

The driver for the LCD display is optimised for displaying text. For performance, the display is operated in 65K colour depth (RGB565) and any colour from this colour depth can be chosen and used. 

This driver uses very little RAM leaving more for your project as a frame buffer in the Pico RAM is not used.

## lcd_init

`void lcd_init(void)`

Initialise the LCD controller.


## lcd_display_on

`void lcd_display_on(void)`

Turn the display on.


## lcd_display_off

`void lcd_display_off(void)`

Turn the display off.


## lcd_blit

`void lcd_blit(uint16_t *pixels, uint16_t x, uint16_t y, uint16_t width, uint16_t height)`

Writes pixel data to a region of the frame buffer in the display controller and takes into account the scrolled display.

### Parameters

- pixels – array of pixels (RGB565)
- x – left edge corner of the region in pixels
- y – top edge of the region in pixels
- width – width of the region in pixels
- height - height of the region in pixels


## lcd_solid_rectangle

`void lcd_solid_rectangle(uint16_t colour, uint16_t x, uint16_t y, uint16_t width, uint16_t height)`

Draws a solid rectangle using a single colour.

### Parameters

- colour – the RGB565 colour
- x – left edge corner of the rectangle in pixels
- y – top edge of the rectangle in pixels
- width – width of the rectangle in pixels
- height - height of the rectangle in pixels


## lcd_define_scrolling

`void lcd_define_scrolling(uint16_t top_fixed_area, uint16_t bottom_fixed_area)`

Define the area that will be scrolled on the display. The scrollable area is between the top fixed area and the bottom fixed area.

### Parameters

- top_fixed_area – Number of pixel rows fixed at the top of the display
- bottom_fixed_area – Number of pixel rows fixed at the bottom of the display


## lcd_scroll_up

`void lcd_scroll_up(void)`

Scroll the screen up one line adding room for a line of text at the bottom of the scrollable area.


## lcd_scroll_down

`void lcd_scroll_down(void)`

Scroll the screen down one line adding room for a line of text at the top of the scrollable area.


## lcd_clear_screen

`void lcd_clear_screen(void)`

Clear the display.


## lcd_putc

`void lcd_putc(uint8_t column, uint8_t row, uint8_t c)`

Draws a glyph at a location on the display.

### Parameters

- column - horizontal location to draw
- row – vertical location to draw
- c – glygh to draw (font offset)


## lcd_move_cursor

`void lcd_move_cursor(uint8_t column, uint8_t row)`

Move to cursor to a location.

### Parameters

- column - horizontal location to draw
- row – vertical location to draw


## lcd_draw_cursor

`void lcd_draw_cursor(void)`

Draws the cursor, if enabled.


## lcd_erase_cursor

`void lcd_erase_cursor(void)`

Erases the cursor, if enabled.


## lcd_enable_cursor

`void lcd_enable_cursor(bool cursor_on)`

Enable or disable the cursor.


## lcd_cursor_enabled

`bool lcd_cursor_enabled(void)`

Determine if the cursor is enabled.
