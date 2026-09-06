/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_debugger.h"

#include <string.h>
#include <stdint.h>

static bool valid_range(wz_word_t address, size_t length)
{
    return length <= 65536u - (size_t)address;
}

static bool writable_range(const wz_machine_t* machine,
                           wz_word_t address, size_t length)
{
    if (machine == 0 || machine->profile == 0 || !valid_range(address, length)) {
        return false;
    }
    if (address < WZ_48K_ROM_SIZE &&
        (machine->profile->kind == WZ_MACHINE_128K_PAL ||
         machine->has_48k_rom != 0u)) {
        return false;
    }
    return length == 0u || address >= WZ_48K_ROM_SIZE;
}

static void wz_debugger_trace_result(wz_machine_t* machine,
                                     wz_byte_t operation,
                                     wz_result_t result,
                                     const wz_z80_state_t* state)
{
    wz_trace_event_t event;

    if (machine == 0 || machine->timing_trace == 0) {
        return;
    }
    event.kind = WZ_TRACE_DEVELOPER_MARKER;
    event.master_tick = machine->master_tick;
    event.sequence = 0u;
    event.address = 0u;
    event.program_counter = state == 0 ? machine->cpu.program_counter
                                       : state->program_counter;
    event.stack_pointer = state == 0 ? machine->cpu.stack_pointer
                                     : state->stack_pointer;
    event.register_snapshot = 0u;
    event.value = (wz_byte_t)result;
    event.auxiliary = operation;
    event.cycle = 0u;
    event.t_states = 0u;
    wz_trace_emit_detail(machine->timing_trace, &event);
}

static void wz_debugger_trace_memory_result(wz_machine_t* machine,
                                            wz_word_t address,
                                            wz_byte_t value,
                                            wz_result_t result)
{
    wz_trace_event_t event;

    if (machine == 0 || machine->timing_trace == 0) {
        return;
    }
    event.kind = WZ_TRACE_DEVELOPER_MARKER;
    event.master_tick = machine->master_tick;
    event.sequence = 0u;
    event.address = address;
    event.program_counter = machine->cpu.program_counter;
    event.stack_pointer = machine->cpu.stack_pointer;
    event.register_snapshot = 0u;
    event.value = value;
    event.auxiliary = (wz_byte_t)result;
    event.cycle = WZ_DEBUGGER_TRACE_MEMORY_MUTATION;
    event.t_states = 0u;
    wz_trace_emit_detail(machine->timing_trace, &event);
}

