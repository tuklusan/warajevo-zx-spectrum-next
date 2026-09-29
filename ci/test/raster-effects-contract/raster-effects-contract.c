/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"

static bool check_timed_raster_effects(void)
{
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    const size_t raster_size = (size_t)WZ_RASTER_CANONICAL_WIDTH *
        WZ_RASTER_CANONICAL_HEIGHT;
    wz_machine_t machine;
    wz_raster_buffer_t raster;
    wz_byte_t* pixels = (wz_byte_t*)malloc(raster_size);
    wz_master_tick_t ticks_per_tstate;
    wz_master_tick_t line_ticks;
    wz_master_tick_t first_attribute_tick;
    wz_master_tick_t frame_ticks;
    static const wz_byte_t attributes[8] = {
        0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x41u
    };
    bool success = false;

    memset(&machine, 0, sizeof(machine));
    memset(&raster, 0, sizeof(raster));
    if (profile == 0 || pixels == 0 ||
        wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        free(pixels);
        return false;
    }

    ticks_per_tstate = profile->master_ticks_per_cpu_tstate;
    line_ticks = (wz_master_tick_t)profile->tstates_per_line * ticks_per_tstate;
    frame_ticks = (wz_master_tick_t)profile->tstates_per_frame * ticks_per_tstate;
    first_attribute_tick =
        ((wz_master_tick_t)profile->ula_fetch_start_tstate +
         profile->ula_attribute_offset_tstates) * ticks_per_tstate;

    /* Change one ordinary Spectrum attribute at each timed ULA attribute fetch.
     * This creates scanline-level multicolor from bus timing and memory alone. */
    for (wz_dword_t row = 0u; row < 8u; ++row) {
        wz_word_t bitmap_address = (wz_word_t)(0x4000u + row * 0x100u);
        wz_master_tick_t attribute_tick = first_attribute_tick +
            (wz_master_tick_t)row * line_ticks;
        wz_machine_memory_write(&machine, bitmap_address, 0xffu);
        if (wz_machine_memory_write_at_tick(&machine, 0x5800u,
                attributes[row], attribute_tick) != WZ_RESULT_OK) {
            goto cleanup;
        }
    }

    /* Border changes at adjacent raster clocks yield a rainbow edge without
     * software recognition or a presentation-layer effect. */
    {
        const wz_master_tick_t line = (wz_master_tick_t)10u * line_ticks;
        wz_machine_ula_port_fe_write(&machine, 0x00feu, 1u, line + 1u);
        wz_machine_ula_port_fe_write(&machine, 0x00feu, 2u, line + 2u);
        wz_machine_ula_port_fe_write(&machine, 0x00feu, 3u, line + 3u);
    }

    machine.master_tick = frame_ticks;
    if (wz_raster_buffer_init(&raster, WZ_RASTER_CANONICAL_WIDTH,
            WZ_RASTER_CANONICAL_HEIGHT, pixels, raster_size) != WZ_RESULT_OK ||
        wz_machine_render_raster(&machine, &raster) != WZ_RESULT_OK) {
        goto cleanup;
    }

    for (size_t row = 0u; row < 8u; ++row) {
        wz_byte_t expected = (wz_byte_t)((attributes[row] & 0x07u) +
            ((attributes[row] & 0x40u) != 0u ? 8u : 0u));
        size_t pixel = (64u + row) * WZ_RASTER_CANONICAL_WIDTH + 96u;
        if (pixels[pixel] != expected) {
            fprintf(stderr, "timed attribute row %zu: expected %u got %u\n",
                    row, (unsigned)expected, (unsigned)pixels[pixel]);
            goto cleanup;
        }
    }
    if (pixels[10u * WZ_RASTER_CANONICAL_WIDTH + 97u] != 0x11u ||
        pixels[10u * WZ_RASTER_CANONICAL_WIDTH + 98u] != 0x12u ||
        pixels[10u * WZ_RASTER_CANONICAL_WIDTH + 99u] != 0x13u) {
        fputs("timestamped border colors did not render at raster clocks\n",
              stderr);
        goto cleanup;
    }

    puts("PASS timed_attribute_multicolor_and_border_rainbow");
    success = true;

cleanup:
    wz_machine_destroy(&machine);
    free(pixels);
    return success;
}

int main(void)
{
    return check_timed_raster_effects() ? 0 : 1;
}
