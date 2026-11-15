/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_input_arbiter.h"
#include "app/wz_keyboard_mapping.h"
#include "app/wz_kempston_mapping.h"
#include "core/wz_kempston.h"

#include <stdint.h>
#include <string.h>

bool wz_input_event_set_local_key(
    const wz_keyboard_mapping_t* keyboard_mapping,
    wz_kempston_mapping_t* kempston_mapping,
    wz_kempston_t* kempston,
    wz_input_arbiter_t* arbiter,
    unsigned char* host_key_down,
    uint16_t* spectrum_key_refcount,
    unsigned host_key,
    bool pressed)
{
    wz_keyboard_key_t spectrum_key;
    size_t physical_key = 0u;
    bool keyboard_mapped;
    bool joystick_mapped;

    if (keyboard_mapping == NULL || kempston_mapping == NULL ||
        kempston == NULL || arbiter == NULL || host_key_down == NULL ||
        spectrum_key_refcount == NULL || host_key == 0u ||
        host_key >= WZ_KEYBOARD_MAPPING_HOST_KEY_LIMIT) {
        return false;
    }
    keyboard_mapped = wz_keyboard_mapping_resolve(keyboard_mapping,
                                                   host_key, &spectrum_key);
    if (keyboard_mapped) {
        physical_key = (size_t)spectrum_key;
        if (physical_key >= WZ_INPUT_ARBITER_KEY_COUNT) {
            keyboard_mapped = false;
        }
    }
    if (host_key_down[host_key] == (unsigned char)(pressed ? 1u : 0u)) {
        return keyboard_mapped || wz_kempston_mapping_apply(
            kempston_mapping, kempston, host_key, pressed);
    }
    if (keyboard_mapped) {
        if (pressed) {
            if (spectrum_key_refcount[physical_key] != UINT16_MAX) {
                ++spectrum_key_refcount[physical_key];
            }
        } else if (spectrum_key_refcount[physical_key] > 0u) {
            --spectrum_key_refcount[physical_key];
        }
        if (!wz_input_arbiter_set(arbiter, WZ_INPUT_SOURCE_LOCAL,
                                  physical_key,
                                  spectrum_key_refcount[physical_key] > 0u)) {
            return false;
        }
    }
    host_key_down[host_key] = pressed ? 1u : 0u;
    joystick_mapped = wz_kempston_mapping_apply(kempston_mapping, kempston,
                                                host_key, pressed);
    return keyboard_mapped || joystick_mapped;
}

bool wz_input_event_release_local_keys(
    wz_kempston_mapping_t* kempston_mapping,
    wz_kempston_t* kempston,
    wz_input_arbiter_t* arbiter,
    unsigned char* host_key_down,
    uint16_t* spectrum_key_refcount)
{
    if (kempston_mapping == NULL || kempston == NULL || arbiter == NULL ||
        host_key_down == NULL || spectrum_key_refcount == NULL) {
        return false;
    }
    for (unsigned host_key = 0u;
         host_key < WZ_KEYBOARD_MAPPING_HOST_KEY_LIMIT; ++host_key) {
        if (host_key_down[host_key] != 0u) {
            (void)wz_kempston_mapping_apply(kempston_mapping, kempston,
                                            host_key, false);
        }
        host_key_down[host_key] = 0u;
    }
    memset(spectrum_key_refcount, 0,
           sizeof(uint16_t) * WZ_INPUT_ARBITER_KEY_COUNT);
    return wz_input_arbiter_release_source(arbiter, WZ_INPUT_SOURCE_LOCAL);
}
