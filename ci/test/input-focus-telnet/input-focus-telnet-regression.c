/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_input_focus.h"
#include "app/wz_telnet_input.h"

#include <stdio.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "input focus/Telnet regression failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_input_arbiter_t arbiter;
    wz_input_focus_controller_t focus;
    const size_t key = 17u;

    wz_input_arbiter_init(&arbiter);
    wz_input_focus_init(&focus, &arbiter);
    REQUIRE(wz_telnet_input_set_key(&arbiter, key, true));
    REQUIRE(wz_input_arbiter_key_down(&arbiter, key));
    REQUIRE(wz_input_focus_set_target(&focus, WZ_INPUT_FOCUS_TEXT_CONTROL));
    REQUIRE(wz_input_arbiter_key_down(&arbiter, key));
    REQUIRE(wz_input_arbiter_source_key_down(
        &arbiter, WZ_INPUT_SOURCE_TELNET, key));

    REQUIRE(wz_input_arbiter_set(&arbiter, WZ_INPUT_SOURCE_LOCAL, key, true));
    REQUIRE(wz_input_focus_lost(&focus));
    REQUIRE(!wz_input_arbiter_source_key_down(
        &arbiter, WZ_INPUT_SOURCE_LOCAL, key));
    REQUIRE(wz_input_arbiter_key_down(&arbiter, key));
    REQUIRE(wz_input_arbiter_source_key_down(
        &arbiter, WZ_INPUT_SOURCE_TELNET, key));

    REQUIRE(wz_input_focus_gained(&focus));
    REQUIRE(wz_input_focus_dialog_leave(&focus));
    REQUIRE(wz_input_arbiter_key_down(&arbiter, key));
    REQUIRE(wz_telnet_input_release_all(&arbiter));
    REQUIRE(!wz_input_arbiter_key_down(&arbiter, key));
    REQUIRE(wz_input_focus_forwards_viewport_keys(&focus));

    puts("GUI focus preserves independent Telnet key ownership");
    return 0;
}
