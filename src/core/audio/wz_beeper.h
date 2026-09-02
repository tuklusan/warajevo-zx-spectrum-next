/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_AUDIO_WZ_BEEPER_H
#define WZ_CORE_AUDIO_WZ_BEEPER_H

#include <stddef.h>

#include "core/wz_types.h"

#define WZ_BEEPER_EVENT_CAPACITY 1024u

typedef struct {
    wz_master_tick_t master_tick;
    wz_byte_t level;
} wz_beeper_event_t;

typedef struct {
    wz_byte_t level;
    wz_byte_t mic_level;
    wz_master_tick_t level_tick;
    wz_beeper_event_t events[WZ_BEEPER_EVENT_CAPACITY];
    size_t event_count;
} wz_beeper_t;

void wz_beeper_init(wz_beeper_t* beeper);
void wz_beeper_port_fe_write(wz_beeper_t* beeper,
                             wz_byte_t value,
                             wz_master_tick_t master_tick);
wz_byte_t wz_beeper_level(const wz_beeper_t* beeper);
wz_byte_t wz_beeper_mic_level(const wz_beeper_t* beeper);
size_t wz_beeper_events(const wz_beeper_t* beeper,
                        wz_beeper_event_t* events,
                        size_t capacity);

#endif
