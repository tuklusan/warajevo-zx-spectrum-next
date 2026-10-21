/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
Proprietary rights reserved except as expressly licensed herein.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
patent, trademark, and governing-law provisions.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_bus.h"

#include "core/wz_machine.h"
#include "core/wz_machine_internal.h"

#include <stdbool.h>

void wz_bus_request_init(wz_bus_request_t* request,
                         wz_bus_cycle_t cycle,
                         wz_master_tick_t master_tick,
                         wz_word_t address,
                         wz_byte_t value,
                         wz_byte_t t_states)
{
    if (request == 0) {
        return;
    }
    request->cycle = cycle;
    request->master_tick = master_tick;
    request->address = address;
    request->value = value;
    request->t_states = t_states;
    request->contention_delay = 0u;
    request->direction = cycle == WZ_BUS_MEMORY_WRITE ||
                                cycle == WZ_BUS_IO_WRITE
                            ? WZ_BUS_DIRECTION_WRITE
                            : cycle == WZ_BUS_M1_OPCODE_FETCH ||
                                      cycle == WZ_BUS_MEMORY_READ ||
                                      cycle == WZ_BUS_IO_READ ||
                                      cycle == WZ_BUS_INTERRUPT_ACKNOWLEDGE
                                  ? WZ_BUS_DIRECTION_READ
                                  : WZ_BUS_DIRECTION_NONE;
    request->source = WZ_BUS_SOURCE_NONE;
}

void wz_bus_observer_init(wz_bus_observer_t* observer,
                          wz_bus_observer_fn record,
                          void* context)
{
    if (observer == 0) {
        return;
    }
    observer->record = record;
    observer->context = context;
}

void wz_bus_input_init(wz_bus_input_t* input,
                       wz_bus_input_fn read,
                       void* context)
{
    if (input == 0) {
        return;
    }
    input->read = read;
    input->context = context;
}

void wz_bus_data_source_init(wz_bus_data_source_t* source,
                             wz_bus_data_source_fn read,
                             void* context)
{
    if (source == 0) {
        return;
    }
    source->read = read;
    source->context = context;
}

wz_result_t wz_machine_set_bus_observer(wz_machine_t* machine,
                                        const wz_bus_observer_t* observer)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (observer == 0) {
        machine->bus_observer.record = 0;
        machine->bus_observer.context = 0;
        return WZ_RESULT_OK;
    }
    machine->bus_observer = *observer;
    return WZ_RESULT_OK;
}

wz_result_t wz_machine_set_bus_input(wz_machine_t* machine,
                                     const wz_bus_input_t* input)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (input == 0) {
        wz_bus_input_init(&machine->bus_input, 0, 0);
    } else {
        machine->bus_input = *input;
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_machine_set_bus_data_source(wz_machine_t* machine,
                                           const wz_bus_data_source_t* source)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (source == 0) {
        wz_bus_data_source_init(&machine->bus_data_source, 0, 0);
    } else {
        machine->bus_data_source = *source;
    }
    return WZ_RESULT_OK;
}

