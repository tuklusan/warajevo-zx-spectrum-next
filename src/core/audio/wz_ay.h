/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
patent, trademark, and governing-law provisions.
*/

#ifndef WZ_CORE_AUDIO_WZ_AY_H
#define WZ_CORE_AUDIO_WZ_AY_H

#include <stddef.h>

#include "core/wz_types.h"

#define WZ_AY_REGISTER_COUNT 16u
#define WZ_AY_EVENT_CAPACITY 1024u
#define WZ_AY_CHANNEL_COUNT 3u
#define WZ_AY_MASTER_TICKS_PER_CLOCK 4u
/* AY input-clock prescalers for tone edges, noise shifts, and envelope steps. */
#define WZ_AY_TONE_INPUT_CLOCK_DIVIDER 8u
#define WZ_AY_NOISE_INPUT_CLOCK_DIVIDER 16u
#define WZ_AY_ENVELOPE_INPUT_CLOCK_DIVIDER 256u
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
    wz_byte_t tone_input_clock_phase;
    wz_byte_t noise_input_clock_phase;
    wz_byte_t envelope_input_clock_phase;
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
