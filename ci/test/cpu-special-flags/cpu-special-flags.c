/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdio.h>

#include "core/wz_machine.h"

typedef struct {
    const char* name;
    wz_byte_t program[4];
    size_t length;
    unsigned steps;
    wz_byte_t initial_a;
    wz_byte_t initial_f;
    wz_byte_t expected_a;
    wz_byte_t expected_f;
    wz_master_tick_t expected_tick;
} case_t;

static bool run_case(const case_t* test)
{
    wz_machine_t machine;
    bool passed = false;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        (void)fprintf(stderr, "%s: machine init failed\n", test->name);
        return false;
    }
    for (size_t index = 0u; index < test->length; ++index) {
        wz_machine_memory_write(&machine, (wz_word_t)(0x8000u + index),
                                test->program[index]);
    }
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.main.a = test->initial_a;
    machine.cpu.main.f = test->initial_f;
    for (unsigned index = 0u; index < test->steps; ++index) {
        if (wz_z80_step(&machine) != WZ_RESULT_OK) {
            (void)fprintf(stderr, "%s: step %u failed\n", test->name, index);
            goto cleanup;
        }
    }
    if (machine.cpu.program_counter != (wz_word_t)(0x8000u + test->length) ||
        machine.master_tick != test->expected_tick ||
        machine.cpu.r != (wz_byte_t)test->steps ||
        machine.cpu.main.a != test->expected_a ||
        machine.cpu.main.f != test->expected_f) {
        (void)fprintf(stderr,
            "%s: A/F=%02x/%02x expected=%02x/%02x PC=%04x tick=%llu\n",
            test->name, machine.cpu.main.a, machine.cpu.main.f,
            test->expected_a, test->expected_f,
            machine.cpu.program_counter,
            (unsigned long long)machine.master_tick);
        goto cleanup;
    }
    passed = true;
cleanup:
    wz_machine_destroy(&machine);
    return passed;
}

int main(void)
{
    static const case_t cases[] = {
        {"DAA after add", {0x27u}, 1u, 1u, 0x9au, 0x00u, 0x00u, 0x55u, 8u},
        {"DAA after subtract", {0x27u}, 1u, 1u, 0x15u, 0x12u, 0x0fu, 0x1eu, 8u},
        {"CPL", {0x2fu}, 1u, 1u, 0x00u, 0xc5u, 0xffu, 0xffu, 8u},
        {"SCF with prior Q clear", {0x37u}, 1u, 1u, 0x00u, 0xffu,
         0x00u, 0xedu, 8u},
        {"CCF with prior Q clear", {0x3fu}, 1u, 1u, 0x00u, 0xffu,
         0x00u, 0xfcu, 8u},
        {"CP then SCF with prior Q set", {0xfeu,0x28u,0x37u}, 3u, 2u,
         0x00u, 0x00u, 0x00u, 0x81u, 22u},
        {"CP then CCF with prior Q set", {0xfeu,0x28u,0x3fu}, 3u, 2u,
         0x00u, 0x00u, 0x00u, 0x90u, 22u},
        {"CP NOP SCF with prior Q clear", {0xfeu,0x28u,0x00u,0x37u},
         4u, 3u, 0x00u, 0x00u, 0x00u, 0xa9u, 30u}
    };
    for (size_t index = 0u; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        if (!run_case(&cases[index])) return 1;
    }
    (void)puts("{\"testId\":\"cpu-special-flags\",\"status\":\"pass\","
               "\"cases\":8,\"qSensitiveCases\":3}");
    return 0;
}
