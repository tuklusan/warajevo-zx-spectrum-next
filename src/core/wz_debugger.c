/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_debugger.h"

#include <stdint.h>

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
