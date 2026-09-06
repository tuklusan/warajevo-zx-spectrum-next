/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>

#include "core/wz_machine.h"

static int check_profile(const wz_machine_profile_t* profile)
{
    wz_machine_t machine;
    wz_byte_t storage[WZ_RASTER_CANONICAL_WIDTH * WZ_RASTER_CANONICAL_HEIGHT];
    wz_raster_buffer_t raster = {storage, WZ_RASTER_CANONICAL_WIDTH,
                                 WZ_RASTER_CANONICAL_HEIGHT};
    wz_byte_t sample;
    wz_master_tick_t tick;

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        return 1;
    }
    wz_machine_memory_write(&machine, 0x4000u, 0x80u);
    wz_machine_memory_write(&machine, 0x5800u, 0x47u);
    wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x03u, 99u);
    tick = machine.master_tick;
    if (wz_machine_render_raster(&machine, &raster) != WZ_RESULT_OK ||
        wz_raster_buffer_read(&raster, 0u, 0u, &sample) != WZ_RESULT_OK ||
        sample != 0x13u ||
        wz_raster_buffer_read(&raster, 96u, 60u, &sample) != WZ_RESULT_OK ||
        sample != 15u ||
        wz_raster_buffer_read(&raster, 97u, 60u, &sample) != WZ_RESULT_OK ||
        sample != 8u || machine.master_tick != tick ||
        machine.border_color != 3u) {
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    return 0;
}

int main(void)
{
    if (check_profile(wz_machine_profile_48k_pal()) != 0 ||
        check_profile(wz_machine_profile_128k_pal()) != 0) {
        fputs("screen inspection projection contract failed\n", stderr);
        return 1;
    }
    puts("wz_screen_inspection projection contract passed");
    return 0;
}
