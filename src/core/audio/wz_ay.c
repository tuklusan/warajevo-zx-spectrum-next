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

void wz_ay_init(wz_ay_t* ay)
{
    if (ay == 0) {
        return;
    }
    memset(ay, 0, sizeof(*ay));
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
