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

typedef struct {
    wz_byte_t a;
    wz_byte_t f;
} expected_t;

static int signed_byte(unsigned value)
{
    return value < 128u ? (int)value : (int)value - 256;
}

static bool even_parity(unsigned value)
{
    unsigned bits = 0u;
    for (unsigned index = 0u; index < 8u; ++index) {
        bits += (value >> index) & 1u;
    }
    return (bits & 1u) == 0u;
}

static unsigned result_flags(unsigned result)
{
    unsigned flags = result & (FLAG_S | FLAG_Y | FLAG_X);
    if (result == 0u) flags |= FLAG_Z;
    return flags;
}

static expected_t expected_alu(unsigned operation, unsigned left,
                               unsigned operand, unsigned input_carry)
{
    expected_t expected;
    unsigned flags;
    unsigned result;
    unsigned carry = (operation == 1u || operation == 3u) ? input_carry : 0u;
    expected.a = (wz_byte_t)left;
    if (operation <= 1u) {
        const unsigned wide = left + operand + carry;
        const int signed_sum = signed_byte(left) + signed_byte(operand) +
                               (int)carry;
        result = wide & 0xffu;
        flags = result_flags(result);
        if (((left & 15u) + (operand & 15u) + carry) > 15u) flags |= FLAG_H;
        if (signed_sum < -128 || signed_sum > 127) flags |= FLAG_PV;
        if (wide > 255u) flags |= FLAG_C;
        expected.a = (wz_byte_t)result;
    } else if (operation <= 3u || operation == 7u) {
        const int difference = (int)left - (int)operand - (int)carry;
        const int signed_difference = signed_byte(left) -
                                      signed_byte(operand) - (int)carry;
        result = (unsigned)difference & 0xffu;
        flags = result_flags(result) | FLAG_N;
        if (operation == 7u) {
            flags = (flags & ~(FLAG_Y | FLAG_X)) |
                    (operand & (FLAG_Y | FLAG_X));
        } else {
            expected.a = (wz_byte_t)result;
        }
        if ((left & 15u) < ((operand & 15u) + carry)) flags |= FLAG_H;
        if (signed_difference < -128 || signed_difference > 127) flags |= FLAG_PV;
        if (difference < 0) flags |= FLAG_C;
    } else {
        if (operation == 4u) result = left & operand;
        else if (operation == 5u) result = left ^ operand;
        else result = left | operand;
        flags = result_flags(result);
        if (even_parity(result)) flags |= FLAG_PV;
        if (operation == 4u) flags |= FLAG_H;
        expected.a = (wz_byte_t)result;
    }
    expected.f = (wz_byte_t)flags;
    return expected;
}

static expected_t expected_inc_dec(unsigned initial, unsigned input_carry,
                                   bool decrement)
{
    expected_t expected;
    const unsigned result = decrement ? ((initial - 1u) & 0xffu) :
                                        ((initial + 1u) & 0xffu);
    unsigned flags = result_flags(result) | input_carry;
    if (decrement) {
        flags |= FLAG_N;
        if ((initial & 15u) == 0u) flags |= FLAG_H;
        if (initial == 0x80u) flags |= FLAG_PV;
    } else {
        if ((initial & 15u) == 15u) flags |= FLAG_H;
        if (initial == 0x7fu) flags |= FLAG_PV;
    }
    expected.a = (wz_byte_t)result;
    expected.f = (wz_byte_t)flags;
    return expected;
}

static bool run_alu_matrix(wz_machine_t* machine)
{
    static const wz_byte_t opcodes[8] = {
        0xc6u, 0xceu, 0xd6u, 0xdeu, 0xe6u, 0xeeu, 0xf6u, 0xfeu
    };
    for (unsigned operation = 0u; operation < 8u; ++operation) {
        wz_machine_memory_write(machine, 0x8000u, opcodes[operation]);
        for (unsigned input_carry = 0u; input_carry <= 1u; ++input_carry) {
            for (unsigned operand = 0u; operand < 256u; ++operand) {
                wz_machine_memory_write(machine, 0x8001u, (wz_byte_t)operand);
                for (unsigned left = 0u; left < 256u; ++left) {
                    const expected_t expected = expected_alu(
                        operation, left, operand, input_carry);
                    const wz_master_tick_t before = machine->master_tick;
                    machine->cpu.program_counter = 0x8000u;
                    machine->cpu.main.a = (wz_byte_t)left;
                    machine->cpu.main.f = input_carry != 0u ? FLAG_C : 0u;
                    if (wz_z80_step(machine) != WZ_RESULT_OK ||
                        machine->cpu.program_counter != 0x8002u ||
                        machine->master_tick != before + 14u ||
                        machine->cpu.main.a != expected.a ||
                        machine->cpu.main.f != expected.f) {
                        (void)fprintf(stderr,
                            "ALU mismatch op=%u A=%02x operand=%02x carry=%u "
                            "actual=%02x/%02x expected=%02x/%02x\n",
                            operation, left, operand, input_carry,
                            machine->cpu.main.a, machine->cpu.main.f,
                            expected.a, expected.f);
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

static bool run_inc_dec_matrix(wz_machine_t* machine)
{
    for (unsigned decrement = 0u; decrement <= 1u; ++decrement) {
        wz_machine_memory_write(machine, 0x8000u,
                                decrement != 0u ? 0x3du : 0x3cu);
        for (unsigned input_carry = 0u; input_carry <= 1u; ++input_carry) {
            for (unsigned initial = 0u; initial < 256u; ++initial) {
                const expected_t expected = expected_inc_dec(
                    initial, input_carry, decrement != 0u);
                const wz_master_tick_t before = machine->master_tick;
                machine->cpu.program_counter = 0x8000u;
                machine->cpu.main.a = (wz_byte_t)initial;
                machine->cpu.main.f = input_carry != 0u ? FLAG_C : 0u;
                if (wz_z80_step(machine) != WZ_RESULT_OK ||
                    machine->cpu.program_counter != 0x8001u ||
                    machine->master_tick != before + 8u ||
                    machine->cpu.main.a != expected.a ||
                    machine->cpu.main.f != expected.f) {
                    (void)fprintf(stderr,
                        "INC/DEC mismatch dec=%u A=%02x carry=%u "
                        "actual=%02x/%02x expected=%02x/%02x\n",
                        decrement, initial, input_carry,
                        machine->cpu.main.a, machine->cpu.main.f,
                        expected.a, expected.f);
                    return false;
                }
            }
        }
    }
    return true;
}

int main(void)
{
    wz_machine_t machine;
    bool passed;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        (void)fprintf(stderr, "machine init failed\n");
        return 1;
    }
    passed = run_alu_matrix(&machine) && run_inc_dec_matrix(&machine);
    wz_machine_destroy(&machine);
    if (!passed) return 1;
    (void)puts("{\"testId\":\"cpu-alu-flags\",\"status\":\"pass\","
               "\"cases\":1049600,\"immediateAluCases\":1048576,"
               "\"incDecCases\":1024}");
    return 0;
}
