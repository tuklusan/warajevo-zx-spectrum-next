/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/wz_machine.h"
#include "core/wz_z80.h"
#include "diagnostics/wz_trace_file.h"

#define REQUIRED_FRAMES UINT64_C(8)
#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Timing trace retention failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

typedef struct {
    wz_master_tick_t first_tick;
    wz_master_tick_t last_tick;
    size_t count;
    bool has_tick;
} recovered_span_t;

static bool record_span(const wz_trace_event_t* event, void* context)
{
    recovered_span_t* span = (recovered_span_t*)context;
    if (event == NULL || span == NULL) return false;
    if (!span->has_tick) {
        span->first_tick = event->master_tick;
        span->has_tick = true;
    }
    if (span->count != 0u && event->master_tick < span->last_tick) {
        return false;
    }
    span->last_tick = event->master_tick;
    ++span->count;
    return true;
}

static unsigned char* read_rom(const char* path, size_t* length)
{
    FILE* file = fopen(path, "rb");
    long size;
    unsigned char* data;
    if (file == NULL || fseek(file, 0L, SEEK_END) != 0 ||
        (size = ftell(file)) != (long)WZ_48K_ROM_SIZE ||
        fseek(file, 0L, SEEK_SET) != 0) {
        if (file != NULL) (void)fclose(file);
        return NULL;
    }
    data = (unsigned char*)malloc((size_t)size);
    if (data == NULL || fread(data, 1u, (size_t)size, file) != (size_t)size) {
        free(data);
        (void)fclose(file);
        return NULL;
    }
    if (fclose(file) != 0) {
        free(data);
        return NULL;
    }
    *length = (size_t)size;
    return data;
}

int main(int argc, char** argv)
{
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    wz_machine_t machine;
    wz_trace_file_t trace_file = {0};
    wz_trace_sink_t trace_sink;
    recovered_span_t span = {0};
    unsigned char* rom = NULL;
    size_t rom_length = 0u;
    size_t recovered_count = 0u;
    wz_master_tick_t frame_ticks;
    wz_master_tick_t required_ticks;
    long file_size;
    bool machine_initialized = false;
    int result = 1;

    REQUIRE(argc == 3 && profile != NULL);
    frame_ticks = (wz_master_tick_t)profile->tstates_per_frame *
        profile->master_ticks_per_cpu_tstate;
    REQUIRE(frame_ticks != 0u && REQUIRED_FRAMES <= UINT64_MAX / frame_ticks);
    required_ticks = REQUIRED_FRAMES * frame_ticks;
    rom = read_rom(argv[1], &rom_length);
    REQUIRE(rom != NULL);
    REQUIRE(wz_machine_init(&machine, profile) == WZ_RESULT_OK);
    machine_initialized = true;
    REQUIRE(wz_machine_load_48k_rom(&machine, rom, rom_length) == WZ_RESULT_OK);
    REQUIRE(wz_trace_file_create(&trace_file, argv[2], 1u,
        (wz_dword_t)profile->kind,
        wz_machine_rom_identity(rom, rom_length), UINT32_MAX) == WZ_RESULT_OK);
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    wz_machine_set_timing_trace(&machine, &trace_sink);

    while (machine.master_tick < required_ticks ||
           trace_file.last_master_tick < required_ticks) {
        REQUIRE(wz_z80_step(&machine) == WZ_RESULT_OK);
        REQUIRE(!trace_file.failed);
    }
    REQUIRE(machine.master_tick >= required_ticks);
    REQUIRE(wz_trace_file_freeze(&trace_file) == WZ_RESULT_OK);
    REQUIRE(fseek(trace_file.file, 0L, SEEK_END) == 0);
    REQUIRE((file_size = ftell(trace_file.file)) == (long)WZ_TRACE_FILE_SIZE);
    REQUIRE(wz_trace_file_recover(argv[2], record_span, &span,
                                  &recovered_count) == WZ_RESULT_OK);
    REQUIRE(span.has_tick && recovered_count == span.count && span.count > 0u);
    result = span.last_tick >= span.first_tick &&
        span.last_tick - span.first_tick >= required_ticks ? 0 : 1;
    (void)printf("{\"status\":\"%s\",\"requestedFrames\":%llu,"
        "\"frameTicks\":%llu,\"machineTicks\":%llu,"
        "\"retainedFrameSpan\":%llu,\"retainedRecords\":%llu,"
        "\"ringGenerations\":%llu,\"fileBytes\":%ld}\n",
        result == 0 ? "pass" : "fail",
        (unsigned long long)REQUIRED_FRAMES,
        (unsigned long long)frame_ticks,
        (unsigned long long)machine.master_tick,
        (unsigned long long)(span.last_tick >= span.first_tick ?
            (span.last_tick - span.first_tick) / frame_ticks : 0u),
        (unsigned long long)span.count,
        (unsigned long long)trace_file.generation, file_size);
    if (result != 0) {
        (void)fprintf(stderr,
            "retention shortfall: first=%llu last=%llu required=%llu "
            "records=%llu slots=%llu generations=%llu\n",
            (unsigned long long)span.first_tick,
            (unsigned long long)span.last_tick,
            (unsigned long long)required_ticks,
            (unsigned long long)span.count,
            (unsigned long long)trace_file.record_count,
            (unsigned long long)trace_file.generation);
    }

cleanup:
    wz_trace_file_close(&trace_file);
    if (machine_initialized) wz_machine_destroy(&machine);
    free(rom);
    if (result != 0 && argc == 3) (void)remove(argv[2]);
    return result;
}