wz_result_t wz_debugger_snapshot(const wz_machine_t* machine,
                                 wz_debugger_snapshot_t* snapshot)
{
    if (machine == 0 || snapshot == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    snapshot->cpu = machine->cpu;
    snapshot->master_tick = machine->master_tick;
    snapshot->border_color = machine->border_color;
    snapshot->networking_mode = (wz_byte_t)machine->networking_mode;
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_read_memory(const wz_machine_t* machine,
                                    wz_word_t address,
                                    wz_byte_t* value)
{
    if (machine == 0 || value == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *value = wz_machine_memory_read(machine, address);
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_read_memory_block(const wz_machine_t* machine,
                                          wz_word_t address,
                                          wz_byte_t* values,
                                          size_t length)
{
    if (machine == 0 || (length > 0u && values == 0)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length > (size_t)UINT16_MAX + 1u - (size_t)address) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    for (size_t index = 0u; index < length; ++index) {
        values[index] = wz_machine_memory_read(machine,
                                               (wz_word_t)(address + index));
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_find_memory(const wz_machine_t* machine,
                                    wz_word_t address,
                                    size_t length,
                                    wz_byte_t value,
                                    wz_word_t* found_address)
{
    if (machine == 0 || found_address == 0 || !valid_range(address, length)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (size_t index = 0u; index < length; ++index) {
        if (wz_machine_memory_read(machine, (wz_word_t)(address + index)) == value) {
            *found_address = (wz_word_t)(address + index);
            return WZ_RESULT_OK;
        }
    }
    return WZ_RESULT_NOT_FOUND;
}

wz_result_t wz_debugger_set_access_mode(wz_machine_t* machine,
                                         wz_debugger_access_mode_t mode)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (mode != WZ_DEBUGGER_READ_ONLY &&
        mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_ACCESS_MODE,
                                 WZ_RESULT_INVALID_ARGUMENT, 0);
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    machine->debugger_access_mode = (wz_byte_t)mode;
    wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_ACCESS_MODE,
                             WZ_RESULT_OK, 0);
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_set_cpu_state(wz_machine_t* machine,
                                      const wz_z80_state_t* state)
{
    wz_result_t result;

    if (machine == 0 || state == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        result = WZ_RESULT_INVALID_STATE;
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_CPU_MUTATION,
                                 result, state);
        return result;
    }
    result = wz_z80_state_validate(state);
    if (result != WZ_RESULT_OK) {
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_CPU_MUTATION,
                                 result, state);
        return result;
    }
    machine->cpu = *state;
    wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_CPU_MUTATION,
                             WZ_RESULT_OK, state);
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_write_memory(wz_machine_t* machine,
                                     wz_word_t address,
                                     wz_byte_t value)
{
    wz_result_t result;

    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        result = WZ_RESULT_INVALID_STATE;
        wz_debugger_trace_memory_result(machine, address, value, result);
        return result;
    }
    if (machine->profile == 0 ||
        (address < WZ_48K_ROM_SIZE &&
         (machine->profile->kind == WZ_MACHINE_128K_PAL ||
          machine->has_48k_rom != 0u))) {
        result = WZ_RESULT_UNSUPPORTED_OPERATION;
        wz_debugger_trace_memory_result(machine, address, value, result);
        return result;
    }
    wz_machine_memory_write(machine, address, value);
    wz_debugger_trace_memory_result(machine, address, value, WZ_RESULT_OK);
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_write_memory_word(wz_machine_t* machine,
                                          wz_word_t address,
                                          wz_word_t value)
{
    wz_result_t result;
    if (machine == 0 || !valid_range(address, 2u)) return WZ_RESULT_INVALID_ARGUMENT;
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        result = WZ_RESULT_INVALID_STATE;
    } else if (!writable_range(machine, address, 2u)) {
        result = WZ_RESULT_UNSUPPORTED_OPERATION;
    } else {
        wz_machine_memory_write(machine, address, (wz_byte_t)value);
        wz_machine_memory_write(machine, (wz_word_t)(address + 1u),
                                (wz_byte_t)(value >> 8u));
        result = WZ_RESULT_OK;
    }
    wz_debugger_trace_memory_result(machine, address, (wz_byte_t)value, result);
    return result;
}

wz_result_t wz_debugger_fill_memory(wz_machine_t* machine,
                                    wz_word_t address,
                                    size_t length,
                                    wz_byte_t value)
{
    wz_result_t result;
    if (machine == 0 || !valid_range(address, length)) return WZ_RESULT_INVALID_ARGUMENT;
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        result = WZ_RESULT_INVALID_STATE;
    } else if (!writable_range(machine, address, length)) {
        result = WZ_RESULT_UNSUPPORTED_OPERATION;
    } else {
        for (size_t index = 0u; index < length; ++index) {
            wz_machine_memory_write(machine, (wz_word_t)(address + index), value);
        }
        result = WZ_RESULT_OK;
    }
    wz_debugger_trace_memory_result(machine, address, value, result);
    return result;
}

wz_result_t wz_debugger_copy_memory(wz_machine_t* machine,
                                    wz_word_t source_address,
                                    wz_word_t destination_address,
                                    size_t length)
{
    wz_result_t result;
    if (machine == 0 || !valid_range(source_address, length) ||
        !valid_range(destination_address, length)) return WZ_RESULT_INVALID_ARGUMENT;
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        result = WZ_RESULT_INVALID_STATE;
    } else if (!writable_range(machine, destination_address, length)) {
        result = WZ_RESULT_UNSUPPORTED_OPERATION;
    } else {
        memmove(&machine->memory[destination_address],
                &machine->memory[source_address], length);
        result = WZ_RESULT_OK;
    }
    wz_debugger_trace_memory_result(machine, destination_address,
                                    length == 0u ? 0u : machine->memory[destination_address],
                                    result);
    return result;
}

wz_result_t wz_debugger_set_breakpoint(wz_machine_t* machine,
                                       wz_word_t address)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    machine->debugger_breakpoint_address = address;
    machine->debugger_breakpoint_active = 1u;
    machine->debugger_breakpoint_hit = 0u;
    wz_debugger_trace_result(machine, WZ_TRACE_DEBUGGER_BREAKPOINT_HIT,
                             WZ_RESULT_OK, 0);
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_clear_breakpoint(wz_machine_t* machine)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    machine->debugger_breakpoint_active = 0u;
    machine->debugger_breakpoint_hit = 0u;
    wz_debugger_trace_result(machine, WZ_TRACE_DEBUGGER_BREAKPOINT_HIT,
                             WZ_RESULT_OK, 0);
    return WZ_RESULT_OK;
}

bool wz_debugger_breakpoint_active(const wz_machine_t* machine)
{
    return machine != 0 && machine->debugger_breakpoint_active != 0u;
}

bool wz_debugger_breakpoint_hit(const wz_machine_t* machine)
{
    return machine != 0 && machine->debugger_breakpoint_hit != 0u;
}

void wz_debugger_clear_breakpoint_hit(wz_machine_t* machine)
{
    if (machine != 0) {
        machine->debugger_breakpoint_hit = 0u;
    }
}

wz_result_t wz_debugger_step(wz_machine_t* machine, size_t* executed)
{
    wz_result_t result;

    if (machine == 0 || executed == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *executed = 0u;
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        return WZ_RESULT_INVALID_STATE;
    }
    result = wz_z80_step(machine);
    if (result != WZ_RESULT_OK) {
        return result;
    }
    if (machine->debugger_breakpoint_hit != 0u) {
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_STEP,
                                 WZ_RESULT_BREAKPOINT_HIT, 0);
        return WZ_RESULT_BREAKPOINT_HIT;
    }
    *executed = 1u;
    wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_STEP,
                             WZ_RESULT_OK, 0);
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_continue(wz_machine_t* machine,
                                 size_t max_instructions,
                                 size_t* executed)
{
    wz_result_t result;

    if (machine == 0 || executed == 0u || max_instructions == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *executed = 0u;
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        return WZ_RESULT_INVALID_STATE;
    }
    while (*executed < max_instructions) {
        result = wz_z80_step(machine);
        if (result != WZ_RESULT_OK) {
            return result;
        }
        if (machine->debugger_breakpoint_hit != 0u) {
            wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_CONTINUE,
                                     WZ_RESULT_BREAKPOINT_HIT, 0);
            return WZ_RESULT_BREAKPOINT_HIT;
        }
        ++*executed;
    }
    wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_CONTINUE,
                             WZ_RESULT_OK, 0);
    return WZ_RESULT_OK;
}
