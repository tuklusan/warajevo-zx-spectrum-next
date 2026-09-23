/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_AUDIO_WZ_AY_H
#define WZ_CORE_AUDIO_WZ_AY_H

#include <stddef.h>

#include "core/wz_types.h"

#define WZ_AY_REGISTER_COUNT 16u
#define WZ_AY_EVENT_CAPACITY 1024u
#define WZ_AY_CHANNEL_COUNT 3u
#define WZ_AY_MASTER_TICKS_PER_CLOCK 4u
#define WZ_AY_ENVELOPE_LEVEL_COUNT 16u

typedef enum {
    WZ_AY_EVENT_REGISTER_SELECT = 0,
    WZ_AY_EVENT_REGISTER_WRITE
} wz_ay_event_kind_t;

typedef struct {
    wz_ay_event_kind_t kind;
    wz_master_tick_t master_tick;
    wz_byte_t register_index;
    wz_byte_t value;
} wz_ay_event_t;

typedef struct {
    wz_byte_t selected_register;
    wz_byte_t registers[WZ_AY_REGISTER_COUNT];
    wz_word_t tone_counters[WZ_AY_CHANNEL_COUNT];
    wz_byte_t tone_levels[WZ_AY_CHANNEL_COUNT];
    wz_byte_t noise_counter;
    wz_dword_t noise_lfsr;
    wz_byte_t noise_level;
    wz_word_t envelope_counter;
    wz_word_t envelope_period;
    wz_byte_t envelope_level;
    wz_byte_t envelope_attack;
    wz_byte_t envelope_holding;
    wz_byte_t tone_master_tick_phase;
    wz_ay_event_t events[WZ_AY_EVENT_CAPACITY];
    size_t event_count;
} wz_ay_t;

void wz_ay_init(wz_ay_t* ay);
wz_result_t wz_ay_select_register(wz_ay_t* ay, wz_byte_t value,
                                  wz_master_tick_t master_tick);
wz_result_t wz_ay_write_data(wz_ay_t* ay, wz_byte_t value,
                             wz_master_tick_t master_tick);
wz_byte_t wz_ay_selected_register(const wz_ay_t* ay);
wz_byte_t wz_ay_register_value(const wz_ay_t* ay, wz_byte_t register_index);
size_t wz_ay_events(const wz_ay_t* ay, wz_ay_event_t* events, size_t capacity);
wz_result_t wz_ay_advance_master_ticks(wz_ay_t* ay, wz_master_tick_t ticks);
wz_word_t wz_ay_tone_period(const wz_ay_t* ay, wz_byte_t channel);
wz_byte_t wz_ay_tone_level(const wz_ay_t* ay, wz_byte_t channel);
wz_byte_t wz_ay_noise_period(const wz_ay_t* ay);
wz_byte_t wz_ay_noise_level(const wz_ay_t* ay);
wz_word_t wz_ay_envelope_period(const wz_ay_t* ay);
wz_byte_t wz_ay_envelope_level(const wz_ay_t* ay);

#endif
