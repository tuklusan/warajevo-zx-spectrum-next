/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "core/wz_machine.h"

enum {
    FLAG_C = 0x01,
    FLAG_N = 0x02,
    FLAG_PV = 0x04,
    FLAG_X = 0x08,
    FLAG_H = 0x10,
    FLAG_Y = 0x20,
    FLAG_Z = 0x40,
    FLAG_S = 0x80
};

typedef enum {
    ADD_HL_BC = 0,
    ADC_HL_BC,
    SBC_HL_BC,
    ADD_IX_BC,
    ADD_IY_BC
} operation_t;

typedef struct {
    wz_word_t value;
    wz_byte_t flags;
} expected_t;

static int signed_word(unsigned value)
{
    return value < 32768u ? (int)value : (int)value - 65536;
}

static expected_t expected_result(operation_t operation, unsigned left,
                                  unsigned right, unsigned carry)
{
    expected_t expected;
    unsigned flags;
    unsigned result;
    if (operation == SBC_HL_BC) {
        const int difference = (int)left - (int)right - (int)carry;
        const int signed_difference = signed_word(left) -
                                      signed_word(right) - (int)carry;
        result = (unsigned)difference & 0xffffu;
        flags = FLAG_N;
        if ((left & 0xfffu) < ((right & 0xfffu) + carry)) flags |= FLAG_H;
        if (signed_difference < -32768 || signed_difference > 32767) flags |= FLAG_PV;
        if (difference < 0) flags |= FLAG_C;
    } else {
        const unsigned input_carry = operation == ADC_HL_BC ? carry : 0u;
        const unsigned wide = left + right + input_carry;
        const int signed_sum = signed_word(left) + signed_word(right) +
                               (int)input_carry;
        result = wide & 0xffffu;
        flags = 0u;
        if (((left & 0xfffu) + (right & 0xfffu) + input_carry) > 0xfffu) {
            flags |= FLAG_H;
        }
        if (wide > 0xffffu) flags |= FLAG_C;
        if (operation == ADC_HL_BC &&
            (signed_sum < -32768 || signed_sum > 32767)) flags |= FLAG_PV;
    }
    flags |= (result >> 8u) & (FLAG_Y | FLAG_X);
    if (operation == ADD_HL_BC || operation == ADD_IX_BC ||
        operation == ADD_IY_BC) {
        flags |= FLAG_S | FLAG_Z | FLAG_PV;
    } else {
        if ((result & 0x8000u) != 0u) flags |= FLAG_S;
        if (result == 0u) flags |= FLAG_Z;
    }
    expected.value = (wz_word_t)result;
    expected.flags = (wz_byte_t)flags;
    return expected;
}

static bool run_case(wz_machine_t* machine, operation_t operation,
                     unsigned left, unsigned right, unsigned carry)
{
    const expected_t expected = expected_result(operation, left, right, carry);
    const wz_master_tick_t before = machine->master_tick;
    const wz_master_tick_t duration = operation == ADD_HL_BC ? 22u : 30u;
    const wz_word_t expected_pc = operation == ADD_HL_BC ? 0x8001u : 0x8002u;
    const wz_word_t expected_hl = operation <= SBC_HL_BC ?
                                  expected.value : (wz_word_t)left;
    const wz_word_t expected_ix = operation == ADD_IX_BC ?
                                  expected.value : (wz_word_t)left;
    const wz_word_t expected_iy = operation == ADD_IY_BC ?
                                  expected.value : (wz_word_t)left;
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.f = (wz_byte_t)(0xfeu | carry);
    machine->cpu.main.b = (wz_byte_t)(right >> 8u);
    machine->cpu.main.c = (wz_byte_t)right;
    machine->cpu.main.h = (wz_byte_t)(left >> 8u);
    machine->cpu.main.l = (wz_byte_t)left;
    machine->cpu.ix = (wz_word_t)left;
    machine->cpu.iy = (wz_word_t)left;
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->cpu.program_counter != expected_pc ||
        machine->master_tick != before + duration ||
        machine->cpu.main.f != expected.flags ||
        (((wz_word_t)machine->cpu.main.h << 8u) |
         machine->cpu.main.l) != expected_hl ||
        machine->cpu.ix != expected_ix || machine->cpu.iy != expected_iy ||
        machine->cpu.memptr != (wz_word_t)(left + 1u)) {
        (void)fprintf(stderr,
            "wide flag mismatch op=%u left=%04x right=%04x carry=%u "
            "actual F=%02x expected F=%02x\n",
            (unsigned)operation, left, right, carry,
            machine->cpu.main.f, expected.flags);
        return false;
    }
    return true;
}

int main(void)
{
    static const wz_word_t edges[] = {
        0x0000u, 0x0001u, 0x000fu, 0x0010u, 0x07ffu,
        0x0800u, 0x0fffu, 0x1000u, 0x7fffu, 0x8000u,
        0x8fffu, 0x9000u, 0xffffu, 0x5555u, 0xaaaau
    };
    static const wz_byte_t programs[][2] = {
        {0x09u, 0x00u}, {0xedu, 0x4au}, {0xedu, 0x42u},
        {0xddu, 0x09u}, {0xfdu, 0x09u}
    };
    wz_machine_t machine;
    bool passed = true;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        (void)fprintf(stderr, "machine init failed\n");
        return 1;
    }
    for (unsigned operation = 0u; operation < 5u && passed; ++operation) {
        wz_machine_memory_write(&machine, 0x8000u, programs[operation][0]);
        wz_machine_memory_write(&machine, 0x8001u, programs[operation][1]);
        for (size_t left = 0u; left < 15u && passed; ++left) {
            for (size_t right = 0u; right < 15u && passed; ++right) {
                for (unsigned carry = 0u; carry <= 1u; ++carry) {
                    if (!run_case(&machine, (operation_t)operation,
                                  edges[left], edges[right], carry)) {
                        passed = false;
                        break;
                    }
                }
            }
        }
    }
    wz_machine_destroy(&machine);
    if (!passed) return 1;
    (void)puts("{\"testId\":\"cpu-wide-flags\",\"status\":\"pass\","
               "\"cases\":2250,\"edgeValues\":15,\"operations\":5}");
    return 0;
}
