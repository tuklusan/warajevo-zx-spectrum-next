/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_HOST_AUDIO_PUSH_H
#define WZ_APP_WZ_HOST_AUDIO_PUSH_H

#include <stddef.h>

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
size_t wz_host_audio_queued(const wz_host_audio_push_queue_t* queue);
wz_qword_t wz_host_audio_dropped(const wz_host_audio_push_queue_t* queue);

#endif
