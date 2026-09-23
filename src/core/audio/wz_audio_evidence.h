/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_AUDIO_WZ_AUDIO_EVIDENCE_H
#define WZ_CORE_AUDIO_WZ_AUDIO_EVIDENCE_H

#include <stddef.h>

#include "core/audio/wz_audio_policy.h"
#include "core/wz_types.h"

wz_result_t wz_audio_samples_hash(const wz_audio_sample_t* samples,
                                  size_t count,
                                  wz_qword_t* hash);

#endif
