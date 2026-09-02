/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/audio/wz_audio_evidence.h"

#define WZ_AUDIO_EVIDENCE_FNV_OFFSET UINT64_C(14695981039346656037)
#define WZ_AUDIO_EVIDENCE_FNV_PRIME UINT64_C(1099511628211)

static void wz_audio_hash_byte(wz_qword_t* hash, wz_byte_t value)
{
    *hash ^= value;
    *hash *= WZ_AUDIO_EVIDENCE_FNV_PRIME;
}

static void wz_audio_hash_u64(wz_qword_t* hash, wz_qword_t value)
{
    for (unsigned index = 0u; index < 8u; ++index) {
        wz_audio_hash_byte(hash, (wz_byte_t)(value >> (index * 8u)));
    }
}

wz_result_t wz_audio_samples_hash(const wz_audio_sample_t* samples,
                                  size_t count,
                                  wz_qword_t* hash)
{
    if (hash == 0 || (samples == 0 && count != 0u)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *hash = WZ_AUDIO_EVIDENCE_FNV_OFFSET;
    wz_audio_hash_u64(hash, (wz_qword_t)count);
    for (size_t index = 0u; index < count; ++index) {
        wz_audio_hash_u64(hash, (wz_qword_t)(int64_t)samples[index]);
    }
    return WZ_RESULT_OK;
}
