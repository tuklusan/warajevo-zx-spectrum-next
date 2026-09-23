/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_ZXNET_TRANSPORT_H
#define WZ_APP_WZ_ZXNET_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>

#include "core/wz_types.h"

#define WZ_ZXNET_TRANSPORT_QUEUE_CAPACITY 8u

/* Host packets become deterministic application events before reaching core. */
typedef struct {
    wz_word_t block_id;
    wz_master_tick_t master_tick;
    wz_qword_t sequence;
    size_t length;
    wz_byte_t data[256];
} wz_zxnet_transport_packet_t;

typedef struct {
    wz_zxnet_transport_packet_t packets[WZ_ZXNET_TRANSPORT_QUEUE_CAPACITY];
    size_t head;
    size_t count;
    wz_master_tick_t last_tick;
    wz_qword_t next_sequence;
    bool assigned;
} wz_zxnet_transport_queue_t;

void wz_zxnet_transport_queue_init(wz_zxnet_transport_queue_t* queue);
wz_result_t wz_zxnet_transport_enqueue(
    wz_zxnet_transport_queue_t* queue,
    wz_word_t block_id,
    const wz_byte_t* data,
    size_t length,
    wz_master_tick_t requested_tick);
wz_result_t wz_zxnet_transport_dequeue_due(
    wz_zxnet_transport_queue_t* queue,
    wz_master_tick_t current_tick,
    wz_zxnet_transport_packet_t* packet);
size_t wz_zxnet_transport_queue_size(
    const wz_zxnet_transport_queue_t* queue);

#endif
