/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_ZXNET_H
#define WZ_CORE_WZ_ZXNET_H

#include <stdbool.h>
#include <stddef.h>

#include "core/wz_types.h"

#define WZ_ZXNET_DATA_CAPACITY 256u
#define WZ_ZXNET_DEFAULT_BUSY_LENGTH 5u
#define WZ_ZXNET_DEFAULT_FREE_LENGTH 5u
#define WZ_ZXNET_DEFAULT_DELAY 30u

typedef enum {
    WZ_ZXNET_CLAIM = 1,
    WZ_ZXNET_IDLE,
    WZ_ZXNET_BUSY,
    WZ_ZXNET_FREE,
    WZ_ZXNET_COLLREAD,
    WZ_ZXNET_COLLWRITE
} wz_zxnet_state_t;

typedef wz_result_t (*wz_zxnet_write_callback_t)(wz_word_t block_id,
                                                  const wz_byte_t* data,
                                                  size_t length,
                                                  void* context);

typedef struct {
    wz_zxnet_state_t state;
    wz_byte_t claim_byte;
    wz_word_t block_id;
    wz_word_t last_block_id;
    wz_word_t network_delay;
    wz_word_t busy_length;
    wz_word_t free_length;
    wz_word_t network_count;
    wz_byte_t network_byte;
    wz_byte_t bit_count;
    size_t buffer_position;
    size_t buffer_length;
    wz_byte_t buffer[WZ_ZXNET_DATA_CAPACITY];
    bool read_block_ready;
} wz_zxnet_snapshot_t;

typedef struct {
    wz_zxnet_snapshot_t state;
    wz_zxnet_write_callback_t write_callback;
    void* write_context;
} wz_zxnet_t;

void wz_zxnet_init(wz_zxnet_t* network);
wz_result_t wz_zxnet_snapshot(const wz_zxnet_t* network,
                               wz_zxnet_snapshot_t* output);
wz_result_t wz_zxnet_feed_read_block(wz_zxnet_t* network,
                                     wz_word_t block_id,
                                     const wz_byte_t* data,
                                     size_t length);
wz_result_t wz_zxnet_start_read_collection(wz_zxnet_t* network);
wz_result_t wz_zxnet_begin_claim(wz_zxnet_t* network, wz_byte_t claim_byte);
wz_result_t wz_zxnet_accept_claim(wz_zxnet_t* network, bool accepted);
wz_result_t wz_zxnet_read(wz_zxnet_t* network, wz_byte_t* value);
wz_result_t wz_zxnet_write_bit(wz_zxnet_t* network, bool bit);
wz_result_t wz_zxnet_finish_write(wz_zxnet_t* network);
void wz_zxnet_set_write_callback(wz_zxnet_t* network,
                                 wz_zxnet_write_callback_t callback,
                                 void* context);

#endif
