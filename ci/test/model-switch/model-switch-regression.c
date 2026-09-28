/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/wz_machine.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "model switch regression failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_machine_t machine;
    wz_machine_profile_t invalid_profile;
    wz_tape_segment_t segment = {1000u, 1u, 0u};
    wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE];

    memset(&machine, 0, sizeof(machine));
    memset(interface1_rom, 0x5au, sizeof(interface1_rom));
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    wz_machine_memory_write(&machine, 0x8000u, 0xa5u);
    REQUIRE(wz_machine_mount_tape(&machine, &segment, 1u) == WZ_RESULT_OK);
    REQUIRE(wz_machine_set_tape_motor(&machine, true) == WZ_RESULT_OK);
    REQUIRE(wz_machine_advance_tape(&machine, 37u) == WZ_RESULT_OK);
    REQUIRE(wz_machine_load_interface1_rom(
                &machine, interface1_rom, sizeof(interface1_rom),
                WZ_INTERFACE1_ROM_NEW) == WZ_RESULT_OK);
    REQUIRE(wz_machine_set_networking_mode(
                &machine, WZ_NETWORKING_INTERFACE1) == WZ_RESULT_OK);
    REQUIRE(wz_machine_set_interface1_rom_page(&machine, 1u) ==
            WZ_RESULT_OK);

    REQUIRE(wz_machine_reconfigure_profile(
                &machine, wz_machine_profile_128k_pal()) == WZ_RESULT_OK);
    REQUIRE(machine.profile->kind == WZ_MACHINE_128K_PAL);
    REQUIRE(machine.cpu.program_counter == 0u);
    REQUIRE(wz_machine_memory_read(&machine, 0x8000u) == 0u);
    REQUIRE(machine.tape_mounted != 0u);
    REQUIRE(machine.tape_state.tape == &machine.tape);
    REQUIRE(machine.tape_state.segment_elapsed == 37u);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_INTERFACE1);
    REQUIRE(wz_machine_memory_read(&machine, 0x0001u) == 0x5au);

    REQUIRE(wz_machine_reconfigure_profile(
                &machine, wz_machine_profile_48k_pal()) == WZ_RESULT_OK);
    REQUIRE(machine.profile->kind == WZ_MACHINE_48K_PAL);
    REQUIRE(wz_machine_memory_read(&machine, 0x8000u) == 0u);
    REQUIRE(machine.tape_state.tape == &machine.tape);
    REQUIRE(machine.tape_state.segment_elapsed == 37u);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_INTERFACE1);
    REQUIRE(wz_machine_memory_read(&machine, 0x0001u) == 0x5au);

    invalid_profile = *wz_machine_profile_48k_pal();
    invalid_profile.kind = (wz_machine_kind_t)99;
    REQUIRE(wz_machine_reconfigure_profile(&machine, &invalid_profile) ==
            WZ_RESULT_INVALID_PROFILE);
    REQUIRE(machine.profile == wz_machine_profile_48k_pal());
    REQUIRE(machine.tape_state.tape == &machine.tape);

    wz_machine_destroy(&machine);
    puts("model switch regression passed");
    return 0;
}
