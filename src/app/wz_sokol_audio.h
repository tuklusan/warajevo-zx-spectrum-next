/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. */

#ifndef WZ_APP_WZ_SOKOL_AUDIO_H
#define WZ_APP_WZ_SOKOL_AUDIO_H

#include <stdbool.h>
#include <stddef.h>

#include "app/wz_host_audio_push.h"
#include "app/wz_speed_policy.h"
#include "core/audio/wz_audio_policy.h"

typedef struct {
    bool initialized;
    wz_host_audio_push_queue_t pending;
} wz_sokol_audio_t;

bool wz_sokol_audio_init(wz_sokol_audio_t* audio);
void wz_sokol_audio_shutdown(wz_sokol_audio_t* audio);
bool wz_sokol_audio_valid(const wz_sokol_audio_t* audio);
void wz_sokol_audio_discard_pending(wz_sokol_audio_t* audio);
size_t wz_sokol_audio_push(wz_sokol_audio_t* audio,
                           wz_speed_policy_t speed,
                           const wz_audio_sample_t* samples,
                           size_t count);

#endif
