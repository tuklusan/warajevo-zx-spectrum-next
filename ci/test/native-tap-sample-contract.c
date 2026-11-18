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

static bool sample_case(const wz_byte_t* payload, size_t payload_length,
                        wz_word_t logical_length, wz_word_t signed_length,
                        wz_byte_t status, size_t expected_bits)
{
    wz_native_tap_record_t record;
    wz_byte_t native[4096];
    wz_tape_segment_t* segments = NULL;
    size_t native_length = 0u;
    size_t count = 0u;
    bool ok = false;

    memset(&record, 0, sizeof(record));
    record.stored_length = 65534u;
    record.flag = status;
    record.record_type = 4u;
    record.decompressed_length = logical_length;
    record.compressed_length = (wz_word_t)payload_length;
    record.signed_length = signed_length;
    record.payload = payload;
    record.payload_length = payload_length;
    CHECK(wz_tape_write_native_tap(&record, 1u, native, sizeof(native),
                                   &native_length) == WZ_RESULT_OK);
    CHECK(wz_tape_native_to_segments(native, native_length, 1u, 3500000u,
                                      NULL, 0u, &count) ==
          WZ_RESULT_BUFFER_TOO_SMALL && count == expected_bits);
    segments = (wz_tape_segment_t*)malloc(count * sizeof(*segments));
    CHECK(segments != NULL);
    CHECK(wz_tape_native_to_segments(native, native_length, 1u, 3500000u,
                                     segments, count, &count) == WZ_RESULT_OK);
    CHECK(count == expected_bits && segments[0].ear_level ==
          ((payload[0] >> 7u) & 1u));
    for (size_t index = 0u; index < count; ++index) {
        CHECK(segments[index].duration != 0u &&
              segments[index].motor_stop_after == 0u);
    }
    ok = true;
cleanup:
    free(segments);
    return ok;
}

int main(void)
{
    static const wz_byte_t unpacked[] = {0x80u, 0x01u};
    wz_byte_t source[128];
    wz_byte_t* encoded = NULL;
    size_t encoded_length = 0u;
    wz_word_t signed_length = 0u;

    for (size_t index = 0u; index < sizeof(source); ++index) {
        source[index] = (wz_byte_t)((index & 1u) != 0u ? 0xa5u : 0x5au);
    }
    if (!sample_case(unpacked, sizeof(unpacked), 2u, 0u, 7u, 16u)) {
        fputs("FAIL native TAP sample literal case\n", stderr);
        return EXIT_FAILURE;
    }
    if (wz_tape_native_compress_payload(source, sizeof(source), NULL, 0u,
            &encoded_length, &signed_length) != WZ_RESULT_BUFFER_TOO_SMALL ||
        encoded_length >= sizeof(source)) {
        fputs("FAIL native TAP sample compression setup\n", stderr);
        return EXIT_FAILURE;
    }
    encoded = (wz_byte_t*)malloc(encoded_length);
    if (encoded == NULL ||
        wz_tape_native_compress_payload(source, sizeof(source), encoded,
            encoded_length, &encoded_length, &signed_length) != WZ_RESULT_OK ||
        !sample_case(encoded, encoded_length, (wz_word_t)sizeof(source),
                     signed_length, 7u, sizeof(source) * 8u)) {
        free(encoded);
        fputs("FAIL native TAP sample compressed case\n", stderr);
        return EXIT_FAILURE;
    }
    free(encoded);
    puts("native-tap-sample-contract cases=2 status=pass");
    return EXIT_SUCCESS;
}
