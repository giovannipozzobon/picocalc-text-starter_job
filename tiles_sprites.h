#include <stdint.h>
#include "gfx.h"

/* =====================================================================
   Tileset di esempio (4 tile 16x16) - RGB565
   ---------------------------------------------------------------------
   Tile 0: blu
   Tile 1: verde
   Tile 2: rosso
   Tile 3: scacchiera bianco/nero
   ===================================================================== */

const uint16_t my_tilesheet[] = {
    /* --- Tile 0: blu --- */
    #define TILE_COLOR_BLUE 0x001F
    #define TILE_COLOR_GREEN 0x07E0
    #define TILE_COLOR_RED 0xF800
    #define TILE_COLOR_WHITE 0xFFFF
    #define TILE_COLOR_BLACK 0x0000
    #define TILE_COLOR_YELLOW 0xFFE0
    #define TILE_COLOR_MAGENTA 0xF81F
    #define TILE_COLOR_CYAN 0x07FF
    /* 16x16 pixels per tile = 256 valori */
    #define FILL_TILE(color) \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color, \
        color,color,color,color,color,color,color,color,color,color,color,color,color,color,color,color

    FILL_TILE(TILE_COLOR_BLUE),

    /* --- Tile 1: verde --- */
    FILL_TILE(TILE_COLOR_GREEN),

    /* --- Tile 2: rosso --- */
    FILL_TILE(TILE_COLOR_RED),

    /* --- Tile 3: scacchiera bianco/nero --- */
    #undef FILL_TILE
    #define CHECKER(c1,c2) \
        c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2, \
        c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1, \
        c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2, \
        c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1, \
        c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2, \
        c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1, \
        c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2, \
        c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1, \
        c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2, \
        c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1, \
        c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2, \
        c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1, \
        c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2, \
        c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1, \
        c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2, \
        c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1,c2,c1

    CHECKER(TILE_COLOR_WHITE, TILE_COLOR_BLACK)
};

const uint16_t my_tilesheet_count = 4;

/* =====================================================================
   Sprite di esempio (16x16) - croce semitrasparente
   ---------------------------------------------------------------------
   Trasparente: GFX_TRANSPARENT_COLOR (bianco)
   Colore croce: giallo
   ===================================================================== */
const uint16_t sprite1_pixels[16 * 16] = {
    #define X GFX_TRANSPARENT_COLOR
    #define C TILE_COLOR_YELLOW
    X,X,X,X,C,C,X,X,X,X,C,C,X,X,X,X,
    X,X,X,X,C,C,X,X,X,X,C,C,X,X,X,X,
    X,X,X,X,C,C,X,X,X,X,C,C,X,X,X,X,
    X,X,X,X,C,C,X,X,X,X,C,C,X,X,X,X,
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,
    X,X,X,X,C,C,X,X,X,X,C,C,X,X,X,X,
    X,X,X,X,C,C,X,X,X,X,C,C,X,X,X,X,
    X,X,X,X,C,C,X,X,X,X,C,C,X,X,X,X,
    X,X,X,X,C,C,X,X,X,X,C,C,X,X,X,X,
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,
    X,X,X,X,C,C,X,X,X,X,C,C,X,X,X,X,
    X,X,X,X,C,C,X,X,X,X,C,C,X,X,X,X,
    X,X,X,X,C,C,X,X,X,X,C,C,X,X,X,X,
    X,X,X,X,C,C,X,X,X,X,C,C,X,X,X,X
};
