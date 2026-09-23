/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_TELNET_KEY_PRESS_H
#define WZ_APP_WZ_TELNET_KEY_PRESS_H

#include <stdbool.h>
#include <stddef.h>

#include "app/wz_input_arbiter.h"
#include "core/wz_types.h"

typedef struct {
    wz_master_tick_t release_tick[WZ_INPUT_ARBITER_KEY_COUNT];
    unsigned char pending[WZ_INPUT_ARBITER_KEY_COUNT];
} wz_telnet_key_press_state_t;

void wz_telnet_key_press_state_init(wz_telnet_key_press_state_t* state);
bool wz_telnet_key_press_schedule(wz_telnet_key_press_state_t* state,
                                  wz_input_arbiter_t* arbiter,
                                  size_t key,
                                  wz_master_tick_t start_tick,
                                  wz_master_tick_t frame_ticks);
size_t wz_telnet_key_press_drain(wz_telnet_key_press_state_t* state,
                                 wz_input_arbiter_t* arbiter,
                                 wz_master_tick_t current_tick);
bool wz_telnet_key_press_pending(const wz_telnet_key_press_state_t* state,
                                 size_t key);

#endif
