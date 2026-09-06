/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "app/wz_diagnostic_block.h"
#include "core/wz_state.h"

static int verify_profile(const wz_machine_profile_t* profile)
{
    wz_machine_t machine;
    wz_byte_t block[WZ_STATE_SNAPSHOT_CAPACITY];
    wz_byte_t invalid[WZ_STATE_SNAPSHOT_CAPACITY];
    size_t length = 0u;
    size_t block_length;
    wz_byte_t preserved;

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) return 1;
    machine.cpu.program_counter = 0x3456u;
    machine.memory[0x8000u] = 0xa5u;
    preserved = machine.memory[0x8000u];
    if (wz_diagnostic_block_save(&machine, block, sizeof(block), &length) !=
            WZ_RESULT_OK ||
        length != WZ_STATE_MACHINE_LENGTH) {
        wz_machine_destroy(&machine);
        return 1;
    }
    block_length = length;
    if (wz_diagnostic_block_save(&machine, block, block_length - 1u, &length) !=
        WZ_RESULT_BUFFER_TOO_SMALL) {
        wz_machine_destroy(&machine);
        return 1;
    }
    machine.cpu.program_counter = 0x789au;
    if (wz_diagnostic_block_load(&machine, block, block_length) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x3456u ||
        machine.memory[0x8000u] != preserved) {
        wz_machine_destroy(&machine);
        return 1;
    }
    memcpy(invalid, block, block_length);
    invalid[0u] ^= 0xffu;
    machine.cpu.program_counter = 0x1111u;
    if (wz_diagnostic_block_load(&machine, invalid, block_length) == WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1111u ||
        wz_diagnostic_block_load(&machine, block, block_length - 1u) !=
            WZ_RESULT_INVALID_STATE) {
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    return 0;
}

int main(void)
{
    if (verify_profile(wz_machine_profile_48k_pal()) != 0 ||
        verify_profile(wz_machine_profile_128k_pal()) != 0) {
        fputs("diagnostic block contract failed\n", stderr);
        return 1;
    }
    puts("wz_diagnostic_block contract passed");
    return 0;
}
