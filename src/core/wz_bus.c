/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_bus.h"

#include "core/wz_machine.h"

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

wz_result_t wz_machine_bus_request(wz_machine_t* machine,
                                   wz_bus_request_t* request)
{
    wz_byte_t contention_delay;

    if (machine == 0 || request == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }

    contention_delay = wz_machine_contention_delay(machine, request->cycle,
                                                   request->address,
                                                   request->master_tick,
                                                   request->t_states);
    request->contention_delay = contention_delay;
    if (contention_delay != 0u) {
        machine->master_tick += (wz_master_tick_t)contention_delay * 2u;
        request->master_tick += (wz_master_tick_t)contention_delay * 2u;
    }

    if ((request->cycle == WZ_BUS_M1_OPCODE_FETCH ||
         request->cycle == WZ_BUS_MEMORY_READ ||
         request->cycle == WZ_BUS_IO_READ ||
         request->cycle == WZ_BUS_INTERRUPT_ACKNOWLEDGE) &&
        machine->bus_data_source.read != 0 &&
        machine->bus_data_source.read(request, &request->value,
                                      machine->bus_data_source.context)) {
        request->source = WZ_BUS_SOURCE_DATA_SOURCE;
    } else switch (request->cycle) {
    case WZ_BUS_M1_OPCODE_FETCH:
    case WZ_BUS_MEMORY_READ:
        request->value = wz_machine_memory_read(machine, request->address);
        request->source = WZ_BUS_SOURCE_MEMORY;
        break;
    case WZ_BUS_MEMORY_WRITE:
        wz_machine_memory_write(machine, request->address, request->value);
        request->source = WZ_BUS_SOURCE_MEMORY;
        break;
    case WZ_BUS_IO_READ:
        if (machine->hardware_io_decode_enabled &&
            wz_machine_ula_port_fe_selected(request->address)) {
            request->value = wz_machine_ula_port_fe_read(machine, request->address);
            request->source = WZ_BUS_SOURCE_ULA;
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
            wz_machine_ula_port_fe_selected(request->address)) {
            wz_machine_ula_port_fe_write(machine, request->address, request->value,
                                         request->master_tick);
            request->source = WZ_BUS_SOURCE_ULA;
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

    if (machine->bus_observer.record != 0) {
        machine->bus_observer.record(request, machine->bus_observer.context);
    }
    if (machine->timing_trace != 0) {
        wz_trace_event_t event = {0};
        event.kind = WZ_TRACE_CPU_BUS;
        event.master_tick = request->master_tick;
        event.address = request->address;
        event.value = request->value;
        event.cycle = (wz_byte_t)request->cycle;
        event.t_states = request->t_states;
        event.auxiliary = request->contention_delay;
        wz_trace_emit_detail(machine->timing_trace, &event);
    }
    return WZ_RESULT_OK;
}
