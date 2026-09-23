/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_AUDIO_WZ_AY_MIXER_POLICY_H
#define WZ_CORE_AUDIO_WZ_AY_MIXER_POLICY_H

#include <stdint.h>

#define WZ_AY_CHANNEL_COUNT 3u
#define WZ_AY_VOLUME_LEVEL_COUNT 16u

/* Normalized AY attenuation targets in Q16.16, level 15 is unity. */
static const uint32_t WZ_AY_VOLUME_GAIN_Q16_16[WZ_AY_VOLUME_LEVEL_COUNT] = {
    0u, 4096u, 5793u, 8192u, 11585u, 16384u, 23170u, 32768u,
    40960u, 46340u, 52016u, 58386u, 65536u, 65536u, 65536u, 65536u
};

#endif
