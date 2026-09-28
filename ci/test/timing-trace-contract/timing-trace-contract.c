/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "diagnostics/wz_trace_file.h"

#define TEST_EVENT_MASK (UINT32_C(1) << WZ_TRACE_DEVELOPER_MARKER)
#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Timing trace contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

typedef struct {
    wz_trace_event_t events[8];
    size_t count;
} recovered_events_t;

static bool collect_event(const wz_trace_event_t* event, void* context)
{
    recovered_events_t* recovered = (recovered_events_t*)context;
    if (recovered == NULL || event == NULL ||
        recovered->count >= sizeof(recovered->events) / sizeof(recovered->events[0])) {
        return false;
    }
    recovered->events[recovered->count++] = *event;
    return true;
}

static void emit_event(wz_trace_file_t* trace, wz_qword_t sequence,
                       wz_master_tick_t tick, wz_trace_event_kind_t kind,
                       wz_byte_t value)
{
    wz_trace_event_t event;
    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.sequence = sequence;
    event.master_tick = tick;
    event.address = (wz_word_t)(0x4000u + value);
    event.program_counter = (wz_word_t)(0x8000u + value);
    event.value = value;
    wz_trace_file_emit(&event, trace);
}

static int build_path(char* output, size_t capacity,
                      const char* directory, const char* name)
{
    int written;
    if (output == NULL || directory == NULL || name == NULL ||
        directory[0] == '\0') return 0;
    written = snprintf(output, capacity, "%s/%s", directory, name);
    return written > 0 && (size_t)written < capacity;
}

static int corrupt_commit(const char* path, long offset)
{
    static const unsigned char invalid_commit[4] = {0u, 0u, 0u, 0u};
    FILE* file = fopen(path, "r+b");
    int write_ok;
    int close_ok;
    if (file == NULL) return 0;
    write_ok = fseek(file, offset, SEEK_SET) == 0 &&
        fwrite(invalid_commit, 1u, sizeof(invalid_commit), file) ==
            sizeof(invalid_commit);
    close_ok = fclose(file) == 0;
    return write_ok && close_ok;
}

