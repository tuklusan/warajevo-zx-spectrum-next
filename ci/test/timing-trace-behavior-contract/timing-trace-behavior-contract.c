/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/wz_machine.h"
#include "core/wz_state.h"
#include "core/wz_z80.h"
#include "diagnostics/wz_trace_file.h"

#define STEP_COUNT 4096u
#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Timing trace behavior contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

typedef struct {
    uint64_t hash;
    uint64_t count;
    uint64_t last_sequence;
    wz_master_tick_t last_tick;
    bool ordered;
} event_digest_t;

static void digest_u64(event_digest_t* digest, uint64_t value)
{
    unsigned int shift;
    for (shift = 0u; shift < 64u; shift += 8u) {
        digest->hash ^= (value >> shift) & UINT64_C(0xff);
        digest->hash *= UINT64_C(1099511628211);
    }
}

static void digest_event(const wz_trace_event_t* event, void* context)
{
    event_digest_t* digest = (event_digest_t*)context;
    if (digest == NULL || event == NULL) return;
    if (digest->count != 0u &&
        (event->sequence <= digest->last_sequence ||
         event->master_tick < digest->last_tick)) {
        digest->ordered = false;
    }
    digest_u64(digest, (uint64_t)event->kind);
    digest_u64(digest, (uint64_t)event->master_tick);
    digest_u64(digest, event->sequence);
    digest_u64(digest, event->address);
    digest_u64(digest, event->program_counter);
    digest_u64(digest, event->stack_pointer);
    digest_u64(digest, event->register_snapshot);
    digest_u64(digest, event->value);
    digest_u64(digest, event->auxiliary);
    digest_u64(digest, event->cycle);
    digest_u64(digest, event->t_states);
    digest->last_sequence = event->sequence;
    digest->last_tick = event->master_tick;
    ++digest->count;
}

static unsigned char* read_rom(const char* path, size_t* length)
{
    FILE* file = fopen(path, "rb");
    long size;
    unsigned char* bytes;
    if (file == NULL || fseek(file, 0L, SEEK_END) != 0 ||
        (size = ftell(file)) != (long)WZ_48K_ROM_SIZE ||
        fseek(file, 0L, SEEK_SET) != 0) {
        if (file != NULL) (void)fclose(file);
        return NULL;
    }
    bytes = (unsigned char*)malloc((size_t)size);
    if (bytes == NULL || fread(bytes, 1u, (size_t)size, file) != (size_t)size) {
        free(bytes);
        (void)fclose(file);
        return NULL;
    }
    if (fclose(file) != 0) {
        free(bytes);
        return NULL;
    }
    *length = (size_t)size;
    return bytes;
}

static bool init_machine(wz_machine_t* machine, const unsigned char* rom,
                         size_t rom_length)
{
    if (wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        return false;
    }
    if (wz_machine_load_48k_rom(machine, rom, rom_length) != WZ_RESULT_OK) {
        wz_machine_destroy(machine);
        return false;
    }
    return true;
}

static bool run_steps(wz_machine_t* machine, size_t count)
{
    size_t step;
    for (step = 0u; step < count; ++step) {
        if (wz_z80_step(machine) != WZ_RESULT_OK) return false;
    }
    return true;
}

static bool machine_hash(const wz_machine_t* machine, wz_qword_t* hash)
{
    return wz_state_hash_machine(machine, hash) == WZ_RESULT_OK;
}

