/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_host_scaling.h"
#include "app/wz_host_config.h"

#include <stdio.h>
#include <stdlib.h>
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
    char resolved_path[1024];
    const char settings_path[] = "host-settings-contract.v1";
    FILE* settings_file;

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
    REQUIRE(wz_host_preferences_path(resolved_path, sizeof(resolved_path)));
    REQUIRE(strstr(resolved_path, "host-settings.v1") != NULL);
    (void)remove(settings_path);
    REQUIRE(wz_host_preferences_load(settings_path, &preferences));
    REQUIRE(preferences.window_width == 0);
    REQUIRE(preferences.audio_enabled);
    REQUIRE(preferences.display_scale == 0u);
    REQUIRE(!preferences.crop_to_display);
    preferences.display_scale = 5u;
    REQUIRE(!wz_host_preferences_serialize(&preferences, serialized,
        sizeof(serialized), &serialized_size));
    preferences.display_scale = 0u;
    REQUIRE(wz_host_preferences_serialize(&preferences, serialized,
        sizeof(serialized), &serialized_size));
    REQUIRE(serialized_size > 0u);
    REQUIRE(serialized_size < sizeof(serialized));
    REQUIRE(strstr(serialized, "display_scale=0\n") != NULL);
    REQUIRE(strstr(serialized, "crop_to_display=0\n") != NULL);
    REQUIRE(strstr(serialized, "version=1\n") != NULL);
    preferences.window_width = 1280;
    preferences.window_height = 800;
    preferences.fullscreen = true;
    preferences.display_scale = 3u;
    preferences.crop_to_display = true;
    preferences.audio_enabled = false;
    preferences.status_panel_visible = false;
    REQUIRE(wz_host_preferences_write(settings_path, &preferences));
    wz_host_preferences_init(&preferences);
    REQUIRE(wz_host_preferences_load(settings_path, &preferences));
    REQUIRE(preferences.window_width == 1280);
    REQUIRE(preferences.window_height == 800);
    REQUIRE(preferences.fullscreen);
    REQUIRE(preferences.display_scale == 3u);
    REQUIRE(preferences.crop_to_display);
    REQUIRE(!preferences.audio_enabled);
    REQUIRE(!preferences.status_panel_visible);
    settings_file = fopen(settings_path, "wb");
    REQUIRE(settings_file != NULL);
    REQUIRE(fputs("version=1\nwindow_width=-1\n", settings_file) >= 0);
    REQUIRE(fclose(settings_file) == 0);
    REQUIRE(!wz_host_preferences_load(settings_path, &preferences));
    REQUIRE(preferences.window_width == 1280);
    REQUIRE(remove(settings_path) == 0);

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
