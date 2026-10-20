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

static void wz_zxnet_trace(wz_zxnet_t* network, wz_byte_t action,
                           wz_zxnet_state_t previous_state,
                           wz_word_t previous_block_id)
{
    wz_trace_event_t event = {0};
    if (network == 0 || network->timing_trace == 0) {
        return;
    }
    event.kind = WZ_TRACE_ZXNET_EVENT;
    event.master_tick = network->master_tick;
    event.address = previous_block_id;
    event.program_counter = network->state.block_id;
    event.value = (wz_byte_t)previous_state;
    event.auxiliary = (wz_byte_t)network->state.state;
    event.cycle = action;
    event.t_states = network->state.bit_count;
    event.register_snapshot = (wz_qword_t)network->state.buffer_position;
    wz_trace_emit_detail(network->timing_trace, &event);
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
    network->timing_trace = 0;
    network->master_tick = 0u;
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
    wz_zxnet_state_t previous_state;
    wz_word_t previous_block_id;
    if (network == 0 || data == 0 || length != WZ_ZXNET_DATA_CAPACITY ||
        network->state.read_block_ready) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    previous_state = network->state.state;
    previous_block_id = network->state.block_id;
    memcpy(network->state.buffer, data, length);
    network->state.block_id = block_id;
    network->state.buffer_position = 0u;
    network->state.buffer_length = length;
    network->state.read_block_ready = true;
    wz_zxnet_trace(network, WZ_TRACE_ZXNET_BLOCK_QUEUED, previous_state,
                   previous_block_id);
    return WZ_RESULT_OK;
}

wz_result_t wz_zxnet_start_read_collection(wz_zxnet_t* network)
{
    wz_zxnet_state_t previous_state;
    wz_word_t previous_block_id;
    if (network == 0 || !network->state.read_block_ready ||
        network->state.buffer_length != WZ_ZXNET_DATA_CAPACITY) {
        return WZ_RESULT_INVALID_STATE;
    }
    previous_state = network->state.state;
    previous_block_id = network->state.block_id;
    network->state.network_byte = network->state.buffer[0];
    network->state.buffer_position = 0u;
    network->state.bit_count = 9u;
    network->state.state = WZ_ZXNET_COLLREAD;
    wz_zxnet_trace(network, WZ_TRACE_ZXNET_READ_STARTED, previous_state,
                   previous_block_id);
    return WZ_RESULT_OK;
}

wz_result_t wz_zxnet_begin_claim(wz_zxnet_t* network, wz_byte_t claim_byte)
{
    wz_zxnet_state_t previous_state;
    wz_word_t previous_block_id;
    if (network == 0 || network->state.state == WZ_ZXNET_COLLREAD ||
        network->state.state == WZ_ZXNET_COLLWRITE) {
        return WZ_RESULT_INVALID_STATE;
    }
    previous_state = network->state.state;
    previous_block_id = network->state.block_id;
    network->state.claim_byte = claim_byte;
    network->state.state = WZ_ZXNET_CLAIM;
    wz_zxnet_trace(network, WZ_TRACE_ZXNET_CLAIM_STARTED, previous_state,
                   previous_block_id);
    return WZ_RESULT_OK;
}

wz_result_t wz_zxnet_accept_claim(wz_zxnet_t* network, bool accepted)
{
    wz_zxnet_state_t previous_state;
    wz_word_t previous_block_id;
    if (network == 0 || network->state.state != WZ_ZXNET_CLAIM) {
        return WZ_RESULT_INVALID_STATE;
    }
    previous_state = network->state.state;
    previous_block_id = network->state.block_id;
    if (!accepted) {
        network->state.network_count = network->state.free_length;
        network->state.state = WZ_ZXNET_FREE;
        wz_zxnet_trace(network, WZ_TRACE_ZXNET_CLAIM_RESOLVED,
                       previous_state, previous_block_id);
        return WZ_RESULT_OK;
    }
    wz_zxnet_reset_buffer(network);
    network->state.state = WZ_ZXNET_COLLWRITE;
    wz_zxnet_trace(network, WZ_TRACE_ZXNET_CLAIM_RESOLVED, previous_state,
                   previous_block_id);
    return WZ_RESULT_OK;
}

wz_result_t wz_zxnet_read(wz_zxnet_t* network, wz_byte_t* value)
{
    wz_zxnet_state_t previous_state;
    wz_word_t previous_block_id;
    if (network == 0 || value == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    previous_state = network->state.state;
    previous_block_id = network->state.block_id;
    switch (network->state.state) {
    case WZ_ZXNET_CLAIM:
        *value = network->state.claim_byte;
        network->state.state = WZ_ZXNET_IDLE;
        break;
    case WZ_ZXNET_BUSY:
        *value = 31u;
        if (--network->state.network_count == 0u) {
            network->state.state = WZ_ZXNET_IDLE;
        }
        break;
    case WZ_ZXNET_FREE:
        *value = 30u;
        if (--network->state.network_count == 0u) {
            network->state.state = WZ_ZXNET_IDLE;
        }
        break;
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
        break;
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
        break;
    default:
        return WZ_RESULT_INVALID_STATE;
    }
    wz_zxnet_trace(network, WZ_TRACE_ZXNET_READ_PROGRESS, previous_state,
                   previous_block_id);
    return WZ_RESULT_OK;
}

wz_result_t wz_zxnet_write_bit(wz_zxnet_t* network, bool bit)
{
    wz_zxnet_state_t previous_state;
    wz_word_t previous_block_id;
    if (network == 0 || network->state.state != WZ_ZXNET_COLLWRITE) {
        return WZ_RESULT_INVALID_STATE;
    }
    previous_state = network->state.state;
    previous_block_id = network->state.block_id;
    if (network->state.bit_count == 0u) {
        network->state.bit_count = 9u;
        if (network->state.buffer_position < WZ_ZXNET_DATA_CAPACITY) {
            ++network->state.buffer_position;
        }
        wz_zxnet_trace(network, WZ_TRACE_ZXNET_WRITE_PROGRESS,
                       previous_state, previous_block_id);
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
    wz_zxnet_trace(network, WZ_TRACE_ZXNET_WRITE_PROGRESS, previous_state,
                   previous_block_id);
    return WZ_RESULT_OK;
}

wz_result_t wz_zxnet_finish_write(wz_zxnet_t* network)
{
    wz_result_t result;
    wz_word_t next_id;
    wz_word_t previous_block_id;
    wz_zxnet_state_t previous_state;

    if (network == 0 || network->state.state != WZ_ZXNET_COLLWRITE ||
        network->write_callback == 0) {
        return WZ_RESULT_INVALID_STATE;
    }
    previous_block_id = network->state.block_id;
    previous_state = network->state.state;
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
    wz_zxnet_trace(network, WZ_TRACE_ZXNET_WRITE_COMMITTED, previous_state,
                   previous_block_id);
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

void wz_zxnet_set_timing_trace(wz_zxnet_t* network, wz_trace_sink_t* trace)
{
    if (network != 0) {
        network->timing_trace = trace;
    }
}

void wz_zxnet_set_master_tick(wz_zxnet_t* network,
                              wz_master_tick_t master_tick)
{
    if (network != 0) {
        network->master_tick = master_tick;
    }
}
