/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "core/wz_zxnet.h"

typedef struct {
    wz_zxnet_t* peer;
    unsigned calls;
    wz_word_t block_id;
    size_t length;
    wz_byte_t first_byte;
    wz_result_t result;
} loopback_link_t;

static wz_result_t forward_loopback_block(wz_word_t block_id,
                                           const wz_byte_t* data,
                                           size_t length,
                                           void* context)
{
    loopback_link_t* link = (loopback_link_t*)context;
    link->calls += 1u;
    link->block_id = block_id;
    link->length = length;
    link->first_byte = length == 0u ? 0u : data[0];
    if (link->result != WZ_RESULT_OK) {
        return link->result;
    }
    return wz_zxnet_feed_read_block(link->peer, block_id, data, length);
}

static int run_loopback(void)
{
    wz_zxnet_t sender;
    wz_zxnet_t receiver;
    wz_zxnet_snapshot_t snapshot;
    loopback_link_t link = {0};
    wz_byte_t expected[WZ_ZXNET_DATA_CAPACITY];
    wz_byte_t value = 0u;

    for (size_t index = 0u; index < sizeof(expected); ++index) {
        expected[index] = (wz_byte_t)((index * 37u) ^ 0xa5u);
    }
    wz_zxnet_init(&sender);
    wz_zxnet_init(&receiver);
    link.peer = &receiver;
    link.result = WZ_RESULT_OK;
    wz_zxnet_set_write_callback(&sender, forward_loopback_block, &link);

    if (wz_zxnet_feed_read_block(&receiver, 1u, expected,
                                 sizeof(expected) - 1u) != WZ_RESULT_INVALID_ARGUMENT ||
        wz_zxnet_begin_claim(&sender, 0x5au) != WZ_RESULT_OK ||
        wz_zxnet_read(&sender, &value) != WZ_RESULT_OK || value != 0x5au ||
        wz_zxnet_accept_claim(&sender, true) != WZ_RESULT_INVALID_STATE) {
        fputs("ZX Net loopback claim precondition failed\n", stderr);
        return 1;
    }
    /* A claim is local to each endpoint; acceptance is exercised on a receiver claim. */
    if (wz_zxnet_begin_claim(&receiver, 0x5au) != WZ_RESULT_OK ||
        wz_zxnet_accept_claim(&receiver, true) != WZ_RESULT_OK) {
        fputs("ZX Net loopback claim acceptance failed\n", stderr);
        return 1;
    }
    for (size_t index = 0u; index < sizeof(expected); ++index) {
        for (unsigned bit = 0u; bit < 9u; ++bit) {
            bool data_bit = bit < 8u &&
                ((expected[index] & (wz_byte_t)(1u << bit)) != 0u);
            if (wz_zxnet_write_bit(&receiver, data_bit) != WZ_RESULT_OK) {
                fputs("ZX Net loopback write collection failed\n", stderr);
                return 1;
            }
        }
        if (wz_zxnet_write_bit(&receiver, false) != WZ_RESULT_OK) {
            fputs("ZX Net loopback write separator failed\n", stderr);
            return 1;
        }
    }
    /* Receiver writes back through the loopback link to the sender. */
    link.peer = &sender;
    wz_zxnet_set_write_callback(&receiver, forward_loopback_block, &link);
    if (wz_zxnet_finish_write(&receiver) != WZ_RESULT_OK || link.calls != 1u ||
        link.block_id != 1u || link.length != sizeof(expected) ||
        link.first_byte != expected[0] ||
        wz_zxnet_read(&sender, &value) != WZ_RESULT_OK || value != 30u ||
        wz_zxnet_snapshot(&sender, &snapshot) != WZ_RESULT_OK ||
        snapshot.state != WZ_ZXNET_BUSY ||
        wz_zxnet_start_read_collection(&sender) != WZ_RESULT_OK ||
        wz_zxnet_read(&sender, &value) != WZ_RESULT_OK || value != expected[0]) {
        fputs("ZX Net loopback transfer or collision-read failed\n", stderr);
        return 1;
    }
    puts("ZX Net loopback PASS");
    return 0;
}

int main(void)
{
    return run_loopback();
}
