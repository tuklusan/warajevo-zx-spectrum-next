/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_keyboard_mapping.h"

void wz_keyboard_mapping_init(wz_keyboard_mapping_t* mapping)
{
    if (mapping == NULL) {
        return;
    }
    *mapping = (wz_keyboard_mapping_t){0};
}

bool wz_keyboard_mapping_bind(wz_keyboard_mapping_t* mapping,
                              unsigned host_key,
                              wz_keyboard_key_t spectrum_key)
{
    size_t free_index = WZ_KEYBOARD_MAPPING_BINDING_COUNT;
    if (mapping == NULL || host_key == 0u ||
        host_key >= WZ_KEYBOARD_MAPPING_HOST_KEY_LIMIT ||
        (size_t)spectrum_key >= WZ_KEYBOARD_MATRIX_KEY_COUNT) {
        return false;
    }
    for (size_t index = 0u; index < WZ_KEYBOARD_MAPPING_BINDING_COUNT; ++index) {
        if (mapping->bindings[index].bound &&
            mapping->bindings[index].host_key == host_key) {
            mapping->bindings[index].spectrum_key = spectrum_key;
            return true;
        }
        if (!mapping->bindings[index].bound &&
            free_index == WZ_KEYBOARD_MAPPING_BINDING_COUNT) {
            free_index = index;
        }
    }
    if (free_index == WZ_KEYBOARD_MAPPING_BINDING_COUNT) {
        return false;
    }
    mapping->bindings[free_index] = (wz_keyboard_binding_t){
        host_key, spectrum_key, true
    };
    return true;
}

bool wz_keyboard_mapping_unbind(wz_keyboard_mapping_t* mapping,
                                wz_keyboard_key_t spectrum_key)
{
    bool removed = false;
    if (mapping == NULL || (size_t)spectrum_key >= WZ_KEYBOARD_MATRIX_KEY_COUNT) {
        return false;
    }
    for (size_t index = 0u; index < WZ_KEYBOARD_MAPPING_BINDING_COUNT; ++index) {
        if (mapping->bindings[index].bound &&
            mapping->bindings[index].spectrum_key == spectrum_key) {
            mapping->bindings[index].bound = false;
            removed = true;
        }
    }
    return removed;
}

bool wz_keyboard_mapping_resolve(const wz_keyboard_mapping_t* mapping,
                                 unsigned host_key,
                                 wz_keyboard_key_t* spectrum_key)
{
    if (mapping == NULL || spectrum_key == NULL ||
        host_key == 0u || host_key >= WZ_KEYBOARD_MAPPING_HOST_KEY_LIMIT) {
        return false;
    }
    for (size_t index = 0u; index < WZ_KEYBOARD_MAPPING_BINDING_COUNT; ++index) {
        if (mapping->bindings[index].bound &&
            mapping->bindings[index].host_key == host_key) {
            if ((size_t)mapping->bindings[index].spectrum_key >=
                WZ_KEYBOARD_MATRIX_KEY_COUNT) {
                return false;
            }
            *spectrum_key = mapping->bindings[index].spectrum_key;
            return true;
        }
    }
    return false;
}

bool wz_keyboard_mapping_host_for(const wz_keyboard_mapping_t* mapping,
                                  wz_keyboard_key_t spectrum_key,
                                  unsigned* host_key)
{
    if (mapping == NULL || host_key == NULL ||
        (size_t)spectrum_key >= WZ_KEYBOARD_MATRIX_KEY_COUNT) {
        return false;
    }
    for (size_t index = 0u; index < WZ_KEYBOARD_MAPPING_BINDING_COUNT; ++index) {
        if (mapping->bindings[index].bound &&
            mapping->bindings[index].spectrum_key == spectrum_key) {
            *host_key = mapping->bindings[index].host_key;
            return true;
        }
    }
    return false;
}
