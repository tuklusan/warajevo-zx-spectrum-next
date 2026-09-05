/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/audio/wz_ay.h"

#include <string.h>

static void wz_ay_record(wz_ay_t* ay, wz_ay_event_kind_t kind,
                         wz_master_tick_t master_tick, wz_byte_t value)
{
    if (ay->event_count >= WZ_AY_EVENT_CAPACITY) {
        return;
    }
    ay->events[ay->event_count].kind = kind;
    ay->events[ay->event_count].master_tick = master_tick;
    ay->events[ay->event_count].register_index = ay->selected_register;
    ay->events[ay->event_count].value = value;
    ay->event_count += 1u;
}

static wz_word_t wz_ay_period(const wz_ay_t* ay, wz_byte_t channel)
{
    wz_byte_t low_register = (wz_byte_t)(channel * 2u);
    wz_word_t period;

    period = (wz_word_t)ay->registers[low_register];
    period |= (wz_word_t)(ay->registers[(wz_byte_t)(low_register + 1u)] & 0x0fu) << 8u;
    return period == 0u ? 1u : period;
}

static wz_byte_t wz_ay_noise_period_value(const wz_ay_t* ay)
{
    wz_byte_t period = (wz_byte_t)(ay->registers[6u] & 0x1fu);
    return period == 0u ? 1u : period;
}

static void wz_ay_advance_clock(wz_ay_t* ay)
{
    wz_byte_t channel;

    for (channel = 0u; channel < WZ_AY_CHANNEL_COUNT; ++channel) {
        wz_word_t period = wz_ay_period(ay, channel);
        if (ay->tone_counters[channel] == 0u ||
            ay->tone_counters[channel] > period) {
            ay->tone_counters[channel] = period;
        }
        ay->tone_counters[channel] -= 1u;
        if (ay->tone_counters[channel] == 0u) {
            ay->tone_counters[channel] = period;
            ay->tone_levels[channel] ^= 1u;
        }
    }
    {
        wz_byte_t period = wz_ay_noise_period_value(ay);
        if (ay->noise_counter == 0u || ay->noise_counter > period) {
            ay->noise_counter = period;
        }
        ay->noise_counter -= 1u;
        if (ay->noise_counter == 0u) {
            wz_dword_t feedback = (ay->noise_lfsr ^
                (ay->noise_lfsr >> 3u)) & 1u;
            ay->noise_counter = period;
            ay->noise_lfsr = (ay->noise_lfsr >> 1u) | (feedback << 16u);
            ay->noise_level = (wz_byte_t)(ay->noise_lfsr & 1u);
        }
    }
}

void wz_ay_init(wz_ay_t* ay)
{
    if (ay == 0) {
        return;
    }
    memset(ay, 0, sizeof(*ay));
    ay->noise_lfsr = 0x1ffffu;
    ay->noise_level = 1u;
}

wz_result_t wz_ay_select_register(wz_ay_t* ay, wz_byte_t value,
                                  wz_master_tick_t master_tick)
{
    if (ay == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    ay->selected_register = (wz_byte_t)(value & 0x0fu);
    wz_ay_record(ay, WZ_AY_EVENT_REGISTER_SELECT, master_tick, value);
    return WZ_RESULT_OK;
}

wz_result_t wz_ay_write_data(wz_ay_t* ay, wz_byte_t value,
                             wz_master_tick_t master_tick)
{
    if (ay == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    ay->registers[ay->selected_register] = value;
    wz_ay_record(ay, WZ_AY_EVENT_REGISTER_WRITE, master_tick, value);
    return WZ_RESULT_OK;
}

wz_byte_t wz_ay_selected_register(const wz_ay_t* ay)
{
    return ay == 0 ? 0u : ay->selected_register;
}

wz_byte_t wz_ay_register_value(const wz_ay_t* ay, wz_byte_t register_index)
{
    if (ay == 0 || register_index >= WZ_AY_REGISTER_COUNT) {
        return 0u;
    }
    return ay->registers[register_index];
}

size_t wz_ay_events(const wz_ay_t* ay, wz_ay_event_t* events, size_t capacity)
{
    size_t count;

    if (ay == 0 || (events == 0 && capacity != 0u)) {
        return 0u;
    }
    count = ay->event_count < capacity ? ay->event_count : capacity;
    if (events != 0) {
        memcpy(events, ay->events, count * sizeof(*events));
    }
    return count;
}

wz_result_t wz_ay_advance_master_ticks(wz_ay_t* ay, wz_master_tick_t ticks)
{
    if (ay == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    while (ticks != 0u) {
        ++ay->tone_master_tick_phase;
        if (ay->tone_master_tick_phase >= WZ_AY_MASTER_TICKS_PER_CLOCK) {
            ay->tone_master_tick_phase = 0u;
            wz_ay_advance_clock(ay);
        }
        --ticks;
    }
    return WZ_RESULT_OK;
}

wz_word_t wz_ay_tone_period(const wz_ay_t* ay, wz_byte_t channel)
{
    if (ay == 0 || channel >= WZ_AY_CHANNEL_COUNT) {
        return 0u;
    }
    return wz_ay_period(ay, channel);
}

wz_byte_t wz_ay_tone_level(const wz_ay_t* ay, wz_byte_t channel)
{
    if (ay == 0 || channel >= WZ_AY_CHANNEL_COUNT) {
        return 0u;
    }
    return ay->tone_levels[channel];
}

wz_byte_t wz_ay_noise_period(const wz_ay_t* ay)
{
    return ay == 0 ? 0u : wz_ay_noise_period_value(ay);
}

wz_byte_t wz_ay_noise_level(const wz_ay_t* ay)
{
    return ay == 0 ? 0u : ay->noise_level;
}
