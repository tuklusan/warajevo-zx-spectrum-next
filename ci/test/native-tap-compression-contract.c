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

static bool round_trip(const wz_byte_t* input, size_t input_length,
                       bool require_gain)
{
    wz_byte_t* encoded = NULL;
    wz_byte_t* decoded = NULL;
    wz_native_tap_record_t record;
    size_t encoded_length = 0u;
    size_t decoded_length = 0u;
    wz_word_t signed_length = 0u;
    bool ok = false;

    CHECK(wz_tape_native_compress_payload(input, input_length, NULL, 0u,
                                          &encoded_length, &signed_length) ==
          WZ_RESULT_BUFFER_TOO_SMALL && encoded_length != 0u);
    encoded = (wz_byte_t*)malloc(encoded_length);
    decoded = (wz_byte_t*)malloc(input_length);
    CHECK(encoded != NULL && decoded != NULL);
    CHECK(wz_tape_native_compress_payload(input, input_length, encoded,
                                          encoded_length, &encoded_length,
                                          &signed_length) == WZ_RESULT_OK);
    CHECK(signed_length < encoded_length);
    if (require_gain) CHECK(encoded_length < input_length);
    memset(&record, 0, sizeof(record));
    record.record_type = 5u;
    record.stored_length = 65535u;
    record.decompressed_length = (wz_word_t)input_length;
    record.compressed_length = (wz_word_t)encoded_length;
    record.signed_length = signed_length;
    record.payload = encoded;
    record.payload_length = encoded_length;
    CHECK(wz_tape_native_decompress_record(&record, decoded, input_length,
                                           &decoded_length) == WZ_RESULT_OK);
    CHECK(decoded_length == input_length &&
          memcmp(decoded, input, input_length) == 0);
    ok = true;
cleanup:
    free(encoded);
    free(decoded);
    return ok;
}

int main(void)
{
    wz_byte_t literals[19];
    wz_byte_t repeated[256];
    wz_byte_t long_pattern[1024];

    for (size_t index = 0u; index < sizeof(literals); ++index)
        literals[index] = (wz_byte_t)(index * 37u + 11u);
    for (size_t index = 0u; index < sizeof(repeated); ++index)
        repeated[index] = (wz_byte_t)(index & 3u);
    for (size_t index = 0u; index < sizeof(long_pattern); ++index)
        long_pattern[index] = (wz_byte_t)((index % 47u) * 9u +
                                          (index % 5u));
    if (!round_trip(literals, sizeof(literals), false) ||
        !round_trip(repeated, sizeof(repeated), true) ||
        !round_trip(long_pattern, sizeof(long_pattern), true)) {
        fputs("FAIL native TAP compression contract\n", stderr);
        return EXIT_FAILURE;
    }
    puts("native-tap-compression-contract cases=3 status=pass");
    return EXIT_SUCCESS;
}
