/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/audio/wz_ay.h"

#include <stdio.h>

static unsigned assertions;

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "AY timing contract failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
    ++assertions; \
} while (0)

static int write_register(wz_ay_t* ay, wz_byte_t reg, wz_byte_t value,
                          wz_master_tick_t tick)
{
    return wz_ay_select_register(ay, reg, tick) == WZ_RESULT_OK &&
        wz_ay_write_data(ay, value, tick) == WZ_RESULT_OK;
}

int main(void)
{
    wz_ay_t ay;
    wz_ay_event_t events[8];
    size_t event_count;
    wz_dword_t initial_noise;

    wz_ay_init(&ay);
    REQUIRE(write_register(&ay, 0u, 1u, 100u));
    REQUIRE(write_register(&ay, 1u, 0u, 101u));
    REQUIRE(wz_ay_tone_period(&ay, 0u) == 1u);
    REQUIRE(wz_ay_tone_period(&ay, 3u) == 0u);
    event_count = wz_ay_events(&ay, events, 8u);
    REQUIRE(event_count == 4u);
    REQUIRE(events[0].kind == WZ_AY_EVENT_REGISTER_SELECT &&
            events[0].master_tick == 100u && events[0].register_index == 0u);
    REQUIRE(events[1].kind == WZ_AY_EVENT_REGISTER_WRITE &&
            events[1].master_tick == 100u && events[1].value == 1u);
    REQUIRE(events[2].kind == WZ_AY_EVENT_REGISTER_SELECT &&
            events[2].master_tick == 101u && events[2].register_index == 1u);
    REQUIRE(events[3].kind == WZ_AY_EVENT_REGISTER_WRITE &&
            events[3].master_tick == 101u && events[3].value == 0u);

    wz_ay_init(&ay);
    REQUIRE(write_register(&ay, 0u, 1u, 0u));
    REQUIRE(wz_ay_advance_master_ticks(&ay, 31u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_tone_level(&ay, 0u) == 0u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 1u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_tone_level(&ay, 0u) == 1u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 32u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_tone_level(&ay, 0u) == 0u);

    wz_ay_init(&ay);
    REQUIRE(write_register(&ay, 0u, 2u, 0u));
    REQUIRE(wz_ay_advance_master_ticks(&ay, 63u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_tone_level(&ay, 0u) == 0u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 1u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_tone_level(&ay, 0u) == 1u);

    wz_ay_init(&ay);
    REQUIRE(write_register(&ay, 6u, 1u, 0u));
    initial_noise = ay.noise_lfsr;
    REQUIRE(wz_ay_noise_period(&ay) == 1u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 63u) == WZ_RESULT_OK);
    REQUIRE(ay.noise_lfsr == initial_noise);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 1u) == WZ_RESULT_OK);
    REQUIRE(ay.noise_lfsr != initial_noise && ay.noise_lfsr != 0u);

    wz_ay_init(&ay);
    REQUIRE(write_register(&ay, 6u, 2u, 0u));
    initial_noise = ay.noise_lfsr;
    REQUIRE(wz_ay_advance_master_ticks(&ay, 127u) == WZ_RESULT_OK);
    REQUIRE(ay.noise_lfsr == initial_noise);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 1u) == WZ_RESULT_OK);
    REQUIRE(ay.noise_lfsr != initial_noise);

    wz_ay_init(&ay);
    REQUIRE(write_register(&ay, 11u, 1u, 0u));
    REQUIRE(write_register(&ay, 13u, 0x0eu, 0u));
    REQUIRE(wz_ay_envelope_period(&ay) == 1u);
    REQUIRE(wz_ay_envelope_level(&ay) == 0u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 1023u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_envelope_level(&ay) == 0u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 1u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_envelope_level(&ay) == 1u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 14u * 1024u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_envelope_level(&ay) == 15u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 1024u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_envelope_level(&ay) == 14u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 14u * 1024u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_envelope_level(&ay) == 0u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 1024u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_envelope_level(&ay) == 1u);

    wz_ay_init(&ay);
    REQUIRE(write_register(&ay, 11u, 2u, 0u));
    REQUIRE(write_register(&ay, 13u, 0x0cu, 0u));
    REQUIRE(wz_ay_envelope_period(&ay) == 2u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 2047u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_envelope_level(&ay) == 0u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 1u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_envelope_level(&ay) == 1u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 14u * 2048u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_envelope_level(&ay) == 15u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 2048u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_envelope_level(&ay) == 0u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 2048u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_envelope_level(&ay) == 1u);

    wz_ay_init(&ay);
    REQUIRE(write_register(&ay, 13u, 0x09u, 0u));
    REQUIRE(wz_ay_envelope_level(&ay) == 15u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 15u * 1024u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_envelope_level(&ay) == 0u);
    REQUIRE(wz_ay_advance_master_ticks(&ay, 64u * 1024u) == WZ_RESULT_OK);
    REQUIRE(wz_ay_envelope_level(&ay) == 0u);

    printf("AY timing contract passed (%u assertions)\n", assertions);
    return 0;
}
