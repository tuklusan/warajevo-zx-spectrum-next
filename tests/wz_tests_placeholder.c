/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "core/wz_machine.h"
#include "core/wz_scheduler.h"
#include "core/wz_state.h"
#include "core/wz_runner.h"

static void record_event(void* context)
{
    unsigned* value = (unsigned*)context;
    *value += 1u;
}

static void record_trace(const wz_trace_event_t* event, void* context)
{
    unsigned* count = (unsigned*)context;
    if (event->kind == WZ_TRACE_MASTER_TICK_ADVANCED) {
        *count += 1u;
    }
}

int main(void)
{
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    wz_machine_t machine;
    wz_scheduler_t scheduler;
    wz_byte_t serialized[65568u];
    wz_state_writer_t writer;
    wz_qword_t first_hash;
    wz_qword_t second_hash;
    wz_trace_sink_t trace_sink;
    wz_headless_runner_t runner;
    unsigned trace_count = 0u;
    unsigned dispatched = 0u;

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine initialization failed\n", stderr);
        return 1;
    }
    if (machine.master_tick != 0u || machine.profile != profile) {
        fputs("machine did not initialize deterministic state\n", stderr);
        return 1;
    }
    if (wz_profile_cpu_tstate(5u, profile) != 2u ||
        wz_profile_cpu_phase(5u, profile) != 1u) {
        fputs("master-tick conversion failed\n", stderr);
        return 1;
    }

    wz_state_writer_init(&writer, serialized, sizeof(serialized));
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK ||
        writer.length != 65551u ||
        wz_state_hash_machine(&machine, &first_hash) != WZ_RESULT_OK) {
        fputs("canonical state serialization failed\n", stderr);
        return 1;
    }
    machine.memory[0] = 0x42u;
    if (wz_state_hash_machine(&machine, &second_hash) != WZ_RESULT_OK ||
        first_hash == second_hash) {
        fputs("canonical state hash did not reflect machine state\n", stderr);
        return 1;
    }

    wz_trace_sink_init(&trace_sink, record_trace, &trace_count);
    if (wz_headless_runner_init(&runner, &machine, &trace_sink) != WZ_RESULT_OK ||
        wz_headless_runner_advance(&runner, 3u) != WZ_RESULT_OK ||
        machine.master_tick != 3u || trace_count != 3u) {
        fputs("headless runner or trace sink failed\n", stderr);
        return 1;
    }

    wz_scheduler_init(&scheduler);
    if (wz_scheduler_schedule(&scheduler, 10u, WZ_EVENT_EXTERNAL,
                              record_event, &dispatched) != WZ_RESULT_OK ||
        wz_scheduler_schedule(&scheduler, 10u, WZ_EVENT_CPU,
                              record_event, &dispatched) != WZ_RESULT_OK ||
        wz_scheduler_dispatch_next(&scheduler) != WZ_RESULT_OK ||
        dispatched != 1u) {
        fputs("same-tick scheduler ordering failed\n", stderr);
        return 1;
    }

    if (strstr(wz_machine_boot_message(), "Warajevo") == NULL) {
        fputs("bootstrap message does not identify the project\n", stderr);
        return 1;
    }

    wz_machine_destroy(&machine);
    return 0;
}
