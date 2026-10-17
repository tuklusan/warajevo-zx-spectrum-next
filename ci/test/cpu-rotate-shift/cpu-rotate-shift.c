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

enum { FLAG_C = 1, FLAG_PV = 4, FLAG_X = 8, FLAG_Y = 32,
       FLAG_Z = 64, FLAG_S = 128 };

typedef struct {
    wz_byte_t value;
    wz_byte_t flags;
} expected_t;

static bool even_parity(unsigned value)
{
    unsigned count = 0u;
    for (unsigned bit = 0u; bit < 8u; ++bit) count += (value >> bit) & 1u;
    return (count & 1u) == 0u;
}

static expected_t cb_reference(unsigned operation, unsigned value,
                               unsigned old_carry)
{
    expected_t result;
    unsigned shifted = 0u;
    unsigned carry = 0u;
    switch (operation) {
    case 0u: carry = value >> 7u; shifted = (value << 1u) | carry; break;
    case 1u: carry = value & 1u; shifted = (value >> 1u) | (carry << 7u); break;
    case 2u: carry = value >> 7u; shifted = (value << 1u) | old_carry; break;
    case 3u: carry = value & 1u; shifted = (value >> 1u) | (old_carry << 7u); break;
    case 4u: carry = value >> 7u; shifted = value << 1u; break;
    case 5u: carry = value & 1u; shifted = (value >> 1u) | (value & 128u); break;
    case 6u: carry = value >> 7u; shifted = (value << 1u) | 1u; break;
    default: carry = value & 1u; shifted = value >> 1u; break;
    }
    shifted &= 255u;
    result.value = (wz_byte_t)shifted;
    result.flags = (wz_byte_t)((shifted & (FLAG_S | FLAG_Y | FLAG_X)) | carry);
    if (shifted == 0u) result.flags |= FLAG_Z;
    if (even_parity(shifted)) result.flags |= FLAG_PV;
    return result;
}

/* Modes: B, (HL), DD CB d to B, FD CB d to B, DD CB d to memory. */
static bool cb_case(wz_machine_t* machine, unsigned mode, unsigned operation,
                    unsigned value, unsigned old_carry)
{
    const bool indexed = mode >= 2u;
    const bool memory_only = mode == 4u;
    const wz_byte_t opcode = (wz_byte_t)((operation << 3u) |
                                        (mode == 1u || memory_only ? 6u : 0u));
    const expected_t expected = cb_reference(operation, value, old_carry);
    const wz_master_tick_t before = machine->master_tick;
    const wz_master_tick_t duration = indexed ? 46u : (mode == 1u ? 30u : 16u);
    const wz_word_t address = indexed ? 0x9001u : 0x9000u;
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.f = (wz_byte_t)(0xc4u | old_carry);
    machine->cpu.main.b = (wz_byte_t)(mode == 1u || memory_only ? 0xa5u : value);
    machine->cpu.main.h = 0x90u;
    machine->cpu.main.l = 0x00u;
    machine->cpu.ix = 0x9000u;
    machine->cpu.iy = 0x9000u;
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    wz_machine_memory_write(machine, address, (wz_byte_t)value);
    if (indexed) {
        wz_machine_memory_write(machine, 0x8000u, (wz_byte_t)(mode == 3u ? 0xfdu : 0xddu));
        wz_machine_memory_write(machine, 0x8001u, 0xcbu);
        wz_machine_memory_write(machine, 0x8002u, 0x01u);
        wz_machine_memory_write(machine, 0x8003u, opcode);
    } else {
        wz_machine_memory_write(machine, 0x8000u, 0xcbu);
        wz_machine_memory_write(machine, 0x8001u, opcode);
    }
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->cpu.program_counter != (indexed ? 0x8004u : 0x8002u) ||
        machine->master_tick != before + duration ||
        machine->cpu.r != 2u || machine->cpu.q != 1u ||
        machine->cpu.main.f != expected.flags ||
        machine->cpu.main.b != (mode == 1u || memory_only ? 0xa5u : expected.value) ||
        wz_machine_memory_read(machine, address) !=
            (mode == 0u ? (wz_byte_t)value : expected.value)) {
        (void)fprintf(stderr,
            "CB mismatch mode=%u op=%u value=%02x carry=%u "
            "F=%02x/%02x B=%02x/%02x PC=%04x ticks=%llu/%llu R=%u\n",
            mode, operation, value, old_carry,
            machine->cpu.main.f, expected.flags, machine->cpu.main.b,
            mode == 1u || memory_only ? 0xa5u : expected.value,
            machine->cpu.program_counter,
            (unsigned long long)(machine->master_tick - before),
            (unsigned long long)duration, machine->cpu.r);
        return false;
    }
    return true;
}

static bool accumulator_case(wz_machine_t* machine, unsigned operation,
                             unsigned value, unsigned old_carry)
{
    static const wz_byte_t opcodes[4] = {0x07u, 0x0fu, 0x17u, 0x1fu};
    const unsigned carry = (operation & 1u) != 0u ? value & 1u : value >> 7u;
    const unsigned rotated = (operation & 1u) != 0u ?
        (value >> 1u) | ((operation == 1u ? carry : old_carry) << 7u) :
        ((value << 1u) | (operation == 0u ? carry : old_carry)) & 255u;
    const wz_byte_t expected_f = (wz_byte_t)(0xc4u |
        (rotated & (FLAG_Y | FLAG_X)) | carry);
    const wz_master_tick_t before = machine->master_tick;
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.a = (wz_byte_t)value;
    machine->cpu.main.f = (wz_byte_t)(0xc4u | old_carry);
    machine->cpu.r = 0u;
    machine->cpu.q = 0u;
    wz_machine_memory_write(machine, 0x8000u, opcodes[operation]);
    if (wz_z80_step(machine) != WZ_RESULT_OK ||
        machine->cpu.program_counter != 0x8001u ||
        machine->master_tick != before + 8u || machine->cpu.r != 1u ||
        machine->cpu.q != 1u || machine->cpu.main.a != rotated ||
        machine->cpu.main.f != expected_f) {
        (void)fprintf(stderr,
            "A rotate mismatch op=%u value=%02x carry=%u A=%02x/%02x F=%02x/%02x\n",
            operation, value, old_carry, machine->cpu.main.a, rotated,
            machine->cpu.main.f, expected_f);
        return false;
    }
    return true;
}

int main(void)
{
    wz_machine_t machine;
    unsigned cases = 0u;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        (void)fputs("machine init failed\n", stderr);
        return 1;
    }
    for (unsigned mode = 0u; mode < 5u; ++mode) {
        for (unsigned operation = 0u; operation < 8u; ++operation) {
            for (unsigned value = 0u; value < 256u; ++value) {
                for (unsigned carry = 0u; carry < 2u; ++carry) {
                    if (!cb_case(&machine, mode, operation, value, carry)) {
                        wz_machine_destroy(&machine);
                        return 1;
                    }
                    ++cases;
                }
            }
        }
    }
    for (unsigned operation = 0u; operation < 4u; ++operation) {
        for (unsigned value = 0u; value < 256u; ++value) {
            for (unsigned carry = 0u; carry < 2u; ++carry) {
                if (!accumulator_case(&machine, operation, value, carry)) {
                    wz_machine_destroy(&machine);
                    return 1;
                }
                ++cases;
            }
        }
    }
    wz_machine_destroy(&machine);
    (void)printf("{\"testId\":\"cpu-rotate-shift\",\"status\":\"pass\","
                 "\"cases\":%u,\"cbCases\":20480,\"accumulatorCases\":2048}\n", cases);
    return 0;
}
