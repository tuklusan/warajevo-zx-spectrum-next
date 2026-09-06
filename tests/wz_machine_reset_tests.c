/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "core/wz_debugger.h"

static void record_trace(const wz_trace_event_t* event, void* context)
{
    unsigned* count = (unsigned*)context;
    (void)event;
    *count += 1u;
}

static int verify_reset(const wz_machine_profile_t* profile)
{
    static const wz_tape_segment_t segments[] = {{8u, 1u}};
    wz_machine_t machine;
    wz_trace_sink_t trace;
    unsigned trace_count = 0u;
    wz_byte_t held_key;
    wz_master_tick_t tape_elapsed;

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) return 1;
    wz_trace_sink_init(&trace, record_trace, &trace_count);
    wz_machine_set_timing_trace(&machine, &trace);
    if (wz_machine_mount_tape(&machine, segments, 1u) != WZ_RESULT_OK ||
        wz_machine_set_tape_motor(&machine, true) != WZ_RESULT_OK ||
        wz_machine_advance_tape(&machine, 3u) != WZ_RESULT_OK ||
        wz_machine_set_keyboard_key(&machine, 2u, 3u, true) != WZ_RESULT_OK ||
        wz_machine_set_kempston_control(&machine, WZ_KEMPSTON_FIRE, true) !=
            WZ_RESULT_OK) {
        wz_machine_destroy(&machine);
        return 1;
    }
    machine.cpu.main.a = 0x5au;
    machine.cpu.program_counter = 0x4321u;
    machine.memory[0x8000u] = 0xa5u;
    machine.master_tick = 1234u;
    machine.paging_7ffd = 0x07u;
    machine.debugger_breakpoint_active = 1u;
    machine.debugger_undo_valid = 1u;
    held_key = machine.keyboard_rows[2u];
    tape_elapsed = machine.tape_state.segment_elapsed;
    if (wz_machine_reset(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0u || machine.cpu.program_counter != 0u ||
        machine.master_tick != 0u || machine.paging_7ffd != 0u ||
        machine.memory[0x8000u] != 0xa5u ||
        machine.debugger_breakpoint_active != 0u ||
        machine.debugger_undo_valid != 0u ||
        machine.keyboard_rows[2u] != held_key ||
        machine.tape_mounted == 0u ||
        machine.tape_state.segment_elapsed != tape_elapsed ||
        wz_machine_kempston_read(&machine, 0x001fu) != 0x10u ||
        machine.timing_trace != &trace) {
        wz_machine_destroy(&machine);
        return 1;
    }
    if (wz_machine_reset(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0u || machine.memory[0x8000u] != 0xa5u ||
        machine.tape_mounted == 0u) {
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);
    return 0;
}

int main(void)
{
    if (verify_reset(wz_machine_profile_48k_pal()) != 0 ||
        verify_reset(wz_machine_profile_128k_pal()) != 0) {
        fputs("machine reset contract failed\n", stderr);
        return 1;
    }
    puts("wz_machine_reset contract passed");
    return 0;
}
