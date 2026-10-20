/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdio.h>
#include <string.h>

#include "core/wz_machine_profile.h"
#include "diagnostics/wz_trace_file.h"

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Trace recovery fixture failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

static int build_path(char* output, size_t capacity,
                      const char* directory, const char* name)
{
    int written;
    if (output == NULL || directory == NULL || name == NULL) return 0;
    written = snprintf(output, capacity, "%s/%s", directory, name);
    return written > 0 && (size_t)written < capacity;
}

static void emit_marker(wz_trace_file_t* trace, wz_qword_t sequence,
                        wz_master_tick_t tick, wz_byte_t value)
{
    wz_trace_event_t event = {0};
    event.kind = WZ_TRACE_DEVELOPER_MARKER;
    event.sequence = sequence;
    event.master_tick = tick;
    event.address = (wz_word_t)(0x4000u + value);
    event.program_counter = (wz_word_t)(0x8000u + value);
    event.value = value;
    wz_trace_file_emit(&event, trace);
}

static int leave_uncommitted_tail(const char* path)
{
    wz_byte_t partial_record[8] = {
        (wz_byte_t)WZ_TRACE_RECORD_SIZE,
        (wz_byte_t)WZ_TRACE_DEVELOPER_MARKER,
        0u, 0u, 2u, 0u, 0u, 0u
    };
    FILE* file = fopen(path, "r+b");
    int ok;
    if (file == NULL) return 0;
    ok = fseek(file,
            (long)(WZ_TRACE_HEADER_SIZE + 2u * WZ_TRACE_RECORD_SIZE),
            SEEK_SET) == 0 &&
        fwrite(partial_record, 1u, sizeof(partial_record), file) ==
            sizeof(partial_record) && fflush(file) == 0;
    if (fclose(file) != 0) ok = 0;
    return ok;
}

int main(int argc, char** argv)
{
    wz_trace_file_t wrapped = {0};
    wz_trace_file_t abrupt = {0};
    char wrapped_path[1024] = {0};
    char abrupt_path[1024] = {0};
    int result = 1;

    REQUIRE(argc == 2);
    REQUIRE(build_path(wrapped_path, sizeof(wrapped_path), argv[1],
                       "wrapped-sequence.wztrace"));
    REQUIRE(build_path(abrupt_path, sizeof(abrupt_path), argv[1],
                       "abrupt-append.wztrace"));
    (void)remove(wrapped_path);
    (void)remove(abrupt_path);

    REQUIRE(wz_trace_file_create(&wrapped, wrapped_path, 1u,
            (wz_dword_t)WZ_MACHINE_48K_PAL, 0u,
            UINT32_C(1) << WZ_TRACE_DEVELOPER_MARKER) == WZ_RESULT_OK);
    emit_marker(&wrapped, UINT64_C(0xfffffffd), 1000u, 1u);
    emit_marker(&wrapped, UINT64_C(0xfffffffe), 1002u, 2u);
    emit_marker(&wrapped, UINT64_C(0xffffffff), 1005u, 3u);
    emit_marker(&wrapped, UINT64_C(0x100000000), 1011u, 4u);
    emit_marker(&wrapped, UINT64_C(0x100000001), 1018u, 5u);
    REQUIRE(!wrapped.failed && wrapped.record_count == 3u);
    wz_trace_file_close(&wrapped);

    REQUIRE(wz_trace_file_create(&abrupt, abrupt_path, 2u,
            (wz_dword_t)WZ_MACHINE_48K_PAL, 0u,
            UINT32_C(1) << WZ_TRACE_DEVELOPER_MARKER) == WZ_RESULT_OK);
    emit_marker(&abrupt, 42u, 77u, 4u);
    emit_marker(&abrupt, 43u, 78u, 5u);
    REQUIRE(!abrupt.failed && abrupt.record_count == 2u);
    wz_trace_file_close(&abrupt);
    REQUIRE(leave_uncommitted_tail(abrupt_path));
    result = 0;

cleanup:
    if (wrapped.file != NULL) wz_trace_file_close(&wrapped);
    if (abrupt.file != NULL) wz_trace_file_close(&abrupt);
    if (result != 0) {
        if (wrapped_path[0] != '\0') (void)remove(wrapped_path);
        if (abrupt_path[0] != '\0') (void)remove(abrupt_path);
    }
    return result;
}
