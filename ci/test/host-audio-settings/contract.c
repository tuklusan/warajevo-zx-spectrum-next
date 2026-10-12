/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_host_audio_policy.h"
#include "app/wz_host_config.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    wz_speed_policy_t speed;
    bool speed_allows_output;
} audio_case_t;

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "host audio settings contract failed at line %d\n", \
                __LINE__); \
        return 1; \
    } \
} while (0)

int main(void)
{
    static const audio_case_t cases[] = {
        {WZ_SPEED_25, false},
        {WZ_SPEED_50, true},
        {WZ_SPEED_100, true},
        {WZ_SPEED_200, true},
        {WZ_SPEED_400, false},
        {WZ_SPEED_800, false},
        {WZ_SPEED_UNLIMITED, false},
        {(wz_speed_policy_t)WZ_SPEED_COUNT, false}
    };
    wz_host_preferences_t preferences;
    char serialized[512];
    size_t serialized_size = 0u;

    for (size_t index = 0u; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        REQUIRE(wz_host_audio_enabled(cases[index].speed) ==
                cases[index].speed_allows_output);
        REQUIRE(wz_host_audio_output_enabled(cases[index].speed, true) ==
                cases[index].speed_allows_output);
        REQUIRE(!wz_host_audio_output_enabled(cases[index].speed, false));
    }

    wz_host_preferences_init(&preferences);
    REQUIRE(preferences.audio_enabled);
    REQUIRE(wz_host_preferences_serialize(&preferences, serialized,
        sizeof(serialized), &serialized_size));
    REQUIRE(serialized_size < sizeof(serialized));
    REQUIRE(strstr(serialized, "audio_enabled=1\n") != NULL);
    preferences.audio_enabled = false;
    REQUIRE(wz_host_preferences_serialize(&preferences, serialized,
        sizeof(serialized), &serialized_size));
    REQUIRE(strstr(serialized, "audio_enabled=0\n") != NULL);
    puts("host audio settings contract passed");
    return 0;
}
