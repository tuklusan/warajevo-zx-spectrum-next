/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/audio/wz_audio_mixer.h"

#include "core/audio/wz_ay_mixer_policy.h"

static wz_audio_sample_t wz_audio_mixer_saturate(wz_audio_accumulator_t value)
{
    if (value < (wz_audio_accumulator_t)WZ_AUDIO_MIXER_MIN) {
        return WZ_AUDIO_MIXER_MIN;
    }
    if (value > (wz_audio_accumulator_t)WZ_AUDIO_MIXER_MAX) {
        return WZ_AUDIO_MIXER_MAX;
    }
    return (wz_audio_sample_t)value;
}

static wz_byte_t wz_audio_mixer_channel_level(const wz_ay_t* ay,
                                              wz_byte_t channel)
{
    wz_byte_t volume = wz_ay_register_value(ay, (wz_byte_t)(8u + channel));

    if ((volume & 0x10u) != 0u) {
        return (wz_byte_t)(wz_ay_envelope_level(ay) & 0x0fu);
    }
    return (wz_byte_t)(volume & 0x0fu);
}

static wz_byte_t wz_audio_mixer_channel_output(const wz_ay_t* ay,
                                               wz_byte_t channel)
{
    wz_byte_t mixer = wz_ay_register_value(ay, 7u);
    wz_byte_t tone = wz_ay_tone_level(ay, channel);
    wz_byte_t noise = wz_ay_noise_level(ay);
    wz_byte_t tone_disabled = (wz_byte_t)((mixer >> channel) & 1u);
    wz_byte_t noise_disabled = (wz_byte_t)((mixer >> (channel + 3u)) & 1u);

    /* A disabled AY source is logically high before the AND combination. */
    tone = (wz_byte_t)(tone | tone_disabled);
    noise = (wz_byte_t)(noise | noise_disabled);
    return (wz_byte_t)(tone & noise);
}

wz_audio_sample_t wz_audio_mixer_ay_sample(const wz_ay_t* ay)
{
    wz_audio_accumulator_t total = 0;

    if (ay == 0) {
        return 0;
    }
    for (wz_byte_t channel = 0u; channel < WZ_AY_CHANNEL_COUNT; ++channel) {
        wz_audio_accumulator_t gain =
            (wz_audio_accumulator_t)WZ_AY_VOLUME_GAIN_Q16_16[
                wz_audio_mixer_channel_level(ay, channel)];
        total += wz_audio_mixer_channel_output(ay, channel) != 0u ? gain : -gain;
    }
    return wz_audio_mixer_saturate(total);
}

wz_audio_sample_t wz_audio_mixer_sample(wz_audio_sample_t beeper_sample,
                                        const wz_ay_t* ay)
{
    return wz_audio_mixer_saturate((wz_audio_accumulator_t)beeper_sample +
                                   (wz_audio_accumulator_t)wz_audio_mixer_ay_sample(ay));
}
