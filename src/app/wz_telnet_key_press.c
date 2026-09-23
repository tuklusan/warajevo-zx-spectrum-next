/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_telnet_key_press.h"

void wz_telnet_key_press_state_init(wz_telnet_key_press_state_t* state)
{
    if (state == 0) return;
    for (size_t key = 0u; key < WZ_INPUT_ARBITER_KEY_COUNT; ++key) {
        state->release_tick[key] = 0u;
        state->pending[key] = 0u;
    }
}

bool wz_telnet_key_press_schedule(wz_telnet_key_press_state_t* state,
                                  wz_input_arbiter_t* arbiter,
                                  size_t key,
                                  wz_master_tick_t start_tick,
                                  wz_master_tick_t frame_ticks)
{
    if (state == 0 || arbiter == 0 || key >= WZ_INPUT_ARBITER_KEY_COUNT ||
        frame_ticks == 0u || state->pending[key] != 0u ||
        wz_input_arbiter_source_key_down(arbiter, WZ_INPUT_SOURCE_TELNET, key)) {
        return false;
    }
    if (frame_ticks > (UINT64_MAX - start_tick) / 2u) return false;
    if (!wz_input_arbiter_set(arbiter, WZ_INPUT_SOURCE_TELNET, key, true)) {
        return false;
    }
    state->release_tick[key] = start_tick + (2u * frame_ticks);
    state->pending[key] = 1u;
    return true;
}

size_t wz_telnet_key_press_drain(wz_telnet_key_press_state_t* state,
                                 wz_input_arbiter_t* arbiter,
                                 wz_master_tick_t current_tick)
{
    size_t released = 0u;
    if (state == 0 || arbiter == 0) return 0u;
    for (size_t key = 0u; key < WZ_INPUT_ARBITER_KEY_COUNT; ++key) {
        if (state->pending[key] != 0u &&
            current_tick >= state->release_tick[key]) {
            if (wz_input_arbiter_set(arbiter, WZ_INPUT_SOURCE_TELNET,
                                     key, false)) {
                ++released;
            }
            state->pending[key] = 0u;
        }
    }
    return released;
}

bool wz_telnet_key_press_pending(const wz_telnet_key_press_state_t* state,
                                 size_t key)
{
    return state != 0 && key < WZ_INPUT_ARBITER_KEY_COUNT &&
           state->pending[key] != 0u;
}