// codeql[cpp/poorly-documented-function]
wz_result_t wz_machine_bus_request(wz_machine_t* machine,
                                   wz_bus_request_t* request)
{
    /* Resolve a bus cycle, including contention and optional data sources. */
    wz_byte_t contention_delay;
    bool interface1_unpage_after_fetch = false;

    if (machine == 0 || request == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if ((request->cycle == WZ_BUS_IO_READ || request->cycle == WZ_BUS_IO_WRITE) &&
        request->t_states > 4u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }

    contention_delay = wz_machine_contention_delay(machine, request->cycle,
                                                   request->address,
                                                   request->master_tick,
                                                   request->t_states);
    request->contention_delay = contention_delay;
    if (contention_delay != 0u) {
        wz_machine_advance_master_ticks(machine, (wz_master_tick_t)contention_delay * 2u);
        request->master_tick += (wz_master_tick_t)contention_delay * 2u;
    }

    if (wz_machine_trace_ula_until(machine, request->master_tick, false) !=
        WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }

    if (request->cycle == WZ_BUS_M1_OPCODE_FETCH &&
        machine->networking_mode == WZ_NETWORKING_INTERFACE1 &&
        machine->has_interface1_rom != 0u) {
        if (request->address == 0x0008u || request->address == 0x1708u) {
            machine->interface1_rom_page = 1u;
        } else if (request->address == 0x0700u &&
                   machine->interface1_rom_page != 0u) {
            interface1_unpage_after_fetch = true;
        }
    }

    if ((request->cycle == WZ_BUS_M1_OPCODE_FETCH ||
         request->cycle == WZ_BUS_MEMORY_READ ||
         request->cycle == WZ_BUS_IO_READ ||
         request->cycle == WZ_BUS_INTERRUPT_ACKNOWLEDGE) &&
        machine->bus_data_source.read != 0 &&
        machine->bus_data_source.read(request, &request->value,
                                      machine->bus_data_source.context)) {
        request->source = WZ_BUS_SOURCE_DATA_SOURCE;
    } else switch (request->cycle) { // codeql[cpp/long-switch]
    case WZ_BUS_M1_OPCODE_FETCH:
    case WZ_BUS_MEMORY_READ:
        request->value = wz_machine_memory_read(machine, request->address);
        request->source = WZ_BUS_SOURCE_MEMORY;
        break;
    case WZ_BUS_MEMORY_WRITE:
        if ((machine->timing_trace != 0 ?
                wz_machine_memory_write_at_tick_deferred(
                    machine, request->address, request->value,
                    request->master_tick) :
                wz_machine_memory_write_at_tick(machine, request->address,
                    request->value, request->master_tick)) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
        request->source = WZ_BUS_SOURCE_MEMORY;
        break;
    case WZ_BUS_IO_READ:
        if (machine->hardware_io_decode_enabled &&
            machine->networking_mode == WZ_NETWORKING_INTERFACE1 &&
            wz_machine_interface1_port_selected(request->address, 0xe7u)) {
            if (wz_machine_interface1_data_read(machine, &request->value) !=
                WZ_RESULT_OK) {
                request->value = 0xffu;
            }
            request->source = WZ_BUS_SOURCE_INPUT;
        } else if (machine->hardware_io_decode_enabled &&
            machine->networking_mode == WZ_NETWORKING_INTERFACE1 &&
            wz_machine_interface1_port_selected(request->address, 0xefu)) {
            request->value = wz_machine_interface1_status_read(machine);
            request->source = WZ_BUS_SOURCE_INPUT;
        } else if (machine->hardware_io_decode_enabled &&
            machine->networking_mode == WZ_NETWORKING_INTERFACE1 &&
            wz_machine_interface1_port_selected(request->address, 0xf7u)) {
            request->value = 0xffu;
            request->source = WZ_BUS_SOURCE_INPUT;
        } else if (machine->hardware_io_decode_enabled &&
            wz_kempston_port_selected(request->address)) {
            request->value = wz_machine_kempston_read(machine, request->address);
            request->source = WZ_BUS_SOURCE_INPUT;
        } else if (machine->hardware_io_decode_enabled &&
            wz_machine_ula_port_fe_selected(request->address)) {
            request->value = wz_machine_ula_port_fe_read(machine, request->address);
            request->source = WZ_BUS_SOURCE_ULA;
        } else if (machine->hardware_io_decode_enabled &&
                   wz_printer_port_selected(request->address)) {
            request->value = wz_machine_printer_read(machine, request->address);
            request->source = WZ_BUS_SOURCE_PRINTER;
        } else if (machine->bus_input.read != 0) {
            request->value = machine->bus_input.read(
                request->cycle, request->address, machine->bus_input.context);
            request->source = WZ_BUS_SOURCE_INPUT;
        } else {
            request->value = 0xffu;
            request->source = WZ_BUS_SOURCE_FALLBACK;
        }
        break;
    case WZ_BUS_IO_WRITE:
        if (machine->hardware_io_decode_enabled &&
            machine->profile != 0 && machine->profile->kind == WZ_MACHINE_128K_PAL &&
            (request->address & 0xc002u) == 0xc000u) {
            request->source = WZ_BUS_SOURCE_AY;
            if (wz_ay_select_register(&machine->ay, request->value,
                                      request->master_tick) != WZ_RESULT_OK) {
                return WZ_RESULT_INVALID_STATE;
            }
        } else if (machine->hardware_io_decode_enabled &&
            machine->networking_mode == WZ_NETWORKING_INTERFACE1 &&
            wz_machine_interface1_port_selected(request->address, 0xe7u)) {
            if (wz_machine_interface1_data_write(machine, request->value) !=
                WZ_RESULT_OK) {
                return WZ_RESULT_INVALID_STATE;
            }
            request->source = WZ_BUS_SOURCE_INPUT;
        } else if (machine->hardware_io_decode_enabled &&
            machine->profile != 0 && machine->profile->kind == WZ_MACHINE_128K_PAL &&
            (request->address & 0xc002u) == 0x8000u) {
            request->source = WZ_BUS_SOURCE_AY;
            if (wz_ay_write_data(&machine->ay, request->value,
                                 request->master_tick) != WZ_RESULT_OK) {
                return WZ_RESULT_INVALID_STATE;
            }
        } else if (machine->hardware_io_decode_enabled &&
            machine->profile != 0 && machine->profile->kind == WZ_MACHINE_128K_PAL &&
            wz_machine_128k_paging_write(machine, request->address, request->value) ==
                WZ_RESULT_OK) {
            request->source = WZ_BUS_SOURCE_MEMORY;
        } else if (machine->hardware_io_decode_enabled &&
            wz_machine_ula_port_fe_selected(request->address)) {
            wz_machine_ula_port_fe_write(machine, request->address, request->value,
                                         request->master_tick);
            request->source = WZ_BUS_SOURCE_ULA;
        } else if (machine->hardware_io_decode_enabled &&
                   wz_printer_port_selected(request->address)) {
            if (wz_machine_printer_write(machine, request->address, request->value,
                                         request->master_tick) != WZ_RESULT_OK) {
                return WZ_RESULT_INVALID_STATE;
            }
            request->source = WZ_BUS_SOURCE_PRINTER;
        } else if (machine->hardware_io_decode_enabled &&
                   machine->networking_mode == WZ_NETWORKING_INTERFACE1 &&
                   wz_machine_interface1_port_selected(request->address, 0xefu)) {
            if (wz_machine_interface1_control_write(
                    machine, request->value, request->master_tick) != WZ_RESULT_OK) {
                return WZ_RESULT_INVALID_STATE;
            }
            request->source = WZ_BUS_SOURCE_INPUT;
        } else if (machine->hardware_io_decode_enabled &&
                   machine->networking_mode == WZ_NETWORKING_INTERFACE1 &&
                   wz_machine_interface1_port_selected(request->address, 0xf7u)) {
            request->source = WZ_BUS_SOURCE_INPUT;
        } else {
            request->source = WZ_BUS_SOURCE_FALLBACK;
        }
        break;
    case WZ_BUS_INTERRUPT_ACKNOWLEDGE:
        if (machine->bus_input.read != 0) {
            request->value = machine->bus_input.read(
                request->cycle, request->address, machine->bus_input.context);
            request->source = WZ_BUS_SOURCE_INPUT;
        } else {
            request->value = 0xffu;
            request->source = WZ_BUS_SOURCE_FALLBACK;
        }
        break;
    case WZ_BUS_INTERNAL:
        request->source = WZ_BUS_SOURCE_NONE;
        break;
    default:
        return WZ_RESULT_INVALID_ARGUMENT;
    }

    /* Fetch the Interface 1 RET at 0700h before exposing the main ROM again. */
    if (interface1_unpage_after_fetch) {
        machine->interface1_rom_page = 0u;
    }

    if (machine->bus_observer.record != 0) {
        machine->bus_observer.record(request, machine->bus_observer.context);
    }
    if (machine->timing_trace != 0) {
        wz_trace_event_t event = {0};
        /* Keep the binary trace CPU state synchronized before this access. */
        wz_z80_trace_bus_context(machine, request->master_tick);
        event.kind = WZ_TRACE_CPU_BUS;
        event.master_tick = request->master_tick;
        event.address = request->address;
        event.value = request->value;
        event.cycle = (wz_byte_t)request->cycle;
        event.t_states = request->t_states;
        event.auxiliary = request->contention_delay;
        event.bus_source = request->source;
        event.program_counter = machine->cpu.program_counter;
        event.stack_pointer = machine->cpu.stack_pointer;
        event.register_snapshot = (wz_qword_t)machine->cpu.main.a |
            ((wz_qword_t)machine->cpu.main.f << 8u) |
            ((wz_qword_t)machine->cpu.main.b << 16u) |
            ((wz_qword_t)machine->cpu.main.c << 24u) |
            ((wz_qword_t)machine->cpu.main.d << 32u) |
            ((wz_qword_t)machine->cpu.main.e << 40u) |
            ((wz_qword_t)machine->cpu.main.h << 48u) |
            ((wz_qword_t)machine->cpu.main.l << 56u);
        wz_trace_emit_detail(machine->timing_trace, &event);
        if (machine->networking_mode == WZ_NETWORKING_INTERFACE1 &&
            (request->cycle == WZ_BUS_IO_READ ||
             request->cycle == WZ_BUS_IO_WRITE) &&
            (wz_machine_interface1_port_selected(request->address, 0xe7u) ||
             wz_machine_interface1_port_selected(request->address, 0xefu) ||
             wz_machine_interface1_port_selected(request->address, 0xf7u))) {
            wz_trace_event_t interface1_event = {0};
            interface1_event.kind = WZ_TRACE_INTERFACE1_IO;
            interface1_event.master_tick = request->master_tick;
            interface1_event.address = request->address;
            interface1_event.program_counter = machine->cpu.program_counter;
            interface1_event.value = request->value;
            interface1_event.auxiliary = machine->interface1_control_latch;
            interface1_event.cycle = (wz_byte_t)request->cycle;
            interface1_event.t_states = request->t_states;
            interface1_event.register_snapshot =
                (wz_qword_t)machine->interface1_motor_shift |
                ((wz_qword_t)machine->interface1_active_motor << 8u);
            wz_trace_emit_detail(machine->timing_trace, &interface1_event);
        }
    }
    if (wz_machine_trace_ula_until(machine, request->master_tick, true) !=
        WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    return WZ_RESULT_OK;
}
