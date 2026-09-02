/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_sokol_audio.h"

#include "app/wz_host_audio_policy.h"
#include "app/wz_host_audio_push.h"
#include "sokol_audio.h"

#define WZ_SOKOL_AUDIO_PACKET_FRAMES 512u

bool wz_sokol_audio_init(wz_sokol_audio_t* audio)
{
    saudio_desc description;

    if (audio == 0) {
        return false;
    }
    audio->initialized = false;
    description = (saudio_desc){
        .sample_rate = WZ_CANONICAL_AUDIO_SAMPLE_RATE,
        .num_channels = 1,
        .buffer_frames = WZ_HOST_AUDIO_QUEUE_CAPACITY,
        .packet_frames = WZ_SOKOL_AUDIO_PACKET_FRAMES,
    };
    saudio_setup(&description);
    audio->initialized = saudio_isvalid();
    return audio->initialized;
}

void wz_sokol_audio_shutdown(wz_sokol_audio_t* audio)
{
    if (audio != 0 && audio->initialized) {
        saudio_shutdown();
        audio->initialized = false;
    }
}

bool wz_sokol_audio_valid(const wz_sokol_audio_t* audio)
{
    return audio != 0 && audio->initialized && saudio_isvalid();
}

size_t wz_sokol_audio_push(wz_sokol_audio_t* audio,
                           wz_speed_policy_t speed,
                           const wz_audio_sample_t* samples,
                           size_t count)
{
    float packet[WZ_SOKOL_AUDIO_PACKET_FRAMES];
    size_t submitted = 0u;

    if (!wz_sokol_audio_valid(audio) || !wz_host_audio_enabled(speed) ||
        (samples == 0 && count != 0u)) {
        return 0u;
    }
    while (submitted < count) {
        size_t packet_count = count - submitted;
        int accepted;
        if (packet_count > WZ_SOKOL_AUDIO_PACKET_FRAMES) {
            packet_count = WZ_SOKOL_AUDIO_PACKET_FRAMES;
        }
        for (size_t index = 0u; index < packet_count; ++index) {
            packet[index] = (float)samples[submitted + index] /
                            (float)WZ_AUDIO_MIXER_ONE;
        }
        accepted = saudio_push(packet, (int)packet_count);
        if (accepted <= 0) {
            break;
        }
        submitted += (size_t)accepted;
        if ((size_t)accepted < packet_count) {
            break;
        }
    }
    return submitted;
}
