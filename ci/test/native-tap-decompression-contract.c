/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is licensed under the GNU General Public License, version 2 or later.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/wz_tape.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(value) do { if (!(value)) goto cleanup; } while (0)

static bool convert(const wz_native_tap_record_t* record,
                    wz_byte_t* standard, size_t capacity,
                    size_t expected_length, const wz_byte_t* expected_payload)
{
    wz_byte_t native[64];
    size_t native_length = 0u;
    size_t standard_length = 0u;
    CHECK(wz_tape_write_native_tap(record, 1u, NULL, 0u, &native_length) ==
          WZ_RESULT_BUFFER_TOO_SMALL);
    CHECK(native_length <= sizeof(native));
    CHECK(wz_tape_write_native_tap(record, 1u, native, sizeof(native),
                                   &native_length) == WZ_RESULT_OK);
    CHECK(wz_tape_native_decompress_record(record, standard + 3u,
                                           capacity - 3u, &standard_length) ==
          WZ_RESULT_OK && standard_length == expected_length - 3u &&
          memcmp(standard + 3u, expected_payload, standard_length) == 0);
    CHECK(wz_tape_native_to_standard_tap(native, native_length, NULL, 0u,
                                         &standard_length) ==
          WZ_RESULT_BUFFER_TOO_SMALL && standard_length == expected_length);
    CHECK(wz_tape_native_to_standard_tap(native, native_length, standard,
                                         capacity, &standard_length) ==
          WZ_RESULT_OK && standard_length == expected_length);
    CHECK(standard[0] == (wz_byte_t)(expected_length - 2u) &&
          standard[1] == 0u && standard[2] == record->flag &&
          memcmp(standard + 3u, expected_payload, expected_length - 3u) == 0);
    return true;
cleanup:
    return false;
}

static bool literal_case(void)
{
    static const wz_byte_t encoded[] = {0x00u, 0xabu, 0xcdu};
    static const wz_byte_t expected[] = {0xabu, 0xcdu};
    const wz_native_tap_record_t record = {
        .stored_length = 65535u, .flag = 0x00u, .record_type = 5u,
        .decompressed_length = 2u, .compressed_length = sizeof(encoded),
        .signed_length = 0u, .payload = encoded, .payload_length = sizeof(encoded)};
    wz_byte_t standard[8];
    return convert(&record, standard, sizeof(standard), 5u, expected);
}

static bool match_case(void)
{
    /* 0 literal, 100 run of three, distance high bit zero, low byte two. */
    static const wz_byte_t encoded[] = {0x48u, 0x5au, 0x02u};
    static const wz_byte_t expected[] = {0x5au, 0x5au, 0x5au, 0x5au};
    const wz_native_tap_record_t record = {
        .stored_length = 65535u, .flag = 0xffu, .record_type = 5u,
        .decompressed_length = 4u, .compressed_length = sizeof(encoded),
        .signed_length = 0u, .payload = encoded, .payload_length = sizeof(encoded)};
    wz_byte_t standard[8];
    return convert(&record, standard, sizeof(standard), 7u, expected);
}

static bool short_distance_case(void)
{
    /* 0 literal, 010 run of two, short encoded distance two. */
    static const wz_byte_t encoded[] = {0x50u, 0x33u, 0x02u};
    static const wz_byte_t expected[] = {0x33u, 0x33u, 0x33u};
    const wz_native_tap_record_t record = {
        .stored_length = 65535u, .flag = 0x80u, .record_type = 5u,
        .decompressed_length = 3u, .compressed_length = sizeof(encoded),
        .signed_length = 0u, .payload = encoded, .payload_length = sizeof(encoded)};
    wz_byte_t standard[8];
    return convert(&record, standard, sizeof(standard), 6u, expected);
}

static bool malformed_case(void)
{
    static const wz_byte_t encoded[] = {0x00u, 0xabu, 0xcdu};
    const wz_native_tap_record_t record = {
        .stored_length = 65535u, .record_type = 5u,
        .decompressed_length = 2u, .compressed_length = sizeof(encoded),
        .signed_length = 0u, .payload = encoded, .payload_length = sizeof(encoded)};
    wz_byte_t native[64];
    wz_byte_t standard[8];
    size_t native_length = 0u;
    size_t standard_length = 0u;
    CHECK(wz_tape_native_decompress_record(&record, NULL, 0u,
                                           &standard_length) ==
          WZ_RESULT_PARSE_ERROR && standard_length == 0u);
    CHECK(wz_tape_write_native_tap(&record, 1u, native, sizeof(native),
                                   &native_length) == WZ_RESULT_OK);
    native[27u] = 3u;
    native[28u] = 0u;
    CHECK(wz_tape_native_to_standard_tap(native, native_length, standard,
                                         sizeof(standard), &standard_length) ==
          WZ_RESULT_PARSE_ERROR);
    return true;
cleanup:
    return false;
}

int main(void)
{
    if (!literal_case()) {
        fputs("FAIL native TAP decompression literal case\n", stderr);
        return EXIT_FAILURE;
    }
    if (!match_case()) {
        fputs("FAIL native TAP decompression match case\n", stderr);
        return EXIT_FAILURE;
    }
    if (!short_distance_case()) {
        fputs("FAIL native TAP decompression short-distance case\n", stderr);
        return EXIT_FAILURE;
    }
    if (!malformed_case()) {
        fputs("FAIL native TAP decompression malformed case\n", stderr);
        return EXIT_FAILURE;
    }
    puts("native-tap-decompression-contract cases=4 status=pass");
    return EXIT_SUCCESS;
}
