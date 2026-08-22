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

wz_result_t wz_machine_bus_request(wz_machine_t* machine,
                                   wz_bus_request_t* request)
{
    if (machine == 0 || request == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }

    switch (request->cycle) {
    case WZ_BUS_M1_OPCODE_FETCH:
    case WZ_BUS_MEMORY_READ:
        request->value = machine->memory[request->address];
        break;
    case WZ_BUS_MEMORY_WRITE:
        machine->memory[request->address] = request->value;
        break;
    case WZ_BUS_IO_READ:
    case WZ_BUS_INTERRUPT_ACKNOWLEDGE:
        if (machine->bus_input.read != 0) {
            request->value = machine->bus_input.read(
                request->cycle, request->address, machine->bus_input.context);
        } else {
            request->value = 0xffu;
        }
        break;
    case WZ_BUS_IO_WRITE:
    case WZ_BUS_INTERNAL:
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
        wz_trace_emit_detail(machine->timing_trace, &event);
    }
    return WZ_RESULT_OK;
}
