/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_zxnet.h"

#include <string.h>

static void wz_zxnet_reset_buffer(wz_zxnet_t* network)
{
    network->state.buffer_position = 0u;
    network->state.buffer_length = 0u;
    network->state.bit_count = 9u;
}

void wz_zxnet_init(wz_zxnet_t* network)
{
    if (network == 0) {
        return;
    }
    memset(network, 0, sizeof(*network));
    network->state.state = WZ_ZXNET_IDLE;
    network->state.network_delay = WZ_ZXNET_DEFAULT_DELAY;
    network->state.busy_length = WZ_ZXNET_DEFAULT_BUSY_LENGTH;
    network->state.free_length = WZ_ZXNET_DEFAULT_FREE_LENGTH;
    network->state.bit_count = 9u;
}

wz_result_t wz_zxnet_snapshot(const wz_zxnet_t* network,
                              wz_zxnet_snapshot_t* output)
{
    if (network == 0 || output == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *output = network->state;
    return WZ_RESULT_OK;
}

wz_result_t wz_zxnet_feed_read_block(wz_zxnet_t* network,
                                     wz_word_t block_id,
                                     const wz_byte_t* data,
                                     size_t length)
{
    if (network == 0 || data == 0 || length != WZ_ZXNET_DATA_CAPACITY ||
        network->state.read_block_ready) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    memcpy(network->state.buffer, data, length);
    network->state.block_id = block_id;
    network->state.buffer_position = 0u;
    network->state.buffer_length = length;
    network->state.read_block_ready = true;
    return WZ_RESULT_OK;
}

wz_result_t wz_zxnet_start_read_collection(wz_zxnet_t* network)
{
    if (network == 0 || !network->state.read_block_ready ||
        network->state.buffer_length != WZ_ZXNET_DATA_CAPACITY) {
        return WZ_RESULT_INVALID_STATE;
    }
    network->state.network_byte = network->state.buffer[0];
    network->state.buffer_position = 0u;
    network->state.bit_count = 9u;
    network->state.state = WZ_ZXNET_COLLREAD;
    return WZ_RESULT_OK;
}

wz_result_t wz_zxnet_begin_claim(wz_zxnet_t* network, wz_byte_t claim_byte)
{
    if (network == 0 || network->state.state == WZ_ZXNET_COLLWRITE) {
        return WZ_RESULT_INVALID_STATE;
    }
    network->state.claim_byte = claim_byte;
    network->state.state = WZ_ZXNET_CLAIM;
    return WZ_RESULT_OK;
}

wz_result_t wz_zxnet_accept_claim(wz_zxnet_t* network, bool accepted)
{
    if (network == 0 || network->state.state != WZ_ZXNET_CLAIM) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (!accepted) {
        network->state.network_count = network->state.free_length;
        network->state.state = WZ_ZXNET_FREE;
        return WZ_RESULT_OK;
    }
    wz_zxnet_reset_buffer(network);
    network->state.state = WZ_ZXNET_COLLWRITE;
    return WZ_RESULT_OK;
}

wz_result_t wz_zxnet_read(wz_zxnet_t* network, wz_byte_t* value)
{
    if (network == 0 || value == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    switch (network->state.state) {
    case WZ_ZXNET_CLAIM:
        *value = network->state.claim_byte;
        network->state.state = WZ_ZXNET_IDLE;
        return WZ_RESULT_OK;
    case WZ_ZXNET_BUSY:
        *value = 31u;
        if (--network->state.network_count == 0u) {
            network->state.state = WZ_ZXNET_IDLE;
        }
        return WZ_RESULT_OK;
    case WZ_ZXNET_FREE:
        *value = 30u;
        if (--network->state.network_count == 0u) {
            network->state.state = WZ_ZXNET_IDLE;
        }
        return WZ_RESULT_OK;
    case WZ_ZXNET_COLLREAD:
        *value = network->state.network_byte;
        network->state.network_byte = (wz_byte_t)((network->state.network_byte >> 1u) |
                                                  (network->state.network_byte << 7u));
        if (--network->state.bit_count == 0u) {
            network->state.bit_count = 9u;
            if (++network->state.buffer_position >= network->state.buffer_length) {
                network->state.read_block_ready = false;
                network->state.state = WZ_ZXNET_IDLE;
            } else {
                network->state.network_byte = network->state.buffer[
                    network->state.buffer_position];
            }
        }
        return WZ_RESULT_OK;
    case WZ_ZXNET_IDLE:
        *value = 30u;
        if (network->state.read_block_ready) {
            network->state.state = network->state.block_id ==
                                   network->state.last_block_id ? WZ_ZXNET_FREE :
                                   WZ_ZXNET_BUSY;
            network->state.network_count = network->state.state == WZ_ZXNET_FREE ?
                                            network->state.free_length :
                                            network->state.busy_length;
            network->state.last_block_id = network->state.block_id;
        }
        return WZ_RESULT_OK;
    default:
        return WZ_RESULT_INVALID_STATE;
    }
}

wz_result_t wz_zxnet_write_bit(wz_zxnet_t* network, bool bit)
{
    if (network == 0 || network->state.state != WZ_ZXNET_COLLWRITE) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (network->state.bit_count == 0u) {
        network->state.bit_count = 9u;
        if (network->state.buffer_position < WZ_ZXNET_DATA_CAPACITY) {
            ++network->state.buffer_position;
        }
        return WZ_RESULT_OK;
    }
    if (network->state.buffer_position < WZ_ZXNET_DATA_CAPACITY) {
        network->state.buffer[network->state.buffer_position] =
            (wz_byte_t)((network->state.buffer[network->state.buffer_position] >> 1u) |
                        (bit ? 0x80u : 0u));
        if (network->state.buffer_length < network->state.buffer_position + 1u) {
            network->state.buffer_length = network->state.buffer_position + 1u;
        }
    }
    --network->state.bit_count;
    return WZ_RESULT_OK;
}

wz_result_t wz_zxnet_finish_write(wz_zxnet_t* network)
{
    wz_result_t result;
    wz_word_t next_id;

    if (network == 0 || network->state.state != WZ_ZXNET_COLLWRITE ||
        network->write_callback == 0) {
        return WZ_RESULT_INVALID_STATE;
    }
    next_id = (wz_word_t)(network->state.block_id + 1u);
    result = network->write_callback(next_id, network->state.buffer,
                                     network->state.buffer_length,
                                     network->write_context);
    if (result != WZ_RESULT_OK) {
        return result;
    }
    network->state.block_id = next_id;
    network->state.last_block_id = next_id;
    network->state.state = WZ_ZXNET_IDLE;
    network->state.read_block_ready = false;
    wz_zxnet_reset_buffer(network);
    return WZ_RESULT_OK;
}

void wz_zxnet_set_write_callback(wz_zxnet_t* network,
                                 wz_zxnet_write_callback_t callback,
                                 void* context)
{
    if (network != 0) {
        network->write_callback = callback;
        network->write_context = context;
    }
}
