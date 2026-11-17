/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/wz_tape.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RECORDS 4u
#define CHECK(value) do { if (!(value)) goto cleanup; } while (0)

static void le32(wz_byte_t* p, wz_dword_t value)
{
    p[0] = (wz_byte_t)value; p[1] = (wz_byte_t)(value >> 8u);
    p[2] = (wz_byte_t)(value >> 16u); p[3] = (wz_byte_t)(value >> 24u);
}

static void records_init(wz_native_tap_record_t records[RECORDS])
{
    static const wz_byte_t payload0[] = {0x00u, 0x03u, 0xaau, 0x55u};
    static const wz_byte_t payload1[] = {0x7fu, 0x01u, 0x80u};
    static const wz_byte_t payload4[] = {0xedu, 0xedu};
    static const wz_byte_t payload5[] = {0x00u};
    memset(records, 0, sizeof(*records) * RECORDS);
    records[0] = (wz_native_tap_record_t){.record_type = 0u,
        .stored_length = 6u, .payload = payload0, .payload_length = 4u};
    records[1] = (wz_native_tap_record_t){.record_type = 3u,
        .stored_length = 5u, .flag = 0x80u, .payload = payload1,
        .payload_length = 3u};
    records[2] = (wz_native_tap_record_t){.record_type = 4u,
        .stored_length = 65534u, .decompressed_length = 8u,
        .compressed_length = 2u, .signed_length = 0x1234u,
        .payload = payload4, .payload_length = 2u};
    records[3] = (wz_native_tap_record_t){.record_type = 5u,
        .stored_length = 65535u, .decompressed_length = 1u,
        .compressed_length = 1u, .signed_length = 0x4321u,
        .payload = payload5, .payload_length = 1u};
}

static bool round_trip(void)
{
    wz_native_tap_record_t source[RECORDS], parsed[RECORDS];
    size_t length = 0u, count = 0u;
    wz_byte_t* image = NULL;
    bool ok = false;
    records_init(source);
    CHECK(wz_tape_write_native_tap(source, RECORDS, NULL, 0u, &length) ==
          WZ_RESULT_BUFFER_TOO_SMALL && length != 0u);
    image = (wz_byte_t*)malloc(length);
    CHECK(image != NULL && wz_tape_write_native_tap(source, RECORDS, image,
          length, &length) == WZ_RESULT_OK);
    CHECK(wz_tape_is_native_tap(image, length));
    CHECK(wz_tape_parse_native_tap(image, length, NULL, 0u, &count) ==
          WZ_RESULT_BUFFER_TOO_SMALL && count == RECORDS);
    CHECK(wz_tape_parse_native_tap(image, length, parsed, RECORDS, &count) ==
          WZ_RESULT_OK && count == RECORDS);
    for (size_t i = 0u; i < RECORDS; ++i) {
        CHECK(parsed[i].record_type == source[i].record_type &&
              parsed[i].stored_length == source[i].stored_length &&
              parsed[i].flag == source[i].flag &&
              parsed[i].payload_length == source[i].payload_length &&
              memcmp(parsed[i].payload, source[i].payload,
                     source[i].payload_length) == 0);
    }
    CHECK(parsed[0].previous_offset == 0u &&
          parsed[1].previous_offset == parsed[0].offset &&
          parsed[2].previous_offset == parsed[1].offset &&
          parsed[3].previous_offset == parsed[2].offset &&
          parsed[2].decompressed_length == 8u &&
          parsed[2].compressed_length == 2u && parsed[2].signed_length == 0x1234u);
    ok = true;
cleanup:
    free(image);
    return ok;
}

