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

#include "core/wz_machine_profile.h"
#include "diagnostics/wz_trace_file.h"

#define REQUIRED_FRAMES UINT64_C(8)
#define EVENTS_PER_CPU_TSTATE UINT64_C(1)

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Timing trace density failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

typedef struct {
    wz_master_tick_t first_tick;
    wz_master_tick_t last_tick;
    size_t count;
    bool has_tick;
} density_span_t;

static bool record_span(const wz_trace_event_t* event, void* context)
{
    density_span_t* span = (density_span_t*)context;
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

int main(int argc, char** argv)
{
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    wz_trace_file_t trace_file = {0};
    density_span_t span = {0};
    wz_master_tick_t frame_ticks;
    wz_master_tick_t required_ticks;
    wz_qword_t required_tstates;
    wz_qword_t record_count;
    size_t recovered_count = 0u;
    long file_size;
    int result = 1;

    REQUIRE(argc == 2 && profile != NULL);
    REQUIRE(profile->tstates_per_frame != 0u &&
            profile->master_ticks_per_cpu_tstate != 0u);
    frame_ticks = (wz_master_tick_t)profile->tstates_per_frame *
        profile->master_ticks_per_cpu_tstate;
    REQUIRE(REQUIRED_FRAMES <= UINT64_MAX / frame_ticks);
    required_ticks = REQUIRED_FRAMES * frame_ticks;
    REQUIRE(REQUIRED_FRAMES <= UINT64_MAX / profile->tstates_per_frame);
    required_tstates = REQUIRED_FRAMES * profile->tstates_per_frame;
    REQUIRE(required_tstates <= UINT64_MAX / EVENTS_PER_CPU_TSTATE);
    record_count = required_tstates * EVENTS_PER_CPU_TSTATE + 1u;
    REQUIRE(wz_trace_file_create(&trace_file, argv[1], 1u,
        (wz_dword_t)profile->kind, 0u, UINT32_MAX) == WZ_RESULT_OK);

    for (wz_qword_t index = 0u; index < record_count; ++index) {
        wz_trace_event_t event = {0};
        event.kind = WZ_TRACE_CPU_INSTRUCTION;
        event.sequence = index + 1u;
        event.master_tick = index *
            (wz_qword_t)profile->master_ticks_per_cpu_tstate /
            EVENTS_PER_CPU_TSTATE;
        event.t_states = 1u;
        wz_trace_file_emit(&event, &trace_file);
        REQUIRE(!trace_file.failed);
    }

    REQUIRE(trace_file.record_count == record_count);
    REQUIRE(wz_trace_file_freeze(&trace_file) == WZ_RESULT_OK);
    REQUIRE(fseek(trace_file.file, 0L, SEEK_END) == 0);
    REQUIRE((file_size = ftell(trace_file.file)) == (long)WZ_TRACE_FILE_SIZE);
    REQUIRE(wz_trace_file_recover(argv[1], record_span, &span,
                                  &recovered_count) == WZ_RESULT_OK);
    REQUIRE(span.has_tick && recovered_count == span.count &&
            recovered_count == (size_t)record_count);
    REQUIRE(span.last_tick >= span.first_tick &&
            span.last_tick - span.first_tick >= required_ticks);
    result = 0;
    (void)printf("{\"status\":\"pass\",\"profile\":\"48K PAL\","
        "\"requestedFrames\":%llu,\"frameTicks\":%llu,"
        "\"eventsPerCpuTState\":%llu,\"syntheticRecords\":%llu,"
        "\"recoveredRecords\":%llu,\"retainedFrameSpan\":%llu,"
        "\"fileBytes\":%ld}\n",
        (unsigned long long)REQUIRED_FRAMES,
        (unsigned long long)frame_ticks,
        (unsigned long long)EVENTS_PER_CPU_TSTATE,
        (unsigned long long)record_count,
        (unsigned long long)recovered_count,
        (unsigned long long)((span.last_tick - span.first_tick) /
                             frame_ticks),
        file_size);

cleanup:
    wz_trace_file_close(&trace_file);
    if (result != 0 && argc == 2) (void)remove(argv[1]);
    return result;
}
