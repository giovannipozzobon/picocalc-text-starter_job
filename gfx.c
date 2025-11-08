#include "gfx.h"
#include "drivers/lcd.h"
#include <string.h>
#include <stdlib.h>
#include "pico/time.h"

/* Internal storage:
   - tilesheet_ptr: pointer provided by user (not copied), tiles are consecutive 16*16 pixels per tile
   - tiles_count: number of tiles available in the tilesheet
   - two tilemaps (front/back) of uint16_t tile indices; on gfx_present() we draw tiles that differ (back != front)
   - dirty flags per tile optional (we use compare to reduce memory)
*/

#define GFX_BACKGROUND_COLOR 0x0000  // black

static const uint16_t background = GFX_BACKGROUND_COLOR;
static const uint16_t *tilesheet = NULL;
static uint16_t tiles_count = 0;

/* Tilemaps */
static uint16_t tilemap_front[GFX_TILEMAP_SIZE]; /* currently shown on display */
static uint16_t tilemap_back[GFX_TILEMAP_SIZE];  /* next frame contents */

/* Simple dirty all flag to force full redraw */
static bool force_full_redraw = true;

/* VBlank synchronization */
#define VBLANK_PERIOD_US 16667  // 60Hz = ~16.67ms per frame
static absolute_time_t next_vblank_time;
static bool vblank_sync_enabled = true;

/* Sprite storage */
static gfx_sprite_info_t sprites[GFX_MAX_SPRITES];

/* Double buffering for sprite composition - avoids malloc/free with async DMA */
#define SPRITE_BUFFER_POOL_SIZE 2
#define MAX_SPRITE_PIXELS (32 * 32)  // Max 32x32 sprite
static uint16_t sprite_buffers[SPRITE_BUFFER_POOL_SIZE][MAX_SPRITE_PIXELS] __attribute__((aligned(4)));
static volatile bool sprite_buffer_in_use[SPRITE_BUFFER_POOL_SIZE] = {false, false};
static int next_buffer_index = 0;

/* Helper: index in tilemap from tx,ty */
static inline uint32_t _tile_index(uint16_t tx, uint16_t ty) {
    return (uint32_t)ty * GFX_TILES_X + tx;
}

