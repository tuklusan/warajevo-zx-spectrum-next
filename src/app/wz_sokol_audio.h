/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_SOKOL_AUDIO_H
#define WZ_APP_WZ_SOKOL_AUDIO_H

#include <stdbool.h>
#include <stddef.h>

#include "app/wz_speed_policy.h"
#include "core/audio/wz_audio_policy.h"

typedef struct {
    bool initialized;
} wz_sokol_audio_t;

bool wz_sokol_audio_init(wz_sokol_audio_t* audio);
void wz_sokol_audio_shutdown(wz_sokol_audio_t* audio);
bool wz_sokol_audio_valid(const wz_sokol_audio_t* audio);
size_t wz_sokol_audio_push(wz_sokol_audio_t* audio,
                           wz_speed_policy_t speed,
                           const wz_audio_sample_t* samples,
                           size_t count);

#endif
