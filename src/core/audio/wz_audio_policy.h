/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_AUDIO_WZ_AUDIO_POLICY_H
#define WZ_CORE_AUDIO_WZ_AUDIO_POLICY_H

#include <stdint.h>

#define WZ_CANONICAL_AUDIO_SAMPLE_RATE 44100u
#define WZ_AUDIO_MIXER_FRACTION_BITS 16u
#define WZ_AUDIO_MIXER_ONE (1 << WZ_AUDIO_MIXER_FRACTION_BITS)
#define WZ_AUDIO_MIXER_MIN INT32_MIN
#define WZ_AUDIO_MIXER_MAX INT32_MAX

typedef int32_t wz_audio_sample_t;
typedef int64_t wz_audio_accumulator_t;

#endif
