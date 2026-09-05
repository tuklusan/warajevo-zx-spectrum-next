/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_zxnet_transport.h"

#include <string.h>

void wz_zxnet_transport_queue_init(wz_zxnet_transport_queue_t* queue)
{
    if (queue != 0) {
        memset(queue, 0, sizeof(*queue));
    }
}

wz_result_t wz_zxnet_transport_enqueue(
    wz_zxnet_transport_queue_t* queue,
    wz_word_t block_id,
    const wz_byte_t* data,
    size_t length,
    wz_master_tick_t requested_tick)
{
    wz_zxnet_transport_packet_t* packet;
    wz_master_tick_t tick;

    if (queue == 0 || data == 0 || length != sizeof(packet->data)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (queue->count >= WZ_ZXNET_TRANSPORT_QUEUE_CAPACITY ||
        queue->next_sequence == UINT64_MAX) {
        return WZ_RESULT_INVALID_STATE;
    }
    tick = requested_tick;
    if (queue->assigned && tick < queue->last_tick) {
        tick = queue->last_tick;
    }
    packet = &queue->packets[(queue->head + queue->count) %
                             WZ_ZXNET_TRANSPORT_QUEUE_CAPACITY];
    packet->block_id = block_id;
    packet->master_tick = tick;
    packet->sequence = queue->next_sequence;
    packet->length = length;
    memcpy(packet->data, data, length);
    queue->last_tick = tick;
    queue->next_sequence += 1u;
    queue->assigned = true;
    queue->count += 1u;
    return WZ_RESULT_OK;
}

wz_result_t wz_zxnet_transport_dequeue_due(
    wz_zxnet_transport_queue_t* queue,
    wz_master_tick_t current_tick,
    wz_zxnet_transport_packet_t* packet)
{
    if (queue == 0 || packet == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (queue->count == 0u ||
        queue->packets[queue->head].master_tick > current_tick) {
        return WZ_RESULT_INVALID_STATE;
    }
    *packet = queue->packets[queue->head];
    queue->head = (queue->head + 1u) % WZ_ZXNET_TRANSPORT_QUEUE_CAPACITY;
    queue->count -= 1u;
    return WZ_RESULT_OK;
}

size_t wz_zxnet_transport_queue_size(
    const wz_zxnet_transport_queue_t* queue)
{
    return queue == 0 ? 0u : queue->count;
}
