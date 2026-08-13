/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_z80.h"

#include "core/wz_bus.h"
#include "core/wz_machine.h"

static wz_word_t wz_z80_add16(wz_word_t value, wz_word_t amount)
{
    return (wz_word_t)(value + amount);
}

static wz_result_t wz_z80_bus(wz_machine_t* machine,
                              wz_bus_cycle_t cycle,
                              wz_master_tick_t offset,
                              wz_word_t address,
                              wz_byte_t* value,
                              wz_byte_t t_states)
{
    wz_bus_request_t request;
    wz_bus_request_init(&request, cycle, machine->master_tick + offset, address,
                        value == 0 ? 0u : *value, t_states);
    if (wz_machine_bus_request(machine, &request) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (value != 0) {
        *value = request.value;
    }
    return WZ_RESULT_OK;
}

void wz_z80_state_init(wz_z80_state_t* state)
{
    if (state == 0) {
        return;
    }

    state->main.a = 0u;
    state->main.f = 0u;
    state->main.b = 0u;
    state->main.c = 0u;
    state->main.d = 0u;
    state->main.e = 0u;
    state->main.h = 0u;
    state->main.l = 0u;
    state->alternate.a = 0u;
    state->alternate.f = 0u;
    state->alternate.b = 0u;
    state->alternate.c = 0u;
    state->alternate.d = 0u;
    state->alternate.e = 0u;
    state->alternate.h = 0u;
    state->alternate.l = 0u;
    state->ix = 0u;
    state->iy = 0u;
    state->stack_pointer = 0xffffu;
    state->program_counter = 0u;
    state->i = 0u;
    state->r = 0u;
    state->iff1 = 0u;
    state->iff2 = 0u;
    state->interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_0;
    state->halted = 0u;
}

wz_result_t wz_z80_state_validate(const wz_z80_state_t* state)
{
    if (state == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (state->iff1 > 1u || state->iff2 > 1u || state->halted > 1u) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (state->interrupt_mode > (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2) {
        return WZ_RESULT_INVALID_STATE;
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_z80_step(wz_machine_t* machine)
{
    wz_byte_t opcode = 0u;
    wz_byte_t value = 0u;
    wz_byte_t low = 0u;
    wz_byte_t high = 0u;
    wz_word_t pc;
    wz_word_t address;

    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (wz_z80_state_validate(&machine->cpu) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }

    pc = machine->cpu.program_counter;
    if (wz_z80_bus(machine, WZ_BUS_M1_OPCODE_FETCH, 0u, pc, &opcode, 4u) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    machine->cpu.program_counter = wz_z80_add16(pc, 1u);

    switch (opcode) {
    case 0x00u:
        machine->master_tick += 8u;
        return WZ_RESULT_OK;
    case 0x3eu:
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u,
                       machine->cpu.program_counter, &value, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        machine->cpu.main.a = value;
        machine->master_tick += 14u;
        return WZ_RESULT_OK;
    case 0x32u:
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 8u,
                       machine->cpu.program_counter, &low, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_READ, 14u,
                       machine->cpu.program_counter, &high, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->cpu.program_counter = wz_z80_add16(machine->cpu.program_counter, 1u);
        address = (wz_word_t)low | ((wz_word_t)high << 8u);
        value = machine->cpu.main.a;
        if (wz_z80_bus(machine, WZ_BUS_MEMORY_WRITE, 20u, address, &value, 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->master_tick += 26u;
        return WZ_RESULT_OK;
    default:
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
}
