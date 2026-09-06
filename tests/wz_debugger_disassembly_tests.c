/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>
#include "core/wz_debugger.h"

static int check(const wz_machine_t* machine, wz_word_t address,
                 const char* expected, size_t length)
{
    char text[64];
    size_t consumed = 0u;
    if (wz_debugger_disassemble(machine, address, text, sizeof(text), &consumed) != WZ_RESULT_OK ||
        consumed != length || strcmp(text, expected) != 0) return 0;
    return 1;
}

int main(void)
{
    wz_machine_t machine;
    char text[8];
    char short_text[4];
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) return 1;
    machine.memory[0x5000u] = 0x00u;
    machine.memory[0x5001u] = 0x3eu;
    machine.memory[0x5002u] = 0x7fu;
    machine.memory[0x5003u] = 0xcbu;
    machine.memory[0x5004u] = 0x46u;
    machine.memory[0x5005u] = 0xedu;
    machine.memory[0x5006u] = 0x44u;
    machine.memory[0x5007u] = 0xddu;
    machine.memory[0x5008u] = 0x21u;
    machine.memory[0x5009u] = 0x34u;
    machine.memory[0x500au] = 0x12u;
    if (!check(&machine, 0x5000u, "NOP", 1u) ||
        !check(&machine, 0x5001u, "LD A,$7F", 2u) ||
        !check(&machine, 0x5003u, "BIT 0,(HL)", 2u) ||
        !check(&machine, 0x5005u, "NEG", 2u) ||
        !check(&machine, 0x5007u, "LD IX,$1234", 4u) ||
        wz_debugger_format_hex8(0xa5u, text, sizeof(text)) != WZ_RESULT_OK ||
        strcmp(text, "A5") != 0 ||
        wz_debugger_format_hex16(0x1234u, short_text, sizeof(short_text)) != WZ_RESULT_BUFFER_TOO_SMALL) {
        fputs("debugger disassembly contract failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    puts("wz_debugger_disassembly contract passed");
    return 0;
}
