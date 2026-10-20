/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "core/wz_zxnet.h"

#define CHECK(test) do { \
    if (!(test)) { \
        (void)fprintf(stderr, "ZX Net behavior failed at line %d: %s\n", \
                      __LINE__, #test); \
        return 1; \
    } \
} while (0)

typedef struct {
    const wz_byte_t* expected;
    wz_byte_t* output;
    size_t calls;
    size_t length;
    wz_word_t block_id;
    bool fail;
    bool matches;
} write_probe_t;

static wz_byte_t rotate_right(wz_byte_t value, unsigned shift)
{
    shift %= 8u;
    return shift == 0u ? value :
        (wz_byte_t)((value >> shift) | (value << (8u - shift)));
}

static bool read_block(wz_zxnet_t* network, const wz_byte_t* expected)
{
    wz_byte_t value;
    for (size_t byte = 0u; byte < WZ_ZXNET_DATA_CAPACITY; ++byte) {
        for (unsigned phase = 0u; phase < 9u; ++phase) {
            if (wz_zxnet_read(network, &value) != WZ_RESULT_OK ||
                value != rotate_right(expected[byte], phase)) {
                return false;
            }
        }
    }
    return network->state.state == WZ_ZXNET_IDLE &&
        !network->state.read_block_ready &&
        network->state.buffer_position == WZ_ZXNET_DATA_CAPACITY;
}

static wz_result_t save_block(wz_word_t id, const wz_byte_t* data,
                              size_t length, void* context)
{
    write_probe_t* probe = (write_probe_t*)context;
    if (probe == NULL || data == NULL) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    ++probe->calls;
    probe->block_id = id;
    probe->length = length;
    probe->matches = length == WZ_ZXNET_DATA_CAPACITY &&
        memcmp(data, probe->expected, length) == 0;
    if (probe->matches) {
        memcpy(probe->output, data, length);
    }
    return probe->fail ? WZ_RESULT_INVALID_STATE : WZ_RESULT_OK;
}

int main(void)
{
    wz_zxnet_t receive = {0}, status = {0}, transmit = {0};
    wz_byte_t input[WZ_ZXNET_DATA_CAPACITY];
    wz_byte_t output[WZ_ZXNET_DATA_CAPACITY] = {0}, value = 0u;
    write_probe_t probe = {0};

    wz_zxnet_init(&receive);
    for (size_t i = 0u; i < sizeof(input); ++i) {
        input[i] = (wz_byte_t)(i * 37u + 0xa5u);
    }
    CHECK(wz_zxnet_feed_read_block(&receive, 7u, input,
          sizeof(input) - 1u) == WZ_RESULT_INVALID_ARGUMENT);
    CHECK(receive.state.state == WZ_ZXNET_IDLE && !receive.state.read_block_ready);
    CHECK(wz_zxnet_feed_read_block(&receive, 7u, input, sizeof(input)) ==
          WZ_RESULT_OK);
    CHECK(wz_zxnet_start_read_collection(&receive) == WZ_RESULT_OK);
    CHECK(read_block(&receive, input));

    wz_zxnet_init(&status);
    CHECK(wz_zxnet_feed_read_block(&status, 7u, input, sizeof(input)) ==
          WZ_RESULT_OK);
    CHECK(wz_zxnet_read(&status, &value) == WZ_RESULT_OK && value == 30u);
    CHECK(status.state.state == WZ_ZXNET_BUSY && status.state.network_count == 5u);
    for (unsigned i = 0u; i < 5u; ++i) {
        CHECK(wz_zxnet_read(&status, &value) == WZ_RESULT_OK && value == 31u);
    }
    CHECK(status.state.state == WZ_ZXNET_IDLE);

    wz_zxnet_init(&status);
    CHECK(wz_zxnet_feed_read_block(&status, 0u, input, sizeof(input)) ==
          WZ_RESULT_OK);
    CHECK(wz_zxnet_read(&status, &value) == WZ_RESULT_OK && value == 30u);
    CHECK(status.state.state == WZ_ZXNET_FREE && status.state.network_count == 5u);
    for (unsigned i = 0u; i < 5u; ++i) {
        CHECK(wz_zxnet_read(&status, &value) == WZ_RESULT_OK && value == 30u);
    }
    CHECK(status.state.state == WZ_ZXNET_IDLE);
    CHECK(wz_zxnet_begin_claim(&status, 0x42u) == WZ_RESULT_OK);
    CHECK(wz_zxnet_read(&status, &value) == WZ_RESULT_OK && value == 0x42u);
    CHECK(wz_zxnet_begin_claim(&status, 0x24u) == WZ_RESULT_OK);
    CHECK(wz_zxnet_accept_claim(&status, false) == WZ_RESULT_OK);
    CHECK(status.state.state == WZ_ZXNET_FREE && status.state.network_count == 5u);
    for (unsigned i = 0u; i < 5u; ++i) {
        CHECK(wz_zxnet_read(&status, &value) == WZ_RESULT_OK && value == 30u);
    }
    CHECK(status.state.state == WZ_ZXNET_IDLE);

    wz_zxnet_init(&transmit);
    probe.expected = input;
    probe.output = output;
    wz_zxnet_set_write_callback(&transmit, save_block, &probe);
    CHECK(wz_zxnet_begin_claim(&transmit, 0x42u) == WZ_RESULT_OK);
    CHECK(wz_zxnet_accept_claim(&transmit, true) == WZ_RESULT_OK);
    for (size_t byte = 0u; byte < sizeof(input); ++byte) {
        CHECK(wz_zxnet_write_bit(&transmit, false) == WZ_RESULT_OK);
        for (unsigned bit = 0u; bit < 8u; ++bit) {
            CHECK(wz_zxnet_write_bit(&transmit,
                  ((input[byte] >> bit) & 1u) != 0u) == WZ_RESULT_OK);
        }
        CHECK(wz_zxnet_write_bit(&transmit, false) == WZ_RESULT_OK);
    }
    CHECK(transmit.state.buffer_position == sizeof(input) &&
          transmit.state.buffer_length == sizeof(input));
    probe.fail = true;
    CHECK(wz_zxnet_finish_write(&transmit) == WZ_RESULT_INVALID_STATE);
    CHECK(probe.calls == 1u && probe.block_id == 1u &&
          probe.length == sizeof(input) && probe.matches);
    CHECK(transmit.state.state == WZ_ZXNET_COLLWRITE &&
          transmit.state.block_id == 0u && transmit.state.buffer_position == sizeof(input));
    probe.fail = false;
    CHECK(wz_zxnet_finish_write(&transmit) == WZ_RESULT_OK);
    CHECK(probe.calls == 2u && probe.block_id == 1u && probe.matches);
    CHECK(transmit.state.state == WZ_ZXNET_IDLE && transmit.state.block_id == 1u);
    CHECK(memcmp(output, input, sizeof(input)) == 0);
    CHECK(wz_zxnet_feed_read_block(&receive, 1u, output, sizeof(output)) ==
          WZ_RESULT_OK);
    CHECK(wz_zxnet_start_read_collection(&receive) == WZ_RESULT_OK);
    CHECK(read_block(&receive, output));

    (void)puts("PASS ZX Net block loopback and transfer state behavior");
    return 0;
}
