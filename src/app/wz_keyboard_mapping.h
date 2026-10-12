/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#ifndef WZ_APP_WZ_KEYBOARD_MAPPING_H
#define WZ_APP_WZ_KEYBOARD_MAPPING_H

#include <stdbool.h>
#include <stddef.h>

#include "core/wz_keyboard_matrix.h"

#define WZ_KEYBOARD_MAPPING_BINDING_COUNT 64u
#define WZ_KEYBOARD_MAPPING_HOST_KEY_LIMIT 512u

typedef struct {
    unsigned host_key;
    wz_keyboard_key_t spectrum_key;
    bool bound;
} wz_keyboard_binding_t;

typedef struct {
    wz_keyboard_binding_t bindings[WZ_KEYBOARD_MAPPING_BINDING_COUNT];
} wz_keyboard_mapping_t;

void wz_keyboard_mapping_init(wz_keyboard_mapping_t* mapping);
bool wz_keyboard_mapping_bind(wz_keyboard_mapping_t* mapping,
                              unsigned host_key,
                              wz_keyboard_key_t spectrum_key);
bool wz_keyboard_mapping_unbind(wz_keyboard_mapping_t* mapping,
                                wz_keyboard_key_t spectrum_key);
bool wz_keyboard_mapping_resolve(const wz_keyboard_mapping_t* mapping,
                                 unsigned host_key,
                                 wz_keyboard_key_t* spectrum_key);
bool wz_keyboard_mapping_host_for(const wz_keyboard_mapping_t* mapping,
                                  wz_keyboard_key_t spectrum_key,
                                  unsigned* host_key);

#endif
