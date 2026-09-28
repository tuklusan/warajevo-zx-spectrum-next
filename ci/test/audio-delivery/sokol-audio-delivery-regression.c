/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. */

#include "app/wz_sokol_audio.h"

#include <math.h>
#include <stdio.h>

#include "sokol_audio.h"

#define CAPTURE_CAPACITY 8192u

static bool backend_valid;
static int backend_accept_limit;
static float captured[CAPTURE_CAPACITY];
static size_t captured_count;
static saudio_desc configured;

void saudio_setup(const saudio_desc* description)
{
    configured = *description;
    backend_valid = true;
}

void saudio_shutdown(void)
{
    backend_valid = false;
}

bool saudio_isvalid(void)
{
    return backend_valid;
}

int saudio_push(const float* frames, int num_frames)
{
    int accepted = num_frames < backend_accept_limit
                       ? num_frames
                       : backend_accept_limit;

    for (int index = 0; index < accepted; ++index) {
        if (captured_count >= CAPTURE_CAPACITY) {
            return 0;
        }
        captured[captured_count++] = frames[index];
    }
    return accepted;
}

static int require(bool condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "FAIL %s\n", name);
        return 1;
    }
    printf("PASS %s\n", name);
    return 0;
}

static bool verify_pending_ring(void)
{
    wz_host_audio_push_queue_t queue;
    wz_audio_sample_t initial[WZ_HOST_AUDIO_QUEUE_CAPACITY + 904u];
    wz_audio_sample_t appended[3000u];
    wz_audio_sample_t observed[WZ_HOST_AUDIO_QUEUE_CAPACITY];

    for (size_t index = 0u; index < WZ_HOST_AUDIO_QUEUE_CAPACITY + 904u; ++index) {
        initial[index] = (wz_audio_sample_t)(index % 30000u);
    }
    for (size_t index = 0u; index < 3000u; ++index) {
        appended[index] = (wz_audio_sample_t)(1000u + index);
    }
    wz_host_audio_push_init(&queue);
    if (wz_host_audio_push(&queue, initial,
                           WZ_HOST_AUDIO_QUEUE_CAPACITY + 904u) !=
            WZ_HOST_AUDIO_QUEUE_CAPACITY ||
        wz_host_audio_dropped(&queue) != 904u ||
        wz_host_audio_discard(&queue, 3000u) != 3000u ||
        wz_host_audio_push(&queue, appended, 3000u) != 3000u ||
        wz_host_audio_peek(&queue, observed, WZ_HOST_AUDIO_QUEUE_CAPACITY) !=
            WZ_HOST_AUDIO_QUEUE_CAPACITY) {
        return false;
    }
    for (size_t index = 0u; index < WZ_HOST_AUDIO_QUEUE_CAPACITY - 3000u; ++index) {
        if (observed[index] != initial[index + 3000u]) {
            return false;
        }
    }
    for (size_t index = WZ_HOST_AUDIO_QUEUE_CAPACITY - 3000u;
         index < WZ_HOST_AUDIO_QUEUE_CAPACITY; ++index) {
        if (observed[index] != appended[index -
                                               (WZ_HOST_AUDIO_QUEUE_CAPACITY - 3000u)]) {
            return false;
        }
    }
    return true;
}

int main(void)
{
    wz_sokol_audio_t audio;
    const wz_audio_sample_t first[] = { -2000, -1000, 0, 1000, 2000, 3000 };
    const wz_audio_sample_t second[] = { 4000, 5000 };
    wz_audio_sample_t backlog[WZ_HOST_AUDIO_QUEUE_CAPACITY];
    const wz_audio_sample_t newest = 1234;
    bool samples_match = true;
    int failures = 0;

    backend_accept_limit = 2;
    failures += require(wz_sokol_audio_init(&audio), "backend_initializes");
    failures += require(configured.sample_rate == WZ_CANONICAL_AUDIO_SAMPLE_RATE &&
                        configured.num_channels == 1 && configured.buffer_frames ==
                            (int)WZ_HOST_AUDIO_QUEUE_CAPACITY,
                        "canonical_mono_device_format");
    failures += require(wz_sokol_audio_push(&audio, WZ_SPEED_100,
                                             first, 6u) == 2u &&
                        wz_host_audio_queued(&audio.pending) == 4u,
                        "partial_device_acceptance_retains_remainder");

    backend_accept_limit = 0;
    failures += require(wz_sokol_audio_push(&audio, WZ_SPEED_100,
                                             second, 2u) == 0u &&
                        wz_host_audio_queued(&audio.pending) == 6u,
                        "backpressure_preserves_frame_order");
    backend_accept_limit = 8;
    failures += require(wz_sokol_audio_push(&audio, WZ_SPEED_100,
                                             NULL, 0u) == 6u &&
                        wz_host_audio_queued(&audio.pending) == 0u &&
                        captured_count == 8u,
                        "later_drain_delivers_pending_and_new_samples");
    for (size_t index = 0u; index < 8u; ++index) {
        float expected = (float)(index < 6u ? first[index] : second[index - 6u]) /
                         (float)WZ_AUDIO_MIXER_ONE;
        if (fabsf(captured[index] - expected) >= 0.000001f) {
            samples_match = false;
        }
    }
    failures += require(samples_match,
                        "device_samples_remain_ordered_and_normalized");

    for (size_t index = 0u; index < WZ_HOST_AUDIO_QUEUE_CAPACITY; ++index) {
        backlog[index] = (wz_audio_sample_t)(index % 30000u);
    }
    (void)wz_host_audio_push(&audio.pending, backlog,
                             WZ_HOST_AUDIO_QUEUE_CAPACITY);
    backend_accept_limit = (int)WZ_HOST_AUDIO_QUEUE_CAPACITY;
    failures += require(wz_sokol_audio_push(&audio, WZ_SPEED_100,
                                             &newest, 1u) ==
                            WZ_HOST_AUDIO_QUEUE_CAPACITY + 1u &&
                        wz_host_audio_queued(&audio.pending) == 0u &&
                        wz_host_audio_dropped(&audio.pending) == 0u &&
                        captured_count == 8u + WZ_HOST_AUDIO_QUEUE_CAPACITY + 1u &&
                        fabsf(captured[captured_count - 1u] -
                              (float)newest / (float)WZ_AUDIO_MIXER_ONE) <
                            0.000001f,
                        "drain_frees_capacity_before_accepting_new_samples");

    backend_accept_limit = 0;
    (void)wz_sokol_audio_push(&audio, WZ_SPEED_100, first, 3u);
    failures += require(wz_host_audio_queued(&audio.pending) == 3u,
                        "backpressure_queue_is_visible");
    (void)wz_sokol_audio_push(&audio, WZ_SPEED_400, NULL, 0u);
    failures += require(wz_host_audio_queued(&audio.pending) == 0u,
                        "disabled_audio_discards_obsolete_pending_frames");
    wz_sokol_audio_shutdown(&audio);
    failures += require(!wz_sokol_audio_valid(&audio), "backend_shutdown");
    failures += require(verify_pending_ring(),
                        "pending_ring_wrap_and_overflow_accounting");
    return failures == 0 ? 0 : 1;
}
