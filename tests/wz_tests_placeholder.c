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
#include "diagnostics/wz_trace_file.h"

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

static bool recover_trace(const wz_trace_event_t* event, void* context)
{
    wz_qword_t* last_sequence = (wz_qword_t*)context;
    *last_sequence = event->sequence;
    return true;
}

int main(void)
{
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    wz_machine_t machine;
    wz_machine_t restored;
    wz_scheduler_t scheduler;
    wz_byte_t serialized[65584u];
    wz_state_writer_t writer;
    wz_qword_t first_hash;
    wz_qword_t second_hash;
    wz_trace_sink_t trace_sink;
    wz_headless_runner_t runner;
    unsigned trace_count = 0u;
    unsigned dispatched = 0u;
    wz_trace_file_t trace_file;
    wz_trace_file_t duplicate;
    wz_qword_t recovered_last = 0u;
    size_t recovered_count = 0u;
    FILE* trace_stream;
    const char* trace_path = "wz-trace-regression.bin";

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
        writer.length != 65576u ||
        wz_state_hash_machine(&machine, &first_hash) != WZ_RESULT_OK) {
        fputs("canonical state serialization failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x12u;
    machine.cpu.alternate.f = 0x34u;
    machine.cpu.ix = 0xabcdu;
    machine.cpu.iy = 0x2345u;
    machine.cpu.i = 0x56u;
    machine.cpu.r = 0x78u;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2;
    machine.cpu.halted = 1u;
    wz_state_writer_init(&writer, serialized, sizeof(serialized));
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK ||
        wz_state_deserialize_machine(&restored, serialized, writer.length) != WZ_RESULT_OK ||
        restored.cpu.main.a != 0x12u ||
        restored.cpu.alternate.f != 0x34u ||
        restored.cpu.ix != 0xabcdu ||
        restored.cpu.iy != 0x2345u ||
        restored.cpu.i != 0x56u ||
        restored.cpu.r != 0x78u ||
        restored.cpu.iff1 != 1u ||
        restored.cpu.iff2 != 1u ||
        restored.cpu.interrupt_mode != (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2 ||
        restored.cpu.halted != 1u) {
        fputs("Z80 state round trip failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reinitialization failed\n", stderr);
        return 1;
    }
    wz_state_writer_init(&writer, serialized, sizeof(serialized));
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK ||
        wz_state_hash_machine(&machine, &first_hash) != WZ_RESULT_OK) {
        fputs("canonical state baseline refresh failed\n", stderr);
        return 1;
    }
    machine.memory[0] = 0x42u;
    if (wz_state_hash_machine(&machine, &second_hash) != WZ_RESULT_OK ||
        first_hash == second_hash) {
        fputs("canonical state hash did not reflect machine state\n", stderr);
        return 1;
    }
    if (wz_state_deserialize_machine(&restored, serialized, writer.length) != WZ_RESULT_OK ||
        wz_state_hash_machine(&restored, &second_hash) != WZ_RESULT_OK ||
        first_hash != second_hash) {
        fputs("canonical state round trip failed\n", stderr);
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

    remove(trace_path);
    if (wz_trace_file_create(&trace_file, trace_path, 1u,
                             (wz_dword_t)profile->kind, 0x1234u, UINT32_MAX) != WZ_RESULT_OK ||
        wz_trace_file_create(&duplicate, trace_path, 2u,
                             (wz_dword_t)profile->kind, 0x1234u, UINT32_MAX) == WZ_RESULT_OK) {
        fputs("exclusive trace creation failed\n", stderr);
        return 1;
    }
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    for (wz_qword_t index = 0u; index < 800000u; ++index) {
        wz_trace_emit(&trace_sink, WZ_TRACE_MASTER_TICK_ADVANCED, index);
    }
    if (wz_trace_file_freeze(&trace_file) != WZ_RESULT_OK) {
        fputs("trace freeze failed\n", stderr);
        return 1;
    }
    wz_trace_emit(&trace_sink, WZ_TRACE_MASTER_TICK_ADVANCED, 800001u);
    wz_trace_file_close(&trace_file);
    if (wz_trace_file_recover(trace_path, recover_trace, &recovered_last,
                              &recovered_count) != WZ_RESULT_OK ||
        recovered_count < (8u * 69888u) || recovered_last != 799999u) {
        fputs("trace wrap recovery failed\n", stderr);
        return 1;
    }
    trace_stream = fopen(trace_path, "r+b");
    if (trace_stream == NULL || fseek(trace_stream, 0, SEEK_END) != 0 ||
        ftell(trace_stream) != (long)WZ_TRACE_FILE_SIZE ||
        fseek(trace_stream,
              (long)(WZ_TRACE_HEADER_SIZE +
                     (799999u % ((WZ_TRACE_FILE_SIZE - WZ_TRACE_HEADER_SIZE) /
                                  WZ_TRACE_RECORD_SIZE)) * WZ_TRACE_RECORD_SIZE + 20u),
              SEEK_SET) != 0 || fputc(0, trace_stream) == EOF ||
        fclose(trace_stream) != 0) {
        fputs("trace size or truncation fixture failed\n", stderr);
        return 1;
    }
    recovered_last = 0u;
    if (wz_trace_file_recover(trace_path, recover_trace, &recovered_last,
                              &recovered_count) != WZ_RESULT_OK ||
        recovered_last != 799998u) {
        fputs("incomplete trace record was not rejected\n", stderr);
        return 1;
    }
    remove(trace_path);

    wz_machine_destroy(&machine);
    wz_machine_destroy(&restored);
    return 0;
}
