/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. */

#ifndef WZ_APP_WZ_HOST_AUDIO_PUSH_H
#define WZ_APP_WZ_HOST_AUDIO_PUSH_H

#include <stddef.h>

#include "core/wz_types.h"
#include "core/audio/wz_audio_policy.h"

#define WZ_HOST_AUDIO_QUEUE_CAPACITY 4096u

typedef struct {
    wz_audio_sample_t samples[WZ_HOST_AUDIO_QUEUE_CAPACITY];
    size_t read_index;
    size_t write_index;
    size_t count;
    wz_qword_t dropped_samples;
} wz_host_audio_push_queue_t;

void wz_host_audio_push_init(wz_host_audio_push_queue_t* queue);
size_t wz_host_audio_push(wz_host_audio_push_queue_t* queue,
                          const wz_audio_sample_t* samples,
                          size_t count);
size_t wz_host_audio_pop(wz_host_audio_push_queue_t* queue,
                         wz_audio_sample_t* samples,
                         size_t count);
size_t wz_host_audio_peek(const wz_host_audio_push_queue_t* queue,
                          wz_audio_sample_t* samples,
                          size_t count);
size_t wz_host_audio_discard(wz_host_audio_push_queue_t* queue, size_t count);
size_t wz_host_audio_queued(const wz_host_audio_push_queue_t* queue);
wz_qword_t wz_host_audio_dropped(const wz_host_audio_push_queue_t* queue);

#endif