int main(int argc, char** argv)
{
    wz_machine_t* baseline = NULL;
    wz_machine_t* toggled = NULL;
    wz_machine_t* traced_a = NULL;
    wz_machine_t* traced_b = NULL;
    wz_machine_t* failed_machine = NULL;
    wz_machine_t* file_traced = NULL;
    wz_trace_sink_t toggle_sink;
    wz_trace_sink_t sink_a;
    wz_trace_sink_t sink_b;
    wz_trace_sink_t file_sink;
    wz_trace_file_t failed_trace;
    wz_trace_file_t file_trace;
    event_digest_t toggle_digest = {0u, 0u, 0u, 0u, true};
    event_digest_t digest_a = {0u, 0u, 0u, 0u, true};
    event_digest_t digest_b = {0u, 0u, 0u, 0u, true};
    unsigned char* rom = NULL;
    size_t rom_length = 0u;
    wz_qword_t baseline_hash;
    wz_qword_t toggled_hash;
    wz_qword_t traced_a_hash;
    wz_qword_t traced_b_hash;
    wz_qword_t failed_hash;
    wz_qword_t baseline_mid_hash;
    wz_qword_t file_mid_hash;
    wz_qword_t file_final_hash;
    wz_qword_t before_freeze_hash;
    wz_qword_t after_freeze_hash;
    wz_qword_t before_toggle_hash;
    wz_qword_t after_toggle_hash;
    wz_qword_t before_detach_hash;
    wz_qword_t after_detach_hash;
    char failure_path[1024] = {0};
    char trace_path[1024] = {0};
    int path_length;
    size_t initialized_machine_count = 0u;
    int result = 1;

    memset(&failed_trace, 0, sizeof(failed_trace));
    memset(&file_trace, 0, sizeof(file_trace));
    if (argc != 3 || argv[2][0] == '\0') return 2;
    toggle_digest.hash = UINT64_C(14695981039346656037);
    digest_a.hash = UINT64_C(14695981039346656037);
    digest_b.hash = UINT64_C(14695981039346656037);
    rom = read_rom(argv[1], &rom_length);
    REQUIRE(rom != NULL);
    baseline = (wz_machine_t*)calloc(1u, sizeof(*baseline));
    toggled = (wz_machine_t*)calloc(1u, sizeof(*toggled));
    traced_a = (wz_machine_t*)calloc(1u, sizeof(*traced_a));
    traced_b = (wz_machine_t*)calloc(1u, sizeof(*traced_b));
    failed_machine = (wz_machine_t*)calloc(1u, sizeof(*failed_machine));
    file_traced = (wz_machine_t*)calloc(1u, sizeof(*file_traced));
    REQUIRE(baseline != NULL && toggled != NULL && traced_a != NULL &&
            traced_b != NULL && failed_machine != NULL && file_traced != NULL);
    REQUIRE(init_machine(baseline, rom, rom_length));
    ++initialized_machine_count;
    REQUIRE(init_machine(toggled, rom, rom_length));
    ++initialized_machine_count;
    REQUIRE(init_machine(traced_a, rom, rom_length));
    ++initialized_machine_count;
    REQUIRE(init_machine(traced_b, rom, rom_length));
    ++initialized_machine_count;
    REQUIRE(init_machine(failed_machine, rom, rom_length));
    ++initialized_machine_count;
    REQUIRE(init_machine(file_traced, rom, rom_length));
    ++initialized_machine_count;

    wz_trace_sink_init(&toggle_sink, digest_event, &toggle_digest);
    wz_trace_sink_init(&sink_a, digest_event, &digest_a);
    wz_trace_sink_init(&sink_b, digest_event, &digest_b);
    wz_trace_sink_init(&file_sink, wz_trace_file_emit, &file_trace);
    REQUIRE(machine_hash(toggled, &before_toggle_hash));
    wz_machine_set_timing_trace(toggled, &toggle_sink);
    REQUIRE(machine_hash(toggled, &after_toggle_hash));
    REQUIRE(before_toggle_hash == after_toggle_hash);
    wz_machine_set_timing_trace(traced_a, &sink_a);
    wz_machine_set_timing_trace(traced_b, &sink_b);

    path_length = snprintf(trace_path, sizeof(trace_path),
                           "%s/state.wztrace", argv[2]);
    REQUIRE(path_length > 0 && (size_t)path_length < sizeof(trace_path));
    REQUIRE(wz_trace_file_create(&file_trace, trace_path, 2u,
        (wz_dword_t)wz_machine_profile_48k_pal()->kind,
        wz_machine_rom_identity(rom, rom_length), UINT32_MAX) == WZ_RESULT_OK);
    wz_machine_set_timing_trace(file_traced, &file_sink);

    path_length = snprintf(failure_path, sizeof(failure_path),
                           "%s/missing/trace.wztrace", argv[2]);
    REQUIRE(path_length > 0 && (size_t)path_length < sizeof(failure_path));
    REQUIRE(machine_hash(failed_machine, &before_detach_hash));
    REQUIRE(wz_trace_file_create(&failed_trace, failure_path, 1u,
        (wz_dword_t)wz_machine_profile_48k_pal()->kind,
        wz_machine_rom_identity(rom, rom_length), UINT32_MAX) ==
        WZ_RESULT_TRACE_FAILURE);
    REQUIRE(failed_trace.file == NULL);
    REQUIRE(machine_hash(failed_machine, &after_detach_hash));
    REQUIRE(before_detach_hash == after_detach_hash);
    REQUIRE(run_steps(baseline, STEP_COUNT / 2u));
    REQUIRE(run_steps(toggled, STEP_COUNT / 2u));
    REQUIRE(toggle_digest.count > 0u && toggle_digest.ordered);
    REQUIRE(machine_hash(baseline, &baseline_mid_hash));
    REQUIRE(machine_hash(toggled, &toggled_hash));
    REQUIRE(baseline_mid_hash == toggled_hash);
    REQUIRE(machine_hash(toggled, &before_detach_hash));
    wz_machine_set_timing_trace(toggled, NULL);
    REQUIRE(machine_hash(toggled, &after_detach_hash));
    REQUIRE(before_detach_hash == after_detach_hash);
    REQUIRE(run_steps(file_traced, STEP_COUNT / 2u));
    REQUIRE(file_trace.generation > 0u);
    REQUIRE(machine_hash(file_traced, &file_mid_hash));
    REQUIRE(file_mid_hash == baseline_mid_hash);
    REQUIRE(machine_hash(file_traced, &before_freeze_hash));
    REQUIRE(wz_trace_file_freeze(&file_trace) == WZ_RESULT_OK);
    REQUIRE(machine_hash(file_traced, &after_freeze_hash));
    REQUIRE(before_freeze_hash == after_freeze_hash && file_trace.frozen);
    REQUIRE(run_steps(baseline, STEP_COUNT / 2u));
    REQUIRE(run_steps(toggled, STEP_COUNT / 2u));
    REQUIRE(run_steps(file_traced, STEP_COUNT / 2u));
    REQUIRE(run_steps(traced_a, STEP_COUNT));
    REQUIRE(run_steps(traced_b, STEP_COUNT));
    REQUIRE(run_steps(failed_machine, STEP_COUNT));
    REQUIRE(digest_a.count > 0u && digest_a.ordered);
    REQUIRE(digest_b.count == digest_a.count && digest_b.ordered);
    REQUIRE(digest_b.hash == digest_a.hash);
    REQUIRE(machine_hash(baseline, &baseline_hash));
    REQUIRE(machine_hash(toggled, &toggled_hash));
    REQUIRE(machine_hash(traced_a, &traced_a_hash));
    REQUIRE(machine_hash(traced_b, &traced_b_hash));
    REQUIRE(machine_hash(failed_machine, &failed_hash));
    REQUIRE(machine_hash(file_traced, &file_final_hash));
    REQUIRE(baseline_hash == toggled_hash && baseline_hash == traced_a_hash &&
            baseline_hash == traced_b_hash && baseline_hash == failed_hash &&
            baseline_hash == file_final_hash);

    (void)puts("PASS timing trace state neutrality, deterministic ordering, wrap, freeze, enable/disable, and file failure");
    result = 0;

cleanup:
    if (failed_machine != NULL) wz_machine_set_timing_trace(failed_machine, NULL);
    if (file_traced != NULL) wz_machine_set_timing_trace(file_traced, NULL);
    wz_trace_file_close(&failed_trace);
    wz_trace_file_close(&file_trace);
    if (initialized_machine_count > 0u) wz_machine_destroy(baseline);
    if (initialized_machine_count > 1u) wz_machine_destroy(toggled);
    if (initialized_machine_count > 2u) wz_machine_destroy(traced_a);
    if (initialized_machine_count > 3u) wz_machine_destroy(traced_b);
    if (initialized_machine_count > 4u) wz_machine_destroy(failed_machine);
    if (initialized_machine_count > 5u) wz_machine_destroy(file_traced);
    free(baseline);
    free(toggled);
    free(traced_a);
    free(traced_b);
    free(failed_machine);
    free(file_traced);
    free(rom);
    if (failure_path[0] != '\0') (void)remove(failure_path);
    if (trace_path[0] != '\0') (void)remove(trace_path);
    return result;
}