int main(int argc, char** argv)
{
    wz_trace_file_t first;
    wz_trace_file_t second;
    wz_trace_file_t duplicate;
    recovered_events_t recovered;
    char first_path[1024] = {0};
    char second_path[1024] = {0};
    char partial_path[1024] = {0};
    FILE* file = NULL;
    long file_size;
    size_t recovered_count = 0u;
    int result = 1;

    memset(&first, 0, sizeof(first));
    memset(&second, 0, sizeof(second));
    memset(&duplicate, 0, sizeof(duplicate));
    memset(&recovered, 0, sizeof(recovered));
    REQUIRE(argc == 2);
    REQUIRE(build_path(first_path, sizeof(first_path), argv[1], "first.wztrace"));
    REQUIRE(build_path(second_path, sizeof(second_path), argv[1], "second.wztrace"));
    REQUIRE(build_path(partial_path, sizeof(partial_path), argv[1], "partial.wztrace"));
    (void)remove(first_path);
    (void)remove(second_path);
    (void)remove(partial_path);

    REQUIRE(wz_trace_file_create(&first, first_path, 1u, 2u, 3u,
                                 TEST_EVENT_MASK) == WZ_RESULT_OK);
    REQUIRE(wz_trace_file_create(&second, second_path, 2u, 2u, 3u,
                                 TEST_EVENT_MASK) == WZ_RESULT_OK);
    REQUIRE(wz_trace_file_create(&duplicate, first_path, 3u, 2u, 3u,
                                 TEST_EVENT_MASK) == WZ_RESULT_TRACE_FAILURE);
    REQUIRE(first.file != NULL && second.file != NULL);
    REQUIRE(fseek(first.file, 0L, SEEK_END) == 0);
    REQUIRE((file_size = ftell(first.file)) == (long)WZ_TRACE_FILE_SIZE);

    emit_event(&second, 0u, 1u, WZ_TRACE_DEVELOPER_MARKER, 90u);
    for (wz_qword_t sequence = 0u; sequence < 9u; ++sequence) {
        emit_event(&first, sequence, sequence + 1u,
                   (sequence & 1u) == 0u ? WZ_TRACE_DEVELOPER_MARKER :
                                          WZ_TRACE_MASTER_TICK_ADVANCED,
                   (wz_byte_t)sequence);
    }
    REQUIRE(!first.failed && !second.failed);
    REQUIRE(first.record_count == 3u && first.generation == 1u);
    REQUIRE(wz_trace_file_freeze(&first) == WZ_RESULT_OK);
    emit_event(&first, 9u, 10u, WZ_TRACE_DEVELOPER_MARKER, 9u);
    REQUIRE(wz_trace_file_recover(first_path, collect_event, &recovered,
                                  &recovered_count) == WZ_RESULT_OK);
    REQUIRE(recovered_count == 3u && recovered.count == 3u);
    REQUIRE(recovered.events[0].sequence == 4u &&
            recovered.events[0].master_tick == 5u &&
            recovered.events[0].value == 4u);
    REQUIRE(recovered.events[1].sequence == 6u &&
            recovered.events[1].master_tick == 7u &&
            recovered.events[1].value == 6u);
    REQUIRE(recovered.events[2].sequence == 8u &&
            recovered.events[2].master_tick == 9u &&
            recovered.events[2].value == 8u);
    wz_trace_file_close(&first);
    wz_trace_file_close(&second);

    REQUIRE(corrupt_commit(first_path,
        (long)(WZ_TRACE_HEADER_SIZE + 2u * WZ_TRACE_RECORD_SIZE +
               WZ_TRACE_COMMIT_OFFSET)));
    memset(&recovered, 0, sizeof(recovered));
    REQUIRE(wz_trace_file_recover(first_path, collect_event, &recovered,
                                  &recovered_count) == WZ_RESULT_OK);
    REQUIRE(recovered_count == 2u && recovered.count == 2u);
    REQUIRE(recovered.events[0].sequence == 6u &&
            recovered.events[0].master_tick == 7u);
    REQUIRE(recovered.events[1].sequence == 8u &&
            recovered.events[1].master_tick == 9u);

    memset(&recovered, 0, sizeof(recovered));
    REQUIRE(wz_trace_file_create(&duplicate, partial_path, 4u, 2u, 3u,
                                 TEST_EVENT_MASK) == WZ_RESULT_OK);
    emit_event(&duplicate, 10u, 11u, WZ_TRACE_DEVELOPER_MARKER, 10u);
    emit_event(&duplicate, 12u, 13u, WZ_TRACE_DEVELOPER_MARKER, 12u);
    wz_trace_file_close(&duplicate);
    file = fopen(partial_path, "r+b");
    REQUIRE(file != NULL);
    REQUIRE(fseek(file, (long)(WZ_TRACE_HEADER_SIZE + 2u * WZ_TRACE_RECORD_SIZE),
                  SEEK_SET) == 0);
    REQUIRE(fwrite("partial", 1u, 7u, file) == 7u);
    {
        int close_result = fclose(file);
        file = NULL;
        REQUIRE(close_result == 0);
    }
    REQUIRE(wz_trace_file_recover(partial_path, collect_event, &recovered,
                                  &recovered_count) == WZ_RESULT_OK);
    REQUIRE(recovered_count == 2u && recovered.count == 2u);
    REQUIRE(recovered.events[0].sequence == 10u &&
            recovered.events[0].master_tick == 11u);
    REQUIRE(recovered.events[1].sequence == 12u &&
            recovered.events[1].master_tick == 13u);

    REQUIRE(remove(first_path) == 0);
    first_path[0] = '\0';
    REQUIRE(remove(second_path) == 0);
    second_path[0] = '\0';
    REQUIRE(remove(partial_path) == 0);
    partial_path[0] = '\0';
    (void)puts("PASS timing trace isolation, fixed size, sparse wrap, freeze, and crash-tail recovery");
    result = 0;

cleanup:
    if (file != NULL) (void)fclose(file);
    wz_trace_file_close(&first);
    wz_trace_file_close(&second);
    wz_trace_file_close(&duplicate);
    if (first_path[0] != '\0') (void)remove(first_path);
    if (second_path[0] != '\0') (void)remove(second_path);
    if (partial_path[0] != '\0') (void)remove(partial_path);
    return result;
}
