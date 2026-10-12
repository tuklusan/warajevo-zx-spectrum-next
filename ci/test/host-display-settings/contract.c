/*
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_host_scaling.h"
#include "app/wz_host_config.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "display settings contract failed at line %d\n", \
                __LINE__); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_host_display_settings_t settings;
    wz_host_preferences_t preferences;
    wz_host_presentation_rect_t source;
    wz_host_scale_t scaled = {0u, 0u, 0u};
    size_t source_x = 0u;
    size_t source_y = 0u;
    char serialized[512];
    size_t serialized_size = 0u;

    wz_host_display_settings_init(&settings);
    REQUIRE(wz_host_display_settings_scale(&settings) == 0u);
    REQUIRE(!wz_host_display_settings_crop(&settings));
    for (size_t factor = 1u; factor <= 4u; ++factor) {
        REQUIRE(wz_host_display_settings_set_scale(&settings, factor) ==
                WZ_RESULT_OK);
        REQUIRE(wz_host_display_settings_scale(&settings) == factor);
    }
    REQUIRE(wz_host_display_settings_set_scale(&settings, 5u) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_host_display_settings_scale(&settings) == 4u);
    REQUIRE(wz_host_display_settings_set_scale(&settings, 0u) ==
            WZ_RESULT_OK);
    REQUIRE(wz_host_display_settings_scale(&settings) == 0u);
    REQUIRE(wz_host_display_settings_set_crop(&settings, true) == WZ_RESULT_OK);
    REQUIRE(wz_host_display_settings_crop(&settings));
    REQUIRE(wz_host_display_settings_set_crop(&settings, false) == WZ_RESULT_OK);
    REQUIRE(!wz_host_display_settings_crop(&settings));

    wz_host_preferences_init(&preferences);
    REQUIRE(preferences.display_scale == 0u);
    REQUIRE(!preferences.crop_to_display);
    REQUIRE(wz_host_preferences_serialize(&preferences, serialized,
        sizeof(serialized), &serialized_size));
    REQUIRE(serialized_size > 0u);
    REQUIRE(serialized_size < sizeof(serialized));
    REQUIRE(strstr(serialized, "display_scale=0\n") != NULL);
    REQUIRE(strstr(serialized, "crop_to_display=0\n") != NULL);

    source = wz_host_presentation_default_rect();
    REQUIRE(source.width == WZ_RASTER_CANONICAL_WIDTH);
    REQUIRE(source.height == WZ_RASTER_CANONICAL_HEIGHT);
    REQUIRE(wz_host_scale_integer(&scaled, &source, 2u) == WZ_RESULT_OK);
    REQUIRE(scaled.width == 896u && scaled.height == 624u);
    REQUIRE(wz_host_scale_source_pixel(&scaled, 17u, 31u,
                                       &source_x, &source_y) == WZ_RESULT_OK);
    REQUIRE(source_x == 8u && source_y == 15u);
    REQUIRE(wz_host_scale_source_pixel(&scaled, scaled.width, 0u,
                                       &source_x, &source_y) ==
            WZ_RESULT_INVALID_ARGUMENT);
    puts("host display settings contract passed");
    return 0;
}
