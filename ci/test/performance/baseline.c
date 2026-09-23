/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
patent, trademark, and governing-law provisions.
*/

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app/wz_ui_layout.h"
#include "core/audio/wz_audio_mixer.h"
#include "core/wz_machine.h"
#include "core/wz_presentation_snapshot.h"
#include "core/wz_state.h"

#ifndef WZ_PERF_COMPILER_ID
#define WZ_PERF_COMPILER_ID "unknown"
#endif
#ifndef WZ_PERF_COMPILER_VERSION
#define WZ_PERF_COMPILER_VERSION "unknown"
#endif

#define WZ_PERF_REPETITIONS 3u
#define WZ_CPU_STEPS 250005u
#define WZ_RASTER_FRAMES 384u
#define WZ_AUDIO_SAMPLES 2000000u
#define WZ_TAPE_SEGMENT_COUNT 131072u
#define WZ_TAPE_TICKS 4000000u
#define WZ_SNAPSHOT_WRITES 8192u
#define WZ_UI_FRAMES 512u

typedef struct {
    const char* name;
    const char* unit;
    uint64_t operations;
    double seconds;
    double operations_per_second;
    uint64_t fingerprint;
} metric_t;

typedef bool (*metric_run_fn)(void* context, uint64_t* fingerprint,
                              double* seconds);

