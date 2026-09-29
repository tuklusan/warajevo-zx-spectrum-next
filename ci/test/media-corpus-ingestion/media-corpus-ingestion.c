/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#define _DEFAULT_SOURCE

#include "core/wz_machine.h"
#include "core/wz_tape.h"

#include <ctype.h>
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t files;
    size_t supported;
    size_t unsupported;
    size_t malformed;
    size_t tap_files;
    size_t tzx_files;
    size_t unsupported_tzx_blocks;
    size_t tap_failures;
    size_t tzx_failures;
    size_t read_failures;
    size_t failure_case;
} corpus_counts_t;

static wz_byte_t* read_file(const char* path, size_t* length)
{
    FILE* file;
    long file_size;
    wz_byte_t* data;
    if (path == NULL || length == NULL ||
        (file = fopen(path, "rb")) == NULL) return NULL;
    if (fseek(file, 0L, SEEK_END) != 0 ||
        (file_size = ftell(file)) <= 0L ||
        fseek(file, 0L, SEEK_SET) != 0 ||
        (unsigned long)file_size > (unsigned long)SIZE_MAX) {
        fclose(file);
        return NULL;
    }
    data = (wz_byte_t*)malloc((size_t)file_size);
    if (data == NULL || fread(data, 1u, (size_t)file_size, file) !=
            (size_t)file_size) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *length = (size_t)file_size;
    return data;
}

static int has_extension(const char* name, const char* extension)
{
    size_t name_length;
    size_t extension_length;
    if (name == NULL || extension == NULL) return 0;
    name_length = strlen(name);
    extension_length = strlen(extension);
    if (name_length <= extension_length + 1u ||
        name[name_length - extension_length - 1u] != '.') return 0;
    for (size_t index = 0u; index < extension_length; ++index) {
        if (tolower((unsigned char)name[name_length - extension_length + index]) !=
            tolower((unsigned char)extension[index])) return 0;
    }
    return 1;
}

static int parse_tap(const wz_byte_t* data, size_t length,
                     wz_dword_t ticks_per_tstate)
{
    if (wz_tape_is_native_tap(data, length)) {
        size_t record_count = 0u;
        wz_result_t result = wz_tape_parse_native_tap(data, length, NULL, 0u,
                                                       &record_count);
        wz_native_tap_record_t* records;
        if (result != WZ_RESULT_BUFFER_TOO_SMALL || record_count == 0u ||
            record_count > SIZE_MAX / sizeof(*records)) return 0;
        records = (wz_native_tap_record_t*)malloc(record_count * sizeof(*records));
        if (records == NULL) return 0;
        result = wz_tape_parse_native_tap(data, length, records, record_count,
                                          &record_count);
        free(records);
        return result == WZ_RESULT_OK;
    }

    size_t segment_count = 0u;
    wz_result_t result = wz_tape_parse_standard_tap(data, length,
        ticks_per_tstate, NULL, 0u, &segment_count);
    wz_tape_segment_t* segments;
    if (result != WZ_RESULT_BUFFER_TOO_SMALL || segment_count == 0u ||
        segment_count > SIZE_MAX / sizeof(*segments)) return 0;
    segments = (wz_tape_segment_t*)malloc(segment_count * sizeof(*segments));
    if (segments == NULL) return 0;
    result = wz_tape_parse_standard_tap(data, length, ticks_per_tstate,
                                        segments, segment_count,
                                        &segment_count);
    free(segments);
    return result == WZ_RESULT_OK;
}