static bool rejection(void)
{
    wz_native_tap_record_t records[RECORDS];
    size_t length = 0u, count = 0u;
    wz_byte_t* image = NULL;
    bool ok = false;
    records_init(records);
    CHECK(wz_tape_write_native_tap(records, RECORDS, NULL, 0u, &length) ==
          WZ_RESULT_BUFFER_TOO_SMALL);
    image = (wz_byte_t*)malloc(length);
    CHECK(image != NULL && wz_tape_write_native_tap(records, RECORDS, image,
          length, &length) == WZ_RESULT_OK);
    image[23u] = 6u;
    CHECK(wz_tape_parse_native_tap(image, length, NULL, 0u, &count) ==
          WZ_RESULT_PARSE_ERROR);
    image[23u] = 0u;
    le32(image + 28u, 0u);
    CHECK(wz_tape_parse_native_tap(image, length, NULL, 0u, &count) ==
          WZ_RESULT_PARSE_ERROR);
    le32(image + 28u, 12u);
    le32(image + 16u, (wz_dword_t)(length + 1u));
    CHECK(wz_tape_parse_native_tap(image, length, NULL, 0u, &count) ==
          WZ_RESULT_PARSE_ERROR);
    records[0].stored_length = 5u;
    CHECK(wz_tape_write_native_tap(records, RECORDS, image, length, &length) ==
          WZ_RESULT_PARSE_ERROR);
    ok = true;
cleanup:
    free(image);
    return ok;
}

static bool conversion_case(void)
{
    static const wz_byte_t payload_a[] = {0x00u, 0x11u, 0x22u, 0x33u};
    static const wz_byte_t payload_b[] = {0xffu, 0xaau, 0x55u, 0xffu};
    wz_native_tap_record_t records[2] = {
        {.record_type = 0u, .stored_length = 6u, .flag = 0x00u,
         .payload = payload_a, .payload_length = sizeof(payload_a)},
        {.record_type = 3u, .stored_length = 6u, .flag = 0xffu,
         .payload = payload_b, .payload_length = sizeof(payload_b)}};
    wz_native_tap_record_t extended[RECORDS];
    wz_byte_t native[128];
    wz_byte_t standard[32];
    size_t native_length = 0u;
    size_t standard_length = 0u;
    size_t block_count = 0u;
    wz_tap_block_t blocks[2];
    bool ok = false;

    CHECK(wz_tape_write_native_tap(records, 2u, native, sizeof(native),
                                   &native_length) == WZ_RESULT_OK);
    CHECK(wz_tape_native_to_standard_tap(native, native_length, NULL, 0u,
                                         &standard_length) ==
          WZ_RESULT_BUFFER_TOO_SMALL && standard_length == 16u);
    CHECK(wz_tape_native_to_standard_tap(native, native_length, standard,
                                         sizeof(standard), &standard_length) ==
          WZ_RESULT_OK);
    CHECK(wz_tape_parse_standard_tap(standard, standard_length, 1u, NULL, 0u,
                                     &block_count) == WZ_RESULT_BUFFER_TOO_SMALL &&
          block_count == 2u);
    CHECK(wz_tape_parse_standard_tap(standard, standard_length, 1u, blocks, 2u,
                                     &block_count) == WZ_RESULT_OK &&
          blocks[0].length == 4u && blocks[0].data[0] == 0x00u &&
          blocks[1].length == 4u && blocks[1].data[0] == 0xffu);
    records_init(extended);
    CHECK(wz_tape_write_native_tap(extended, RECORDS, native, sizeof(native),
                                   &native_length) == WZ_RESULT_OK);
    CHECK(wz_tape_native_to_standard_tap(native, native_length, standard,
                                         sizeof(standard), &standard_length) ==
          WZ_RESULT_UNSUPPORTED_OPERATION);
    ok = true;
cleanup:
    return ok;
}

int main(void)
{
    if (!round_trip() || !rejection() || !conversion_case()) {
        fputs("FAIL native TAP compatibility contract\n", stderr);
        return EXIT_FAILURE;
    }
    puts("native-tap-contract cases=3 status=pass");
    return EXIT_SUCCESS;
}