/* Clip helper */
static inline int16_t _clamp_i16(int16_t v, int16_t a, int16_t b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

/* DMA completion callback - called from interrupt context */
static void gfx_on_dma_complete(const uint16_t *buffer) {
    // IMPORTANT: This is called from interrupt context - must be fast and safe!
    // Only release buffer if it's from our pool
    for (int i = 0; i < SPRITE_BUFFER_POOL_SIZE; i++) {
        if (buffer == sprite_buffers[i]) {
            sprite_buffer_in_use[i] = false;
            return;
        }
    }

    // If buffer is not from pool, do NOT free it!
    // It could be:
    // - A tile buffer (temporary, will be freed by caller)
    // - A const buffer (from ROM/flash)
    // - A stack buffer
    // Calling free() in interrupt context is unsafe and can cause HardFault!
    // So we simply ignore non-pool buffers.
}

/* Buffer pool management for sprite composition */
static uint16_t* gfx_alloc_sprite_buffer(void) {
    // Use round-robin allocation for double buffering
    int idx = next_buffer_index;

    /* With true async DMA and callback-based release:
     * - We don't wait here anymore
     * - If buffer is in use, we skip to next buffer
     * - With 2 buffers, this gives DMA time to complete
     */
    if (sprite_buffer_in_use[idx]) {
        // Buffer still in use by DMA, try next one
        idx = (idx + 1) % SPRITE_BUFFER_POOL_SIZE;
        if (sprite_buffer_in_use[idx]) {
            // Both buffers in use - this means we're rendering faster than DMA can transfer
            // Fall back to malloc (rare case)
            return NULL;
        }
    }

    sprite_buffer_in_use[idx] = true;
    next_buffer_index = (idx + 1) % SPRITE_BUFFER_POOL_SIZE;
    return sprite_buffers[idx];
}

static void gfx_free_sprite_buffer(uint16_t* buffer) {
    // With async DMA, we DON'T free immediately
    // The buffer will be freed by gfx_on_dma_complete() callback
    // when DMA completes
    // So this function is now a no-op for pool buffers

    // Only free if it's a malloc'd buffer (not from pool)
    bool is_pool_buffer = false;
    for (int i = 0; i < SPRITE_BUFFER_POOL_SIZE; i++) {
        if (buffer == sprite_buffers[i]) {
            is_pool_buffer = true;
            break;
        }
    }

    if (!is_pool_buffer && buffer != NULL) {
        // This was malloc'd, free it immediately
        // (shouldn't happen in normal operation)
        free(buffer);
    }
    // For pool buffers, do nothing - callback will handle it
}

/* Initialize gfx */
void gfx_init(const uint16_t *tilesheet_ptr, uint16_t tcount) {
    tilesheet = tilesheet_ptr;
    tiles_count = tcount;

    /* init tilemaps to "blank" sentinel (UINT16_MAX) */
    for (uint32_t i = 0; i < GFX_TILEMAP_SIZE; i++) {
        tilemap_front[i] = UINT16_MAX;
        tilemap_back[i] = UINT16_MAX;
    }

    /* clear sprites */
    for (int i = 0; i < GFX_MAX_SPRITES; i++) {
        sprites[i].active = false;
        sprites[i].image = NULL;
        sprites[i].has_prev = false;
    }

    /* Register DMA completion callback for async buffer management */
    lcd_set_dma_completion_callback(gfx_on_dma_complete);

    /* Initialize vblank timer */
    next_vblank_time = make_timeout_time_us(VBLANK_PERIOD_US);

    force_full_redraw = true;
}

/* Set tilesheet pointer */
void gfx_set_tilesheet(const uint16_t *tilesheet_ptr, uint16_t tcount) {
    tilesheet = tilesheet_ptr;
    tiles_count = tcount;
    force_full_redraw = true;
}

/* Set tile in backmap */
void gfx_set_tile(uint16_t tx, uint16_t ty, uint16_t tile_index) {
    if (tx >= GFX_TILES_X || ty >= GFX_TILES_Y) return;
    tilemap_back[_tile_index(tx, ty)] = tile_index;
}

/* Get tile from backmap */
uint16_t gfx_get_tile(uint16_t tx, uint16_t ty) {
    if (tx >= GFX_TILES_X || ty >= GFX_TILES_Y) return UINT16_MAX;
    return tilemap_back[_tile_index(tx, ty)];
}

/* Clear backmap to tile_index */
void gfx_clear_backmap(uint16_t tile_index) {
    for (uint32_t i = 0; i < GFX_TILEMAP_SIZE; i++) {
        tilemap_back[i] = tile_index;
    }
    force_full_redraw = true;
}

/* Mark all dirty */
void gfx_mark_all_dirty(void) {
    force_full_redraw = true;
}

/* Force draw single tile immediately (uses tilemap_back content) */
void gfx_force_draw_tile(uint16_t tx, uint16_t ty) {
    if (tx >= GFX_TILES_X || ty >= GFX_TILES_Y) return;
    uint16_t tile_index = tilemap_back[_tile_index(tx, ty)];
    uint16_t screen_x = tx * GFX_TILE_W;
    uint16_t screen_y = ty * GFX_TILE_H;

    if (tile_index == UINT16_MAX) {
        /* blank: fill with background colour */
        /* use lcd_solid_rectangle for simplicity */
        lcd_solid_rectangle(background, screen_x, screen_y, GFX_TILE_W, GFX_TILE_H);
        /* sync front map */
        tilemap_front[_tile_index(tx, ty)] = tile_index;
        return;
    }
    if (tilesheet == NULL) return; /* nothing to draw */
    if (tile_index >= tiles_count) return;

    const uint16_t *tile_pixels = tilesheet + (size_t)tile_index * (GFX_TILE_W * GFX_TILE_H);
    lcd_blit(tile_pixels, screen_x, screen_y, GFX_TILE_W, GFX_TILE_H);
    tilemap_front[_tile_index(tx, ty)] = tile_index;
}

/* Internal: draw tile at tx,ty from tile_index (no frontmap update) */
static void _draw_tile_pixels(uint16_t tile_index, uint16_t screen_x, uint16_t screen_y) {
    if (tile_index == UINT16_MAX) {
        lcd_solid_rectangle(background, screen_x, screen_y, GFX_TILE_W, GFX_TILE_H);
        return;
    }
    if (!tilesheet) return;
    if (tile_index >= tiles_count) return;
    const uint16_t *tile_pixels = tilesheet + (size_t)tile_index * (GFX_TILE_W * GFX_TILE_H);
    lcd_blit(tile_pixels, screen_x, screen_y, GFX_TILE_W, GFX_TILE_H);
}

/* Helper: redraw tiles covered by a rectangular region (to erase sprite) */
static void _redraw_region(int16_t x, int16_t y, uint8_t w, uint8_t h) {
    if (w == 0 || h == 0) return;

    /* Calculate which tiles are affected */
    int16_t tile_x_start = x / GFX_TILE_W;
    int16_t tile_y_start = y / GFX_TILE_H;
    int16_t tile_x_end = (x + w - 1) / GFX_TILE_W;
    int16_t tile_y_end = (y + h - 1) / GFX_TILE_H;

    /* Clamp to valid tile range */
    if (tile_x_start < 0) tile_x_start = 0;
    if (tile_y_start < 0) tile_y_start = 0;
    if (tile_x_end >= GFX_TILES_X) tile_x_end = GFX_TILES_X - 1;
    if (tile_y_end >= GFX_TILES_Y) tile_y_end = GFX_TILES_Y - 1;

    /* Redraw affected tiles */
    for (int16_t ty = tile_y_start; ty <= tile_y_end; ty++) {
        for (int16_t tx = tile_x_start; tx <= tile_x_end; tx++) {
            uint32_t idx = _tile_index(tx, ty);
            uint16_t t = tilemap_back[idx];
            uint16_t sx = tx * GFX_TILE_W;
            uint16_t sy = ty * GFX_TILE_H;
            _draw_tile_pixels(t, sx, sy);
            tilemap_front[idx] = t;
        }
    }
}

/* Present: compare backmap to frontmap, draw changed tiles, then draw sprites (sorted by z)
   After present, frontmap is updated to match backmap.
*/
void gfx_present(void) {
    /* VBlank synchronization: wait until next vblank period to reduce tearing */
    if (vblank_sync_enabled) {
        // Wait until the next vblank time
        sleep_until(next_vblank_time);

        // Schedule next vblank
        next_vblank_time = delayed_by_us(next_vblank_time, VBLANK_PERIOD_US);
    }

    /* First, erase previous sprite positions by redrawing tiles underneath */
    for (int i = 0; i < GFX_MAX_SPRITES; i++) {
        if (sprites[i].active && sprites[i].has_prev) {
            /* Redraw tiles at previous sprite position */
            _redraw_region(sprites[i].prev_x, sprites[i].prev_y, sprites[i].w, sprites[i].h);
        }
    }

    /* Draw tiles that changed (or everything if forced) */
    if (force_full_redraw) {
        for (uint16_t ty = 0; ty < GFX_TILES_Y; ty++) {
            for (uint16_t tx = 0; tx < GFX_TILES_X; tx++) {
                uint32_t idx = _tile_index(tx, ty);
                uint16_t t = tilemap_back[idx];
                uint16_t sx = tx * GFX_TILE_W;
                uint16_t sy = ty * GFX_TILE_H;
                _draw_tile_pixels(t, sx, sy);
                tilemap_front[idx] = t;
            }
        }
        force_full_redraw = false;
    } else {
        for (uint16_t ty = 0; ty < GFX_TILES_Y; ty++) {
            for (uint16_t tx = 0; tx < GFX_TILES_X; tx++) {
                uint32_t idx = _tile_index(tx, ty);
                uint16_t t_back = tilemap_back[idx];
                uint16_t t_front = tilemap_front[idx];
                if (t_back != t_front) {
                    uint16_t sx = tx * GFX_TILE_W;
                    uint16_t sy = ty * GFX_TILE_H;
                    _draw_tile_pixels(t_back, sx, sy);
                    tilemap_front[idx] = t_back;
                }
            }
        }
    }

    /* Draw sprites: sort by z (simple stable selection) */
    /* Build index list of active sprites */
    int active_ids[GFX_MAX_SPRITES];
    int active_count = 0;
    for (int i = 0; i < GFX_MAX_SPRITES; i++) {
        if (sprites[i].active && sprites[i].image && sprites[i].w > 0 && sprites[i].h > 0) {
            active_ids[active_count++] = i;
        }
    }

    /* simple insertion sort by z */
    for (int i = 1; i < active_count; i++) {
        int key = active_ids[i];
        int j = i - 1;
        while (j >= 0 && sprites[active_ids[j]].z > sprites[key].z) {
            active_ids[j + 1] = active_ids[j];
            j--;
        }
        active_ids[j + 1] = key;
    }

    /* For each sprite, composite its image over current tiles and blit */
    /* We'll create a temporary buffer sized w*h for each sprite (stack allocate if small) */
    for (int idx_i = 0; idx_i < active_count; idx_i++) {
        int si = active_ids[idx_i];
        gfx_sprite_info_t *s = &sprites[si];
        if (!s->active) continue;

        /* Clip quickly if entire sprite off-screen */
        if (s->x + s->w <= 0 || s->y + s->h <= 0 || s->x >= (int)WIDTH || s->y >= (int)HEIGHT) {
            continue;
        }

        /* Allocate temp buffer from pool (avoids malloc/free with async DMA) */
        size_t spixels = (size_t)s->w * s->h;

        /* Check if sprite fits in our buffer pool */
        uint16_t *temp = NULL;
        if (spixels <= MAX_SPRITE_PIXELS) {
            temp = gfx_alloc_sprite_buffer();
        }

        /* If pool allocation failed or sprite too large, use malloc as fallback */
        if (!temp) {
            temp = malloc(spixels * sizeof(uint16_t));
        }

        if (!temp) continue; /* allocation failed -> skip sprite */

        /* Composite each pixel:
           if sprite pixel != TRANSPARENT -> use it
           else -> get corresponding background pixel by sampling tilesheet from tilemap_back (already drawn on screen)
        */
        for (uint16_t yy = 0; yy < s->h; yy++) {
            for (uint16_t xx = 0; xx < s->w; xx++) {
                int scr_x = s->x + xx;
                int scr_y = s->y + yy;
                uint16_t outpix = background; /* fallback */
                if (scr_x < 0 || scr_y < 0 || scr_x >= (int)WIDTH || scr_y >= (int)HEIGHT) {
                    /* off screen pixel -> don't draw (we still fill temporary to keep indexing simple) */
                    outpix = background;
                } else {
                    uint32_t sprite_idx = (uint32_t)yy * s->w + xx;
                    uint16_t spx = s->image[sprite_idx];
                    if (spx != GFX_TRANSPARENT_COLOR) {
                        outpix = spx;
                    } else {
                        /* find which tile covers scr_x,scr_y and sample its pixel from tilesheet */
                        uint16_t tx = scr_x / GFX_TILE_W;
                        uint16_t ty = scr_y / GFX_TILE_H;
                        uint32_t tmap_i = _tile_index(tx, ty);
                        uint16_t tindex = tilemap_back[tmap_i]; /* background tile from backmap */
                        if (tindex == UINT16_MAX || tilesheet == NULL || tindex >= tiles_count) {
                            outpix = background;
                        } else {
                            /* pixel within tile */
                            uint16_t px_in_tile = scr_x % GFX_TILE_W;
                            uint16_t py_in_tile = scr_y % GFX_TILE_H;
                            const uint16_t *tile_pixels = tilesheet + (size_t)tindex * (GFX_TILE_W * GFX_TILE_H);
                            outpix = tile_pixels[(size_t)py_in_tile * GFX_TILE_W + px_in_tile];
                        }
                    }
                }
                temp[(size_t)yy * s->w + xx] = outpix;
            }
        }

        /* Blit composed sprite block (lcd_blit handles windowing/clipping) */
        lcd_blit(temp, (uint16_t) s->x, (uint16_t) s->y, s->w, s->h);

        /* NOTE: With double buffering, we DON'T free immediately.
         * The buffer remains valid because:
         * 1. We use a pool of 2 buffers
         * 2. While DMA transfers buffer 0, we prepare sprites in buffer 1
         * 3. Next sprite will get the other buffer from the pool
         * 4. By the time we loop back, DMA has completed and buffer is free
         */
        gfx_free_sprite_buffer(temp);

        /* Save current position as previous for next frame */
        s->prev_x = s->x;
        s->prev_y = s->y;
        s->has_prev = true;
    }
}

/* Sprites API implementations */
gfx_sprite_t gfx_create_sprite(const uint16_t *image, uint8_t w, uint8_t h, int16_t x, int16_t y, uint8_t z) {
    for (int i = 0; i < GFX_MAX_SPRITES; i++) {
        if (!sprites[i].active) {
            sprites[i].active = true;
            sprites[i].image = image;
            sprites[i].w = w;
            sprites[i].h = h;
            sprites[i].x = x;
            sprites[i].y = y;
            sprites[i].z = z;
            sprites[i].has_prev = false; /* No previous position yet */
            return i;
        }
    }
    return -1;
}

bool gfx_destroy_sprite(gfx_sprite_t id) {
    if (id < 0 || id >= GFX_MAX_SPRITES) return false;

    /* Erase sprite at current position before destroying */
    if (sprites[id].active && sprites[id].has_prev) {
        _redraw_region(sprites[id].x, sprites[id].y, sprites[id].w, sprites[id].h);
    }

    sprites[id].active = false;
    sprites[id].image = NULL;
    sprites[id].has_prev = false;
    return true;
}

bool gfx_move_sprite(gfx_sprite_t id, int16_t x, int16_t y) {
    if (id < 0 || id >= GFX_MAX_SPRITES) return false;
    if (!sprites[id].active) return false;
    sprites[id].x = x;
    sprites[id].y = y;
    return true;
}

bool gfx_set_sprite_z(gfx_sprite_t id, uint8_t z) {
    if (id < 0 || id >= GFX_MAX_SPRITES) return false;
    if (!sprites[id].active) return false;
    sprites[id].z = z;
    return true;
}

bool gfx_set_sprite_image(gfx_sprite_t id, const uint16_t *image, uint8_t w, uint8_t h) {
    if (id < 0 || id >= GFX_MAX_SPRITES) return false;
    if (!sprites[id].active) return false;
    sprites[id].image = image;
    sprites[id].w = w;
    sprites[id].h = h;
    return true;
}

/* Fill rectangle in tile units on backmap */
void gfx_fill_tiles_rect(uint16_t tx, uint16_t ty, uint16_t tw, uint16_t th, uint16_t tile_index) {
    for (uint16_t yy = ty; yy < ty + th && yy < GFX_TILES_Y; yy++) {
        for (uint16_t xx = tx; xx < tx + tw && xx < GFX_TILES_X; xx++) {
            tilemap_back[_tile_index(xx, yy)] = tile_index;
        }
    }
}

/* VBlank synchronization control */
void gfx_set_vblank_sync(bool enabled) {
    vblank_sync_enabled = enabled;
    if (enabled) {
        // Reset vblank timer when re-enabling
        next_vblank_time = make_timeout_time_us(VBLANK_PERIOD_US);
    }
}

bool gfx_get_vblank_sync(void) {
    return vblank_sync_enabled;
}
