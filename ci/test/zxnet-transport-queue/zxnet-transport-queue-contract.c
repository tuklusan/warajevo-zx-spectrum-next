/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdio.h>
#include <string.h>

#include "app/wz_zxnet_transport.h"

#define CHECK(test) do { \
    if (!(test)) { \
        (void)fprintf(stderr, "ZX Net transport queue failed at line %d: %s\n", \
                      __LINE__, #test); \
        return 1; \
    } \
} while (0)

static void fill_block(wz_byte_t block[256], wz_byte_t seed)
{
    for (size_t index = 0u; index < 256u; ++index) {
        block[index] = (wz_byte_t)(seed + (wz_byte_t)index);
    }
}

int main(void)
{
    wz_zxnet_transport_queue_t queue;
    wz_zxnet_transport_packet_t packet;
    wz_byte_t queue_snapshot[sizeof(queue)];
    wz_byte_t packet_snapshot[sizeof(packet)];
    wz_byte_t blocks[10][256];

    memset(&queue, 0xa5, sizeof(queue));
    wz_zxnet_transport_queue_init(&queue);
    memcpy(queue_snapshot, &queue, sizeof(queue));
    CHECK(wz_zxnet_transport_queue_size(&queue) == 0u);
    CHECK(wz_zxnet_transport_enqueue(NULL, 1u, blocks[0], 256u, 0u) ==
          WZ_RESULT_INVALID_ARGUMENT);
    CHECK(wz_zxnet_transport_enqueue(&queue, 1u, NULL, 256u, 0u) ==
          WZ_RESULT_INVALID_ARGUMENT);
    CHECK(wz_zxnet_transport_enqueue(&queue, 1u, blocks[0], 255u, 0u) ==
          WZ_RESULT_INVALID_ARGUMENT);
    CHECK(wz_zxnet_transport_queue_size(&queue) == 0u);
    CHECK(memcmp(&queue, queue_snapshot, sizeof(queue)) == 0);

    for (size_t index = 0u; index < 10u; ++index) {
        fill_block(blocks[index], (wz_byte_t)(index * 17u));
    }

    CHECK(wz_zxnet_transport_enqueue(&queue, 0x1001u, blocks[0], 256u, 10u) ==
          WZ_RESULT_OK);
    CHECK(wz_zxnet_transport_enqueue(&queue, 0x1002u, blocks[1], 256u, 5u) ==
          WZ_RESULT_OK);
    CHECK(wz_zxnet_transport_enqueue(&queue, 0x1003u, blocks[2], 256u, 10u) ==
          WZ_RESULT_OK);
    CHECK(queue.packets[0].master_tick == 10u &&
          queue.packets[1].master_tick == 10u &&
          queue.packets[2].master_tick == 10u);
    CHECK(queue.packets[0].sequence == 0u &&
          queue.packets[1].sequence == 1u &&
          queue.packets[2].sequence == 2u);

    memset(&packet, 0x5a, sizeof(packet));
    memcpy(packet_snapshot, &packet, sizeof(packet));
    memcpy(queue_snapshot, &queue, sizeof(queue));
    CHECK(wz_zxnet_transport_dequeue_due(&queue, 9u, &packet) ==
          WZ_RESULT_INVALID_STATE);
    CHECK(memcmp(&queue, queue_snapshot, sizeof(queue)) == 0);
    CHECK(memcmp(&packet, packet_snapshot, sizeof(packet)) == 0);
    CHECK(wz_zxnet_transport_dequeue_due(&queue, 10u, NULL) ==
          WZ_RESULT_INVALID_ARGUMENT);
    CHECK(memcmp(&queue, queue_snapshot, sizeof(queue)) == 0);

    CHECK(wz_zxnet_transport_dequeue_due(&queue, 10u, &packet) == WZ_RESULT_OK);
    CHECK(packet.block_id == 0x1001u && packet.master_tick == 10u &&
          packet.sequence == 0u && packet.length == 256u &&
          memcmp(packet.data, blocks[0], 256u) == 0);
    CHECK(wz_zxnet_transport_dequeue_due(&queue, 10u, &packet) == WZ_RESULT_OK);
    CHECK(packet.block_id == 0x1002u && packet.sequence == 1u &&
          memcmp(packet.data, blocks[1], 256u) == 0);
    CHECK(wz_zxnet_transport_dequeue_due(&queue, 10u, &packet) == WZ_RESULT_OK);
    CHECK(packet.block_id == 0x1003u && packet.sequence == 2u &&
          memcmp(packet.data, blocks[2], 256u) == 0);
    CHECK(wz_zxnet_transport_queue_size(&queue) == 0u);

    for (size_t index = 0u; index < WZ_ZXNET_TRANSPORT_QUEUE_CAPACITY; ++index) {
        CHECK(wz_zxnet_transport_enqueue(
                  &queue, (wz_word_t)(0x2000u + index), blocks[index],
                  256u, (wz_master_tick_t)(20u + index)) == WZ_RESULT_OK);
    }
    memcpy(queue_snapshot, &queue, sizeof(queue));
    CHECK(wz_zxnet_transport_enqueue(&queue, 0x2fffu, blocks[8], 256u, 99u) ==
          WZ_RESULT_INVALID_STATE);
    CHECK(memcmp(&queue, queue_snapshot, sizeof(queue)) == 0);

    CHECK(wz_zxnet_transport_dequeue_due(&queue, 20u, &packet) == WZ_RESULT_OK);
    CHECK(packet.block_id == 0x2000u && packet.sequence == 3u);
    CHECK(wz_zxnet_transport_enqueue(&queue, 0x3000u, blocks[9], 256u, 0u) ==
          WZ_RESULT_OK);
    CHECK(queue.packets[(queue.head + queue.count - 1u) %
                        WZ_ZXNET_TRANSPORT_QUEUE_CAPACITY].master_tick == 27u);
    CHECK(queue.packets[(queue.head + queue.count - 1u) %
                        WZ_ZXNET_TRANSPORT_QUEUE_CAPACITY].sequence == 11u);

    for (size_t index = 1u; index < WZ_ZXNET_TRANSPORT_QUEUE_CAPACITY; ++index) {
        CHECK(wz_zxnet_transport_dequeue_due(
                  &queue, UINT64_MAX, &packet) == WZ_RESULT_OK);
        CHECK(packet.block_id == (wz_word_t)(0x2000u + index));
        CHECK(packet.sequence == (wz_qword_t)(3u + index));
    }
    CHECK(wz_zxnet_transport_dequeue_due(&queue, UINT64_MAX, &packet) ==
          WZ_RESULT_OK);
    CHECK(packet.block_id == 0x3000u && packet.sequence == 11u &&
          memcmp(packet.data, blocks[9], 256u) == 0);
    CHECK(wz_zxnet_transport_queue_size(&queue) == 0u);

    (void)puts("PASS ZX Net transport normalization queue ordering, due delivery, and atomic rejection");
    return 0;
}