static uint64_t hash_bytes(const void* data, size_t length)
{
    const unsigned char* bytes = (const unsigned char*)data;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t index = 0u; index < length; ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static double clock_seconds(clock_t value)
{
    return (double)value / (double)CLOCKS_PER_SEC;
}

static bool measured(metric_run_fn run, void* context, metric_t* metric)
{
    double samples[WZ_PERF_REPETITIONS];
    uint64_t expected_fingerprint = 0u;

    for (size_t index = 0u; index < WZ_PERF_REPETITIONS; ++index) {
        uint64_t fingerprint = 0u;
        if (!run(context, &fingerprint, &samples[index])) {
            fprintf(stderr, "Benchmark workload failed: %s, repetition %zu.\n",
                    metric->name, index + 1u);
            return false;
        }
        if (samples[index] <= 0.0) {
            fprintf(stderr, "Benchmark timer resolution was insufficient: %s.\n",
                    metric->name);
            return false;
        }
        if (index == 0u) {
            expected_fingerprint = fingerprint;
        } else if (fingerprint != expected_fingerprint) {
            fprintf(stderr, "Benchmark correctness fingerprint changed: %s.\n",
                    metric->name);
            return false;
        }
    }
    if (samples[0] > samples[1]) {
        double temporary = samples[0]; samples[0] = samples[1]; samples[1] = temporary;
    }
    if (samples[1] > samples[2]) {
        double temporary = samples[1]; samples[1] = samples[2]; samples[2] = temporary;
    }
    if (samples[0] > samples[1]) {
        double temporary = samples[0]; samples[0] = samples[1]; samples[1] = temporary;
    }
    metric->seconds = samples[1];
    metric->operations_per_second =
        (double)metric->operations / metric->seconds;
    metric->fingerprint = expected_fingerprint;
    return true;
}

static bool init_machine(wz_machine_t* machine)
{
    memset(machine, 0, sizeof(*machine));
    return wz_machine_init(machine, wz_machine_profile_48k_pal()) == WZ_RESULT_OK;
}

static void destroy_machine(wz_machine_t* machine)
{
    wz_machine_destroy(machine);
}

static bool cpu_run(void* context, uint64_t* fingerprint, double* seconds)
{
    wz_machine_t* machine = (wz_machine_t*)malloc(sizeof(*machine));
    clock_t start;
    clock_t end;
    bool success = false;

    (void)context;
    if (machine == NULL || !init_machine(machine)) {
        free(machine);
        return false;
    }
    {
        static const wz_byte_t workload[] = {
            0x3eu, 0x01u,             /* LD A,01h */
            0x23u,                   /* INC HL */
            0xafu,                   /* XOR A */
            0x85u,                   /* ADD A,L */
            0x32u, 0x00u, 0xc0u,     /* LD (C000h),A */
            0x34u,                   /* INC (HL) */
            0xc3u, 0x00u, 0x80u      /* JP 8000h */
        };
        for (size_t index = 0u; index < sizeof(workload); ++index) {
            wz_machine_memory_write(machine, (wz_word_t)(0x8000u + index),
                                    workload[index]);
        }
    }
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.main.h = 0xc0u;

    start = clock();
    if (start == (clock_t)-1) {
        destroy_machine(machine);
        free(machine);
        return false;
    }
    for (size_t index = 0u; index < WZ_CPU_STEPS; ++index) {
        if (wz_z80_step(machine) != WZ_RESULT_OK) {
            goto cpu_done;
        }
    }
    end = clock();
    if (machine->cpu.program_counter != 0x8000u || end == (clock_t)-1 ||
        wz_state_hash_machine(machine, fingerprint) != WZ_RESULT_OK) {
        goto cpu_done;
    }
    *seconds = clock_seconds(end - start);
    success = true;

cpu_done:
    destroy_machine(machine);
    free(machine);
    return success;
}

typedef struct {
    wz_machine_t machine;
    wz_raster_buffer_t raster;
    wz_byte_t* pixels;
} raster_context_t;

static bool raster_run(void* context, uint64_t* fingerprint, double* seconds)
{
    raster_context_t* raster = (raster_context_t*)context;
    clock_t start = clock();
    if (start == (clock_t)-1) return false;
    for (size_t frame = 0u; frame < WZ_RASTER_FRAMES; ++frame) {
        if (wz_machine_render_raster(&raster->machine, &raster->raster) !=
            WZ_RESULT_OK) {
            return false;
        }
    }
    clock_t end = clock();
    if (end == (clock_t)-1) return false;
    *seconds = clock_seconds(end - start);
    *fingerprint = hash_bytes(raster->pixels,
                              WZ_RASTER_CANONICAL_WIDTH *
                                  WZ_RASTER_CANONICAL_HEIGHT);
    return true;
}

static void prepare_raster_machine(wz_machine_t* machine)
{
    for (size_t address = 0x4000u; address < 0x5b00u; ++address) {
        wz_machine_memory_write(
            machine, (wz_word_t)address,
            (wz_byte_t)((address * 37u + (address >> 7u) * 11u) & 0xffu));
    }
    machine->border_color = 5u;
    machine->master_tick =
        (wz_master_tick_t)machine->profile->tstates_per_frame *
        machine->profile->master_ticks_per_cpu_tstate * 16u;
}

static bool audio_run(void* context, uint64_t* fingerprint, double* seconds)
{
    wz_ay_t* ay = (wz_ay_t*)context;
    uint64_t digest = UINT64_C(14695981039346656037);
    clock_t start = clock();
    if (start == (clock_t)-1) return false;
    for (size_t index = 0u; index < WZ_AUDIO_SAMPLES; ++index) {
        int32_t beeper = (int32_t)((index * 7919u) % 131071u) - 65535;
        wz_audio_sample_t sample = wz_audio_mixer_sample(beeper, ay);
        digest ^= (uint32_t)sample;
        digest *= UINT64_C(1099511628211);
    }
    clock_t end = clock();
    if (end == (clock_t)-1) return false;
    *seconds = clock_seconds(end - start);
    *fingerprint = digest;
    return true;
}

typedef struct {
    wz_tape_t tape;
    wz_tape_segment_t* segments;
} tape_context_t;

static bool tape_run(void* context, uint64_t* fingerprint, double* seconds)
{
    tape_context_t* tape = (tape_context_t*)context;
    wz_tape_state_t state;
    if (wz_tape_state_init(&state, &tape->tape) != WZ_RESULT_OK ||
        wz_tape_state_set_motor(&state, true) != WZ_RESULT_OK) {
        return false;
    }
    clock_t start = clock();
    if (start == (clock_t)-1) return false;
    for (size_t tick = 0u; tick < WZ_TAPE_TICKS; ++tick) {
        if (wz_tape_state_advance(&state, 1u) != WZ_RESULT_OK) return false;
    }
    clock_t end = clock();
    if (end == (clock_t)-1) return false;
    *seconds = clock_seconds(end - start);
    *fingerprint = (uint64_t)state.segment_index ^
        ((uint64_t)state.segment_elapsed << 17u) ^
        ((uint64_t)state.ear_level << 61u) ^
        ((uint64_t)state.motor_on << 62u) ^
        ((uint64_t)state.at_end << 63u);
    return true;
}

typedef struct {
    wz_machine_t machine;
    wz_machine_t restored;
    wz_byte_t* bytes;
    wz_byte_t* expected;
} snapshot_context_t;

static bool snapshot_run(void* context, uint64_t* fingerprint, double* seconds)
{
    snapshot_context_t* snapshot = (snapshot_context_t*)context;
    wz_state_writer_t writer;
    wz_qword_t original_hash;
    wz_qword_t restored_hash;
    size_t expected_length = 0u;

    wz_state_writer_init(&writer, snapshot->expected, WZ_STATE_MACHINE_LENGTH);
    if (wz_state_serialize_machine(&snapshot->machine, &writer) != WZ_RESULT_OK) {
        return false;
    }
    expected_length = writer.length;
    wz_state_writer_init(&writer, snapshot->bytes, WZ_STATE_MACHINE_LENGTH);
    clock_t start = clock();
    if (start == (clock_t)-1) return false;
    for (size_t index = 0u; index < WZ_SNAPSHOT_WRITES; ++index) {
        wz_state_writer_init(&writer, snapshot->bytes, WZ_STATE_MACHINE_LENGTH);
        if (wz_state_serialize_machine(&snapshot->machine, &writer) !=
                WZ_RESULT_OK || writer.length != expected_length) {
            return false;
        }
    }
    clock_t end = clock();
    if (end == (clock_t)-1 ||
        memcmp(snapshot->bytes, snapshot->expected, expected_length) != 0 ||
        wz_machine_init(&snapshot->restored, wz_machine_profile_48k_pal()) !=
            WZ_RESULT_OK ||
        wz_state_deserialize_machine(&snapshot->restored, snapshot->bytes,
                                     expected_length) != WZ_RESULT_OK ||
        wz_state_hash_machine(&snapshot->machine, &original_hash) !=
            WZ_RESULT_OK ||
        wz_state_hash_machine(&snapshot->restored, &restored_hash) !=
            WZ_RESULT_OK || original_hash != restored_hash) {
        if (snapshot->restored.profile != NULL) {
            destroy_machine(&snapshot->restored);
        }
        return false;
    }
    destroy_machine(&snapshot->restored);
    *seconds = clock_seconds(end - start);
    *fingerprint = hash_bytes(snapshot->bytes, expected_length);
    return true;
}

typedef struct {
    wz_ui_layout_state_t layout;
    wz_raster_buffer_t source;
    wz_presentation_snapshot_t snapshot;
    wz_byte_t* source_pixels;
    wz_byte_t* snapshot_pixels;
    char status_line[WZ_UI_STATUS_CAPACITY];
    char status_panel[WZ_UI_STATUS_CAPACITY];
} ui_context_t;

static bool ui_run(void* context, uint64_t* fingerprint, double* seconds)
{
    ui_context_t* ui = (ui_context_t*)context;
    wz_ui_layout_state_init(&ui->layout);
    if (wz_raster_buffer_init(&ui->source, WZ_RASTER_CANONICAL_WIDTH,
                              WZ_RASTER_CANONICAL_HEIGHT, ui->source_pixels,
                              WZ_RASTER_CANONICAL_WIDTH *
                                  WZ_RASTER_CANONICAL_HEIGHT) != WZ_RESULT_OK ||
        wz_raster_buffer_clear(&ui->source, WZ_RASTER_BLANKING) !=
            WZ_RESULT_OK ||
        wz_presentation_snapshot_init(
            &ui->snapshot, WZ_RASTER_CANONICAL_WIDTH,
            WZ_RASTER_CANONICAL_HEIGHT, ui->snapshot_pixels,
            WZ_RASTER_CANONICAL_WIDTH * WZ_RASTER_CANONICAL_HEIGHT) !=
            WZ_RESULT_OK) {
        return false;
    }
    for (size_t index = 0u; index < WZ_RASTER_CANONICAL_WIDTH *
                                    WZ_RASTER_CANONICAL_HEIGHT; ++index) {
        ui->source_pixels[index] = (wz_byte_t)(
            (index * 13u + index / WZ_RASTER_CANONICAL_WIDTH) %
            (WZ_RASTER_BLANKING + 1u));
    }

    clock_t start = clock();
    if (start == (clock_t)-1) return false;
    for (size_t frame = 0u; frame < WZ_UI_FRAMES; ++frame) {
        wz_ui_layout_status_line(&ui->layout, ui->status_line,
                                 sizeof(ui->status_line));
        wz_ui_layout_status_panel(&ui->layout, ui->status_panel,
                                  sizeof(ui->status_panel));
        if (wz_presentation_snapshot_publish(&ui->snapshot, &ui->source) !=
            WZ_RESULT_OK) {
            return false;
        }
    }
    clock_t end = clock();
    if (end == (clock_t)-1 ||
        wz_presentation_snapshot_sequence(&ui->snapshot) != WZ_UI_FRAMES) {
        return false;
    }
    uint64_t digest = hash_bytes(ui->status_line, strlen(ui->status_line));
    digest ^= hash_bytes(ui->status_panel, strlen(ui->status_panel));
    digest *= UINT64_C(1099511628211);
    digest ^= hash_bytes(ui->snapshot_pixels,
                         WZ_RASTER_CANONICAL_WIDTH *
                             WZ_RASTER_CANONICAL_HEIGHT);
    digest ^= wz_presentation_snapshot_sequence(&ui->snapshot);
    *fingerprint = digest;
    *seconds = clock_seconds(end - start);
    return true;
}

static const char* environment_or(const char* key, const char* fallback)
{
    const char* value = getenv(key);
    return value == NULL || value[0] == '\0' ? fallback : value;
}

static bool run_metric(metric_t* metric, metric_run_fn run, void* context)
{
    return measured(run, context, metric);
}

int main(void)
{
    metric_t metrics[] = {
        {"cpu_execution", "instructions", WZ_CPU_STEPS, 0.0, 0.0, 0u},
        {"raster_generation", "frames", WZ_RASTER_FRAMES, 0.0, 0.0, 0u},
        {"audio_mixing", "samples", WZ_AUDIO_SAMPLES, 0.0, 0.0, 0u},
        {"tape_playback", "master_ticks", WZ_TAPE_TICKS, 0.0, 0.0, 0u},
        {"snapshot_serialization", "snapshots", WZ_SNAPSHOT_WRITES, 0.0, 0.0, 0u},
        {"ui_presentation", "frames", WZ_UI_FRAMES, 0.0, 0.0, 0u},
    };
    raster_context_t raster;
    tape_context_t tape;
    wz_ay_t ay;
    snapshot_context_t snapshot;
    ui_context_t ui;
    bool initialized = false;
    int result = 1;

    memset(&raster, 0, sizeof(raster));
    memset(&tape, 0, sizeof(tape));
    memset(&snapshot, 0, sizeof(snapshot));
    memset(&ui, 0, sizeof(ui));

    raster.pixels = (wz_byte_t*)malloc(
        WZ_RASTER_CANONICAL_WIDTH * WZ_RASTER_CANONICAL_HEIGHT);
    tape.segments = (wz_tape_segment_t*)malloc(
        sizeof(*tape.segments) * WZ_TAPE_SEGMENT_COUNT);
    snapshot.bytes = (wz_byte_t*)malloc(WZ_STATE_MACHINE_LENGTH);
    snapshot.expected = (wz_byte_t*)malloc(WZ_STATE_MACHINE_LENGTH);
    ui.source_pixels = (wz_byte_t*)malloc(
        WZ_RASTER_CANONICAL_WIDTH * WZ_RASTER_CANONICAL_HEIGHT);
    ui.snapshot_pixels = (wz_byte_t*)malloc(
        WZ_RASTER_CANONICAL_WIDTH * WZ_RASTER_CANONICAL_HEIGHT);
    if (raster.pixels == NULL || tape.segments == NULL ||
        snapshot.bytes == NULL || snapshot.expected == NULL ||
        ui.source_pixels == NULL || ui.snapshot_pixels == NULL ||
        !init_machine(&raster.machine) || !init_machine(&snapshot.machine)) {
        goto cleanup;
    }
    initialized = true;
    prepare_raster_machine(&raster.machine);
    if (wz_raster_buffer_init(&raster.raster, WZ_RASTER_CANONICAL_WIDTH,
                              WZ_RASTER_CANONICAL_HEIGHT, raster.pixels,
                              WZ_RASTER_CANONICAL_WIDTH *
                                  WZ_RASTER_CANONICAL_HEIGHT) != WZ_RESULT_OK) {
        goto cleanup;
    }

    for (size_t index = 0u; index < WZ_TAPE_SEGMENT_COUNT; ++index) {
        tape.segments[index].duration = 8u + (wz_master_tick_t)(index % 57u);
        tape.segments[index].ear_level = (wz_byte_t)((index / 3u) & 1u);
    }
    if (wz_tape_mount(&tape.tape, tape.segments, WZ_TAPE_SEGMENT_COUNT) !=
        WZ_RESULT_OK) {
        goto cleanup;
    }

    wz_ay_init(&ay);
    if (wz_ay_select_register(&ay, 7u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x38u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 8u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x0fu, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 9u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x0bu, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 10u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x07u, 0u) != WZ_RESULT_OK) {
        goto cleanup;
    }

    if (!run_metric(&metrics[0], cpu_run, NULL) ||
        !run_metric(&metrics[1], raster_run, &raster) ||
        !run_metric(&metrics[2], audio_run, &ay) ||
        !run_metric(&metrics[3], tape_run, &tape) ||
        !run_metric(&metrics[4], snapshot_run, &snapshot) ||
        !run_metric(&metrics[5], ui_run, &ui)) {
        fprintf(stderr, "Performance workload failed or was nondeterministic.\n");
        goto cleanup;
    }

    printf("{\"schema\":1,\"status\":\"pass\",\"commit\":\"%s\","
           "\"runner\":\"%s\",\"architecture\":\"%s\","
           "\"timestamp\":\"%s\","
           "\"compiler\":\"%s %s\",\"metrics\":[",
           environment_or("WZ_PERF_COMMIT", "unspecified"),
           environment_or("WZ_PERF_RUNNER", "unspecified"),
           environment_or("WZ_PERF_ARCH", "unspecified"),
           environment_or("WZ_PERF_TIMESTAMP", "unspecified"),
           WZ_PERF_COMPILER_ID, WZ_PERF_COMPILER_VERSION);
    for (size_t index = 0u; index < sizeof(metrics) / sizeof(metrics[0]); ++index) {
        const metric_t* metric = &metrics[index];
        printf("%s{\"name\":\"%s\",\"unit\":\"%s\","
               "\"operations\":\"%" PRIu64 "\",\"median_cpu_seconds\":%.9f,"
               "\"operations_per_second\":%.3f,\"fingerprint\":\"%016" PRIx64 "\"}",
               index == 0u ? "" : ",", metric->name, metric->unit,
               metric->operations, metric->seconds,
               metric->operations_per_second, metric->fingerprint);
    }
    printf("]}\n");
    result = 0;

cleanup:
    if (initialized) {
        destroy_machine(&snapshot.machine);
        destroy_machine(&raster.machine);
    }
    free(raster.pixels);
    free(tape.segments);
    free(snapshot.bytes);
    free(snapshot.expected);
    free(ui.source_pixels);
    free(ui.snapshot_pixels);
    return result;
}
