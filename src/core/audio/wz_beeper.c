/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/audio/wz_beeper.h"

#include <stdint.h>
#include <string.h>

void wz_beeper_init(wz_beeper_t* beeper)
{
    if (beeper == 0) {
        return;
    }
    memset(beeper, 0, sizeof(*beeper));
}

void wz_beeper_port_fe_write(wz_beeper_t* beeper,
                             wz_byte_t value,
                             wz_master_tick_t master_tick)
{
    wz_byte_t level;
    wz_byte_t mic_level;

    if (beeper == 0) {
        return;
    }
    level = (wz_byte_t)((value >> 3u) & 1u);
    mic_level = (wz_byte_t)((value >> 4u) & 1u);
    if (mic_level != beeper->mic_level && beeper->mic_capture_active != 0u) {
        if (beeper->mic_event_count < WZ_MIC_EVENT_CAPACITY) {
            beeper->mic_events[beeper->mic_event_count].master_tick = master_tick;
            beeper->mic_events[beeper->mic_event_count].level = mic_level;
            beeper->mic_event_count += 1u;
        } else {
            beeper->mic_capture_overflow = 1u;
        }
    }
    beeper->mic_level = mic_level;
    if (level == beeper->level) {
        return;
    }
    beeper->level = level;
    beeper->level_tick = master_tick;
    if (beeper->event_count < WZ_BEEPER_EVENT_CAPACITY) {
        beeper->events[beeper->event_count].master_tick = master_tick;
        beeper->events[beeper->event_count].level = level;
        beeper->event_count += 1u;
    }
}

wz_byte_t wz_beeper_level(const wz_beeper_t* beeper)
{
    return beeper == 0 ? 0u : beeper->level;
}

wz_byte_t wz_beeper_mic_level(const wz_beeper_t* beeper)
{
    return beeper == 0 ? 0u : beeper->mic_level;
}

size_t wz_beeper_events(const wz_beeper_t* beeper,
                        wz_beeper_event_t* events,
                        size_t capacity)
{
    size_t count;

    if (beeper == 0 || (events == 0 && capacity != 0u)) {
        return 0u;
    }
    count = beeper->event_count < capacity ? beeper->event_count : capacity;
    if (count > 0u) {
        memcpy(events, beeper->events, count * sizeof(*events));
    }
    return count;
}

wz_result_t wz_beeper_mic_capture_begin(wz_beeper_t* beeper)
{
    if (beeper == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    beeper->mic_event_count = 0u;
    beeper->mic_capture_overflow = 0u;
    beeper->mic_capture_active = 1u;
    return WZ_RESULT_OK;
}

wz_result_t wz_beeper_mic_capture_end(wz_beeper_t* beeper)
{
    if (beeper == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    beeper->mic_capture_active = 0u;
    return WZ_RESULT_OK;
}

size_t wz_beeper_mic_events(const wz_beeper_t* beeper,
                            wz_mic_event_t* events,
                            size_t capacity)
{
    size_t count;

    if (beeper == 0 || (events == 0 && capacity != 0u)) {
        return 0u;
    }
    count = beeper->mic_event_count < capacity ? beeper->mic_event_count : capacity;
    if (count > 0u) {
        memcpy(events, beeper->mic_events, count * sizeof(*events));
    }
    return count;
}

bool wz_beeper_mic_capture_overflowed(const wz_beeper_t* beeper)
{
    return beeper != 0 && beeper->mic_capture_overflow != 0u;
}

static wz_audio_sample_t wz_beeper_average(wz_qword_t high_ticks,
                                            wz_qword_t low_ticks,
                                            wz_qword_t duration)
{
    wz_audio_accumulator_t delta = (wz_audio_accumulator_t)high_ticks -
                                    (wz_audio_accumulator_t)low_ticks;
    wz_audio_accumulator_t magnitude = delta < 0 ? -delta : delta;
    wz_audio_accumulator_t scaled;

    if (magnitude > INT64_MAX / (wz_audio_accumulator_t)WZ_AUDIO_MIXER_ONE) {
        return delta < 0 ? WZ_AUDIO_MIXER_MIN : WZ_AUDIO_MIXER_MAX;
    }
    scaled = (magnitude * (wz_audio_accumulator_t)WZ_AUDIO_MIXER_ONE +
              (wz_audio_accumulator_t)(duration / 2u)) /
             (wz_audio_accumulator_t)duration;
    if (delta < 0) {
        scaled = -scaled;
    }
    if (scaled < WZ_AUDIO_MIXER_MIN) {
        return WZ_AUDIO_MIXER_MIN;
    }
    if (scaled > WZ_AUDIO_MIXER_MAX) {
        return WZ_AUDIO_MIXER_MAX;
    }
    return (wz_audio_sample_t)scaled;
}

bool wz_beeper_render_pcm(const wz_beeper_event_t* events,
                          size_t event_count,
                          wz_byte_t initial_level,
                          wz_master_tick_t start_tick,
                          wz_qword_t master_ticks_per_second,
                          wz_qword_t sample_rate,
                          wz_audio_sample_t* samples,
                          size_t sample_count)
{
    wz_qword_t whole_ticks;
    wz_qword_t remainder_ticks;
    wz_qword_t remainder_accumulator = 0u;
    wz_master_tick_t cursor = start_tick;
    size_t event_index = 0u;
    wz_byte_t level = initial_level;

    if ((events == 0 && event_count != 0u) || initial_level > 1u ||
        (samples == 0 && sample_count != 0u) || master_ticks_per_second == 0u ||
        sample_rate == 0u || sample_rate > master_ticks_per_second) {
        return false;
    }
    for (size_t index = 0u; index < event_count; ++index) {
        if (events[index].level > 1u ||
            (index > 0u && events[index].master_tick < events[index - 1u].master_tick)) {
            return false;
        }
    }
    whole_ticks = master_ticks_per_second / sample_rate;
    remainder_ticks = master_ticks_per_second % sample_rate;
    while (event_index < event_count && events[event_index].master_tick <= cursor) {
        if (events[event_index].level > 1u) {
            return false;
        }
        level = events[event_index].level;
        event_index += 1u;
    }
    for (size_t sample_index = 0u; sample_index < sample_count; ++sample_index) {
        wz_qword_t duration = whole_ticks;
        wz_qword_t high_ticks = 0u;
        wz_qword_t low_ticks = 0u;
        wz_master_tick_t end_tick;
        wz_master_tick_t segment_start;

        if (remainder_accumulator >= sample_rate - remainder_ticks) {
            if (duration == UINT64_MAX) {
                return false;
            }
            duration += 1u;
            remainder_accumulator -= sample_rate - remainder_ticks;
        } else {
            remainder_accumulator += remainder_ticks;
        }
        if (duration == 0u || UINT64_MAX - cursor < duration ||
            duration > (wz_qword_t)INT64_MAX) {
            return false;
        }
        end_tick = cursor + duration;
        segment_start = cursor;
        while (event_index < event_count && events[event_index].master_tick < end_tick) {
            wz_master_tick_t edge = events[event_index].master_tick;
            wz_qword_t segment = edge - segment_start;
            if (level != 0u) {
                high_ticks += segment;
            } else {
                low_ticks += segment;
            }
            if (events[event_index].level > 1u) {
                return false;
            }
            level = events[event_index].level;
            segment_start = edge;
            event_index += 1u;
        }
        if (level != 0u) {
            high_ticks += end_tick - segment_start;
        } else {
            low_ticks += end_tick - segment_start;
        }
        samples[sample_index] = wz_beeper_average(high_ticks, low_ticks, duration);
        cursor = end_tick;
    }
    return true;
}
