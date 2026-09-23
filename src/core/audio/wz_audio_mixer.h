/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_AUDIO_WZ_AUDIO_MIXER_H
#define WZ_CORE_AUDIO_WZ_AUDIO_MIXER_H

#include "core/audio/wz_ay.h"
#include "core/audio/wz_audio_policy.h"

/* Mix the current AY state into one centered canonical Q16.16 sample. */
wz_audio_sample_t wz_audio_mixer_ay_sample(const wz_ay_t* ay);

/* Add a caller-provided centered beeper sample to the AY contribution. */
wz_audio_sample_t wz_audio_mixer_sample(wz_audio_sample_t beeper_sample,
                                        const wz_ay_t* ay);

#endif
