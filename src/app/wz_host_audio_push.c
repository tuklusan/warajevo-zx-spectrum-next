/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_host_audio_push.h"

#include <stdint.h>
#include <string.h>

void wz_host_audio_push_init(wz_host_audio_push_queue_t* queue)
{
    if (queue != 0) {
        memset(queue, 0, sizeof(*queue));
    }
}

size_t wz_host_audio_push(wz_host_audio_push_queue_t* queue,
                          const wz_audio_sample_t* samples,
                          size_t count)
{
    size_t accepted;

    if (queue == 0 || (samples == 0 && count != 0u)) {
        return 0u;
    }
    accepted = count < WZ_HOST_AUDIO_QUEUE_CAPACITY - queue->count
                   ? count
                   : WZ_HOST_AUDIO_QUEUE_CAPACITY - queue->count;
    for (size_t index = 0u; index < accepted; ++index) {
        queue->samples[queue->write_index] = samples[index];
        queue->write_index = (queue->write_index + 1u) % WZ_HOST_AUDIO_QUEUE_CAPACITY;
    }
    queue->count += accepted;
    if ((wz_qword_t)(count - accepted) > UINT64_MAX - queue->dropped_samples) {
        queue->dropped_samples = UINT64_MAX;
    } else {
        queue->dropped_samples += (wz_qword_t)(count - accepted);
    }
    return accepted;
}

size_t wz_host_audio_pop(wz_host_audio_push_queue_t* queue,
                         wz_audio_sample_t* samples,
                         size_t count)
{
    size_t accepted;

    if (queue == 0 || (samples == 0 && count != 0u)) {
        return 0u;
    }
    accepted = count < queue->count ? count : queue->count;
    for (size_t index = 0u; index < accepted; ++index) {
        samples[index] = queue->samples[queue->read_index];
        queue->read_index = (queue->read_index + 1u) % WZ_HOST_AUDIO_QUEUE_CAPACITY;
    }
    queue->count -= accepted;
    return accepted;
}

size_t wz_host_audio_queued(const wz_host_audio_push_queue_t* queue)
{
    return queue == 0 ? 0u : queue->count;
}

wz_qword_t wz_host_audio_dropped(const wz_host_audio_push_queue_t* queue)
{
    return queue == 0 ? 0u : queue->dropped_samples;
}
