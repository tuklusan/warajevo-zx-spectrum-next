/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdbool.h>
#include <stdio.h>

#include "app/wz_host_presentation.h"
#include "app/wz_host_scaling.h"

int main(void)
{
    wz_host_presentation_state_t state;
    wz_host_display_settings_t settings;

    wz_host_presentation_state_init(&state);
    if (wz_host_presentation_is_fullscreen(&state) ||
        wz_host_presentation_set_fullscreen(&state, true) != WZ_RESULT_OK ||
        !wz_host_presentation_is_fullscreen(&state) ||
        wz_host_presentation_set_fullscreen(&state, false) != WZ_RESULT_OK ||
        wz_host_presentation_is_fullscreen(&state) ||
        wz_host_presentation_set_fullscreen(NULL, true) !=
            WZ_RESULT_INVALID_ARGUMENT) {
        fputs("host fullscreen presentation contract failed\n", stderr);
        return 1;
    }

    wz_host_display_settings_init(&settings);
    if (wz_host_display_settings_scale(&settings) != 1u ||
        wz_host_display_settings_crop(&settings) ||
        wz_host_display_settings_set_scale(&settings, 0u) !=
            WZ_RESULT_INVALID_ARGUMENT ||
        wz_host_display_settings_set_scale(&settings, 3u) != WZ_RESULT_OK ||
        wz_host_display_settings_scale(&settings) != 3u ||
        wz_host_display_settings_set_crop(&settings, true) != WZ_RESULT_OK ||
        !wz_host_display_settings_crop(&settings) ||
        wz_host_display_settings_set_crop(NULL, false) !=
            WZ_RESULT_INVALID_ARGUMENT) {
        fputs("host display settings contract failed\n", stderr);
        return 1;
    }

    puts("host fullscreen presentation contract passed");
    return 0;
}
