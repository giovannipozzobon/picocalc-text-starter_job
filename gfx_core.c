//
// Graphics Core - Multicore graphics rendering on Core 1
//
// This module runs all graphics operations on Core 1, allowing Core 0
// to continue with application logic while graphics render asynchronously.
// This also enables safe use of fully async DMA since buffer lifetime
// is managed entirely by Core 1.
//

#include "gfx_core.h"
#include "gfx.h"
#include "pico/multicore.h"
#include "pico/mutex.h"
#include <string.h>

// Command queue (using Pico SDK FIFO for inter-core communication)
#define GFX_QUEUE_SIZE 32

static mutex_t gfx_mutex;
static volatile bool gfx_core_running = false;
static volatile bool gfx_core_busy = false;

// Core 1 main loop
static void gfx_core1_main(void) {
    gfx_core_running = true;

    while (gfx_core_running) {
        // Check if there's a command in the FIFO
        if (multicore_fifo_rvalid()) {
            uint32_t cmd_ptr = multicore_fifo_pop_blocking();

            if (cmd_ptr == 0) {
                // Shutdown command
                gfx_core_running = false;
                break;
            }

            gfx_core_busy = true;

            gfx_command_t *cmd = (gfx_command_t *)cmd_ptr;

            // Execute command on core 1
            switch (cmd->type) {
                case GFX_CMD_INIT:
                    gfx_init(cmd->data.init.tilesheet, cmd->data.init.tiles_count);
                    break;

                case GFX_CMD_PRESENT:
                    gfx_present();
                    break;

                case GFX_CMD_SET_TILE:
                    gfx_set_tile(cmd->data.set_tile.x,
                                cmd->data.set_tile.y,
                                cmd->data.set_tile.tile_index);
                    break;

                case GFX_CMD_CLEAR:
                    gfx_clear_backmap(cmd->data.clear.bg_tile);
                    break;

                case GFX_CMD_CREATE_SPRITE: {
                    int sprite_id = gfx_create_sprite(
                        cmd->data.create_sprite.image,
                        cmd->data.create_sprite.w,
                        cmd->data.create_sprite.h,
                        cmd->data.create_sprite.x,
                        cmd->data.create_sprite.y,
                        cmd->data.create_sprite.z
                    );
                    // Store result if pointer provided
                    if (cmd->data.create_sprite.result_id) {
                        *cmd->data.create_sprite.result_id = sprite_id;
                    }
                    break;
                }

                case GFX_CMD_MOVE_SPRITE:
                    gfx_move_sprite(cmd->data.move_sprite.sprite_id,
                                   cmd->data.move_sprite.x,
                                   cmd->data.move_sprite.y);
                    break;

                case GFX_CMD_DESTROY_SPRITE:
                    gfx_destroy_sprite(cmd->data.destroy_sprite.sprite_id);
                    break;

                case GFX_CMD_DRAW_SPRITE:
                    // This command type is unused - sprites are drawn in gfx_present()
                    break;

                case GFX_CMD_SHUTDOWN:
                    gfx_core_running = false;
                    break;

                default:
                    break;
            }

            gfx_core_busy = false;

            // Signal completion back to core 0
            multicore_fifo_push_blocking(1); // ACK
        }

        tight_loop_contents();
    }
}

// Initialize the graphics core system
void gfx_core_init(void) {
    mutex_init(&gfx_mutex);

    // Launch core 1 with graphics loop
    multicore_launch_core1(gfx_core1_main);

    // Wait a bit for core 1 to start
    sleep_ms(10);
}

// Send a command to the graphics core (blocking - waits for completion)
bool gfx_core_send_command(const gfx_command_t *cmd) {
    if (!gfx_core_running) {
        return false;
    }

    // Send command pointer via FIFO
    multicore_fifo_push_blocking((uint32_t)cmd);

    // Wait for ACK (blocking until command completes)
    multicore_fifo_pop_blocking();

    return true;
}

// Wait for graphics core to finish processing (blocking)
void gfx_core_wait_idle(void) {
    while (gfx_core_busy) {
        tight_loop_contents();
    }
}

// Check if graphics core is busy
bool gfx_core_is_busy(void) {
    return gfx_core_busy;
}

//
// High-level API wrappers
//

void gfx_core_gfx_init(const uint16_t *tilesheet_ptr, uint16_t tiles_count) {
    gfx_command_t cmd = {
        .type = GFX_CMD_INIT,
        .data.init = {
            .tilesheet = tilesheet_ptr,
            .tiles_count = tiles_count
        }
    };
    gfx_core_send_command(&cmd);
}

void gfx_core_gfx_present(void) {
    gfx_command_t cmd = {
        .type = GFX_CMD_PRESENT
    };
    gfx_core_send_command(&cmd);
}

void gfx_core_gfx_set_tile(uint16_t x, uint16_t y, uint16_t tile_index) {
    gfx_command_t cmd = {
        .type = GFX_CMD_SET_TILE,
        .data.set_tile = {
            .x = x,
            .y = y,
            .tile_index = tile_index
        }
    };
    gfx_core_send_command(&cmd);
}

void gfx_core_gfx_clear_backmap(uint16_t bg_tile) {
    gfx_command_t cmd = {
        .type = GFX_CMD_CLEAR,
        .data.clear = {
            .bg_tile = bg_tile
        }
    };
    gfx_core_send_command(&cmd);
}

int gfx_core_gfx_create_sprite(const uint16_t *image, uint8_t w, uint8_t h, int16_t x, int16_t y, uint8_t z) {
    int sprite_id = -1;
    gfx_command_t cmd = {
        .type = GFX_CMD_CREATE_SPRITE,
        .data.create_sprite = {
            .image = image,
            .w = w,
            .h = h,
            .x = x,
            .y = y,
            .z = z,
            .result_id = &sprite_id
        }
    };
    gfx_core_send_command(&cmd);
    return sprite_id;
}

void gfx_core_gfx_move_sprite(int sprite_id, int16_t x, int16_t y) {
    gfx_command_t cmd = {
        .type = GFX_CMD_MOVE_SPRITE,
        .data.move_sprite = {
            .sprite_id = sprite_id,
            .x = x,
            .y = y
        }
    };
    gfx_core_send_command(&cmd);
}

void gfx_core_gfx_destroy_sprite(int sprite_id) {
    gfx_command_t cmd = {
        .type = GFX_CMD_DESTROY_SPRITE,
        .data.destroy_sprite = {
            .sprite_id = sprite_id
        }
    };
    gfx_core_send_command(&cmd);
}
