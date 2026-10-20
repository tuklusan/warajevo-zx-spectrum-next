/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "app/wz_host_output.h"

#define PAYLOAD_SIZE (128u * 1024u)

static void make_payload(unsigned writer, unsigned iteration,
                         unsigned char payload[PAYLOAD_SIZE])
{
    size_t index;
    unsigned char value = (unsigned char)(writer * 31u + iteration * 17u);
    for (index = 0u; index < PAYLOAD_SIZE; ++index) {
        payload[index] = (unsigned char)(value + (unsigned char)(index * 13u));
    }
    {
        char prefix[32];
        int written = snprintf(prefix, sizeof(prefix), "writer=%u;iteration=%u;",
                               writer, iteration);
        if (written > 0 && (size_t)written < sizeof(prefix)) {
            (void)memcpy(payload, prefix, (size_t)written);
        }
    }
}

int main(int argc, char** argv)
{
    unsigned char* payload;
    char* end = NULL;
    unsigned writer;
    unsigned iterations;
    unsigned iteration;
    int exclusive;
    int result = 2;

    if (argc != 5 || (strcmp(argv[1], "atomic") != 0 &&
                      strcmp(argv[1], "exclusive") != 0)) {
        return 2;
    }
    exclusive = strcmp(argv[1], "exclusive") == 0;
    writer = (unsigned)strtoul(argv[3], &end, 10);
    if (end == argv[3] || *end != '\0') return 2;
    iterations = (unsigned)strtoul(argv[4], &end, 10);
    if (end == argv[4] || *end != '\0' || iterations == 0u) return 2;
    payload = (unsigned char*)malloc(PAYLOAD_SIZE);
    if (payload == NULL) return 2;

    result = 0;
    for (iteration = 0u; iteration < iterations; ++iteration) {
        bool written;
        make_payload(writer, iteration, payload);
        written = exclusive ?
            wz_host_output_write_exclusive(argv[2], payload, PAYLOAD_SIZE) :
            wz_host_output_write_atomic(argv[2], payload, PAYLOAD_SIZE);
        if (written) continue;
        result = exclusive ? 3 : 4;
        break;
    }
    free(payload);
    return result;
}
