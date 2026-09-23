/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_diagnostic_block.h"

#include <stdlib.h>

#include "core/wz_state.h"

wz_result_t wz_diagnostic_block_save(const wz_machine_t* machine,
                                     wz_byte_t* data,
                                     size_t capacity,
                                     size_t* length)
{
    wz_state_writer_t writer;
    wz_result_t result;

    if (machine == 0 || data == 0 || length == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (capacity < WZ_STATE_MACHINE_LENGTH) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    wz_state_writer_init(&writer, data, capacity);
    result = wz_state_serialize_machine(machine, &writer);
    if (result != WZ_RESULT_OK) {
        return result;
    }
    *length = writer.length;
    return WZ_RESULT_OK;
}

wz_result_t wz_diagnostic_block_load(wz_machine_t* machine,
                                     const wz_byte_t* data,
                                     size_t length)
{
    wz_machine_t candidate;
    wz_result_t result;

    if (machine == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length != WZ_STATE_MACHINE_LENGTH) {
        return WZ_RESULT_INVALID_STATE;
    }
    result = wz_machine_init(&candidate,
                             data[1u] == (wz_byte_t)WZ_MACHINE_48K_PAL
                                 ? wz_machine_profile_48k_pal()
                                 : data[1u] == (wz_byte_t)WZ_MACHINE_128K_PAL
                                       ? wz_machine_profile_128k_pal()
                                       : 0);
    if (result != WZ_RESULT_OK) {
        return result;
    }
    result = wz_state_deserialize_machine(&candidate, data, length);
    if (result != WZ_RESULT_OK) {
        wz_machine_destroy(&candidate);
        return result;
    }

    candidate.bus_observer = machine->bus_observer;
    candidate.bus_input = machine->bus_input;
    candidate.bus_data_source = machine->bus_data_source;
    candidate.timing_trace = machine->timing_trace;
    candidate.tape = machine->tape;
    candidate.tape_state = machine->tape_state;
    candidate.tape_state.tape = &candidate.tape;
    candidate.tape_mounted = machine->tape_mounted;
    wz_machine_destroy(machine);
    *machine = candidate;
    machine->tape_state.tape = &machine->tape;
    return WZ_RESULT_OK;
}
