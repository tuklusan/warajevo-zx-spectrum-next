/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/audio/wz_beeper.h"

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

    if (beeper == 0) {
        return;
    }
    level = (wz_byte_t)((value >> 3u) & 1u);
    beeper->mic_level = (wz_byte_t)((value >> 4u) & 1u);
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
