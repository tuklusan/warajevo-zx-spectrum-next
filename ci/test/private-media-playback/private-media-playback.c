/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#define _DEFAULT_SOURCE
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "core/wz_machine.h"

typedef struct {
    size_t files;
    size_t tap_files;
    size_t tzx_files;
    size_t segments;
    size_t signal_edges;
    size_t motor_stops;
    uint64_t ticks;
} playback_totals_t;

static int has_extension(const char* name, const char* extension)
{
    size_t n = strlen(name);
    size_t e = strlen(extension);
    return n > e && name[n - e - 1u] == '.' &&
        strcasecmp(name + n - e, extension) == 0;
}

static unsigned char* read_file(const char* path, size_t* length)
{
    FILE* file = fopen(path, "rb");
    long size;
    unsigned char* bytes;
    if (file == NULL || fseek(file, 0, SEEK_END) != 0 ||
        (size = ftell(file)) <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        if (file != NULL) fclose(file);
        return NULL;
    }
    bytes = (unsigned char*)malloc((size_t)size);
    if (bytes == NULL || fread(bytes, 1u, (size_t)size, file) != (size_t)size) {
        free(bytes);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *length = (size_t)size;
    return bytes;
}

static int parse_media(const unsigned char* data, size_t length, int is_tap,
                       uint32_t ticks_per_tstate,
                       wz_tape_segment_t** segments, size_t* count)
{
    wz_result_t result;
    if (is_tap) {
        result = wz_tape_parse_standard_tap(data, length, ticks_per_tstate,
                                             NULL, 0u, count);
        if (result != WZ_RESULT_BUFFER_TOO_SMALL || *count == 0u ||
            *count > SIZE_MAX / sizeof(**segments)) return 0;
        *segments = (wz_tape_segment_t*)malloc(*count * sizeof(**segments));
        return *segments != NULL &&
            wz_tape_parse_standard_tap(data, length, ticks_per_tstate,
                *segments, *count, count) == WZ_RESULT_OK;
    }
    {
        size_t block_count = 0u;
        wz_tzx_block_t* blocks = NULL;
        result = wz_tape_parse_tzx(data, length, NULL, 0u, &block_count);
        if (result != WZ_RESULT_BUFFER_TOO_SMALL || block_count == 0u ||
            block_count > SIZE_MAX / sizeof(*blocks)) return 0;
        blocks = (wz_tzx_block_t*)malloc(block_count * sizeof(*blocks));
        if (blocks == NULL || wz_tape_parse_tzx(data, length, blocks,
                block_count, &block_count) != WZ_RESULT_OK) {
            free(blocks);
            return 0;
        }
        result = wz_tape_expand_tzx_timing(blocks, block_count,
            ticks_per_tstate, NULL, 0u, count);
        if (result != WZ_RESULT_BUFFER_TOO_SMALL || *count == 0u ||
            *count > SIZE_MAX / sizeof(**segments)) {
            free(blocks);
            return 0;
        }
        *segments = (wz_tape_segment_t*)malloc(*count * sizeof(**segments));
        if (*segments == NULL || wz_tape_expand_tzx_timing(blocks, block_count,
                ticks_per_tstate, *segments, *count, count) != WZ_RESULT_OK) {
            free(blocks);
            free(*segments);
            *segments = NULL;
            return 0;
        }
        free(blocks);
    }
    return 1;
}

static int exercise(wz_machine_t* machine, const wz_tape_segment_t* segments,
                    size_t count, playback_totals_t* totals)
{
    unsigned char previous_level;
    if (wz_machine_mount_tape(machine, segments, count) != WZ_RESULT_OK ||
        wz_machine_set_tape_motor(machine, true) != WZ_RESULT_OK) return 0;
    previous_level = wz_machine_tape_ear_level(machine);
    for (size_t i = 0u; i < count; ++i) {
        if (UINT64_MAX - totals->ticks < segments[i].duration ||
            wz_machine_advance_tape(machine, segments[i].duration) !=
                WZ_RESULT_OK) return 0;
        totals->ticks += segments[i].duration;
        if (i != 0u && previous_level != segments[i].ear_level) {
            ++totals->signal_edges;
        }
        previous_level = segments[i].ear_level;
        ++totals->segments;
        if (segments[i].motor_stop_after != 0u) {
            ++totals->motor_stops;
            if (machine->tape_state.segment_index != i ||
                machine->tape_state.segment_elapsed != segments[i].duration ||
                machine->tape_state.motor_on) return 0;
            if (i + 1u < count && wz_machine_set_tape_motor(machine, true) !=
                    WZ_RESULT_OK) return 0;
        } else if (i + 1u < count &&
                   (machine->tape_state.segment_index != i + 1u ||
                    machine->tape_state.ear_level != segments[i + 1u].ear_level)) {
            return 0;
        } else if (i + 1u == count &&
                   !wz_tape_state_at_end(&machine->tape_state)) {
            return 0;
        }
    }
    if (!wz_tape_state_at_end(&machine->tape_state)) return 0;
    return wz_machine_unmount_tape(machine) == WZ_RESULT_OK;
}

int main(int argc, char** argv)
{
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    struct dirent** entries = NULL;
    playback_totals_t totals = {0};
    wz_machine_t machine;
    int initialized = 0;
    int entries_count;
    size_t failures = 0u;
    if (argc != 2 || profile == NULL ||
        wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("private media playback: initialization failed\n", stderr);
        return 2;
    }
    initialized = 1;
    entries_count = scandir(argv[1], &entries, NULL, alphasort);
    if (entries_count < 0) {
        fputs("private media playback: corpus unavailable\n", stderr);
        failures = 1u;
        goto done;
    }
    for (int entry = 0; entry < entries_count; ++entry) {
        int tap = has_extension(entries[entry]->d_name, "tap");
        int tzx = has_extension(entries[entry]->d_name, "tzx");
        char path[4096];
        size_t path_size = strlen(argv[1]) + strlen(entries[entry]->d_name) + 2u;
        size_t length = 0u;
        size_t count = 0u;
        unsigned char* data = NULL;
        wz_tape_segment_t* segments = NULL;
        if (!tap && !tzx) {
            free(entries[entry]);
            continue;
        }
        ++totals.files;
        if (tap) ++totals.tap_files;
        else ++totals.tzx_files;
        if (path_size > sizeof(path)) goto case_failed;
        (void)snprintf(path, sizeof(path), "%s/%s", argv[1],
                       entries[entry]->d_name);
        data = read_file(path, &length);
        if (data == NULL || !parse_media(data, length, tap,
                profile->master_ticks_per_cpu_tstate, &segments, &count) ||
            !exercise(&machine, segments, count, &totals)) goto case_failed;
        free(data);
        free(segments);
        free(entries[entry]);
        continue;
case_failed:
        free(data);
        free(segments);
        free(entries[entry]);
        ++failures;
    }
    free(entries);
    entries = NULL;
done:
    if (entries != NULL) {
        for (int i = 0; i < entries_count; ++i) free(entries[i]);
        free(entries);
    }
    if (initialized) wz_machine_destroy(&machine);
    printf("{\"status\":\"%s\",\"failures\":%zu,\"files\":%zu,"
           "\"tapFiles\":%zu,\"tzxFiles\":%zu,\"segments\":%zu,"
           "\"signalEdges\":%zu,\"motorStops\":%zu,\"ticks\":%llu}\n",
           failures == 0u && totals.files != 0u ? "pass" : "fail",
           failures, totals.files, totals.tap_files, totals.tzx_files,
           totals.segments, totals.signal_edges, totals.motor_stops,
           (unsigned long long)totals.ticks);
    return failures == 0u && totals.files != 0u ? 0 : 1;
}