static int parse_tzx(const wz_byte_t* data, size_t length,
                     wz_dword_t ticks_per_tstate,
                     size_t* unsupported_blocks, int* unsupported_media)
{
    size_t block_count = 0u;
    wz_result_t result = wz_tape_parse_tzx(data, length, NULL, 0u, &block_count);
    wz_tzx_block_t* blocks;
    size_t segment_count = 0u;
    wz_tape_segment_t* segments = NULL;
    if (result != WZ_RESULT_BUFFER_TOO_SMALL || block_count == 0u ||
        block_count > SIZE_MAX / sizeof(*blocks)) return 0;
    blocks = (wz_tzx_block_t*)malloc(block_count * sizeof(*blocks));
    if (blocks == NULL) return 0;
    result = wz_tape_parse_tzx(data, length, blocks, block_count, &block_count);
    if (result != WZ_RESULT_OK) {
        free(blocks);
        return 0;
    }
    for (size_t index = 0u; index < block_count; ++index) {
        if (blocks[index].disposition == WZ_TZX_UNSUPPORTED)
            ++*unsupported_blocks;
    }
    result = wz_tape_expand_tzx_timing(blocks, block_count, ticks_per_tstate,
                                       NULL, 0u, &segment_count);
    if (result == WZ_RESULT_UNSUPPORTED_OPERATION) {
        *unsupported_media = 1;
        free(blocks);
        return 1;
    }
    if (result != WZ_RESULT_BUFFER_TOO_SMALL || segment_count == 0u ||
        segment_count > SIZE_MAX / sizeof(*segments)) {
        free(blocks);
        return 0;
    }
    segments = (wz_tape_segment_t*)malloc(segment_count * sizeof(*segments));
    if (segments == NULL) {
        free(blocks);
        return 0;
    }
    result = wz_tape_expand_tzx_timing(blocks, block_count, ticks_per_tstate,
                                       segments, segment_count, &segment_count);
    free(segments);
    free(blocks);
    return result == WZ_RESULT_OK;
}

int main(int argc, char** argv)
{
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    corpus_counts_t counts = {0};
    struct dirent** entries = NULL;
    int entry_count;
    if (argc != 2 || profile == NULL ||
        (entry_count = scandir(argv[1], &entries, NULL, alphasort)) < 0) {
        fputs("media corpus contract: invalid input directory\n", stderr);
        return 2;
    }
    size_t case_index = 0u;
    for (int entry_index = 0; entry_index < entry_count; ++entry_index) {
        struct dirent* entry = entries[entry_index];
        char path[4096];
        size_t name_length = strlen(entry->d_name);
        int is_tap = has_extension(entry->d_name, "tap");
        int is_tzx = has_extension(entry->d_name, "tzx");
        size_t length = 0u;
        wz_byte_t* data;
        int supported;
        int unsupported_media = 0;
        if (!is_tap && !is_tzx) {
            free(entry);
            continue;
        }
        ++case_index;
        ++counts.files;
        if (is_tap) ++counts.tap_files;
        else ++counts.tzx_files;
        if (name_length + strlen(argv[1]) + 2u > sizeof(path)) {
            ++counts.malformed;
            ++counts.read_failures;
            counts.failure_case = case_index;
            free(entry);
            continue;
        }
        (void)snprintf(path, sizeof(path), "%s/%s", argv[1], entry->d_name);
        data = read_file(path, &length);
        if (data == NULL) {
            ++counts.malformed;
            ++counts.read_failures;
            counts.failure_case = case_index;
            free(entry);
            continue;
        }
        supported = is_tap ? parse_tap(data, length,
            profile->master_ticks_per_cpu_tstate) : parse_tzx(data, length,
            profile->master_ticks_per_cpu_tstate,
            &counts.unsupported_tzx_blocks, &unsupported_media);
        free(data);
        if (!supported) {
            ++counts.malformed;
            if (is_tap) ++counts.tap_failures;
            else ++counts.tzx_failures;
            counts.failure_case = case_index;
        }
        else if (unsupported_media) ++counts.unsupported;
        else ++counts.supported;
        free(entry);
    }
    free(entries);
    printf("{\"status\":\"%s\",\"files\":%zu,\"tapFiles\":%zu,"
           "\"tzxFiles\":%zu,\"supported\":%zu,\"unsupported\":%zu,"
           "\"unsupportedTzxBlocks\":%zu,\"malformed\":%zu,"
           "\"tapFailures\":%zu,\"tzxFailures\":%zu,\"readFailures\":%zu,"
           "\"failureCase\":%zu}\n",
           counts.files != 0u && counts.malformed == 0u ? "pass" : "fail",
           counts.files, counts.tap_files, counts.tzx_files, counts.supported,
           counts.unsupported, counts.unsupported_tzx_blocks, counts.malformed,
           counts.tap_failures, counts.tzx_failures, counts.read_failures,
           counts.failure_case);
    if (counts.files == 0u || counts.malformed != 0u) {
        fputs("media corpus contract: at least one tape was malformed or unreadable\n",
              stderr);
        return 1;
    }
    return 0;
}
