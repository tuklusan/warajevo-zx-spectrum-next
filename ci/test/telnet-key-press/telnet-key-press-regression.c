/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_telnet_key_press.h"

#include <stdio.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "Telnet key press regression failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_input_arbiter_t arbiter;
    wz_telnet_key_press_state_t presses;
    const size_t key = 17u;

    wz_input_arbiter_init(&arbiter);
    wz_telnet_key_press_state_init(&presses);
    REQUIRE(wz_input_arbiter_set(&arbiter, WZ_INPUT_SOURCE_LOCAL,
                                 key, true));
    REQUIRE(wz_telnet_key_press_schedule(&presses, &arbiter, key, 100u, 10u));
    REQUIRE(wz_telnet_key_press_pending(&presses, key));
    REQUIRE(!wz_telnet_key_press_schedule(&presses, &arbiter, key, 100u, 10u));

    /* An emulated Spectrum reset moves master_tick backwards to zero. */
    REQUIRE(wz_telnet_key_press_drain(&presses, &arbiter, 0u) == 0u);
    REQUIRE(wz_telnet_key_press_pending(&presses, key));
    REQUIRE(wz_input_arbiter_source_key_down(
        &arbiter, WZ_INPUT_SOURCE_TELNET, key));
    REQUIRE(wz_telnet_key_press_drain(&presses, &arbiter, 19u) == 0u);
    REQUIRE(wz_input_arbiter_key_down(&arbiter, key));
    REQUIRE(wz_telnet_key_press_drain(&presses, &arbiter, 20u) == 1u);
    REQUIRE(!wz_telnet_key_press_pending(&presses, key));
    REQUIRE(!wz_input_arbiter_source_key_down(
        &arbiter, WZ_INPUT_SOURCE_TELNET, key));
    REQUIRE(wz_input_arbiter_key_down(&arbiter, key));
    REQUIRE(wz_input_arbiter_set(&arbiter, WZ_INPUT_SOURCE_LOCAL,
                                 key, false));
    REQUIRE(!wz_input_arbiter_key_down(&arbiter, key));

    puts("KEY PRESS deadline survives emulated reset and preserves local ownership");
    return 0;
}
