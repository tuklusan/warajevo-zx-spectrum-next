/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_input_arbiter.h"

static bool wz_input_source_valid(wz_input_source_t source)
{
    return (size_t)source < WZ_INPUT_ARBITER_SOURCE_COUNT;
}

void wz_input_arbiter_init(wz_input_arbiter_t* arbiter)
{
    if (arbiter == 0) {
        return;
    }
    for (size_t source = 0u; source < WZ_INPUT_ARBITER_SOURCE_COUNT; ++source) {
        for (size_t key = 0u; key < WZ_INPUT_ARBITER_KEY_COUNT; ++key) {
            arbiter->owned[source][key] = 0u;
        }
    }
}

bool wz_input_arbiter_set(wz_input_arbiter_t* arbiter,
                          wz_input_source_t source, size_t key, bool pressed)
{
    if (arbiter == 0 || !wz_input_source_valid(source) ||
        key >= WZ_INPUT_ARBITER_KEY_COUNT) {
        return false;
    }
    arbiter->owned[source][key] = pressed ? 1u : 0u;
    return true;
}

bool wz_input_arbiter_key_down(const wz_input_arbiter_t* arbiter, size_t key)
{
    if (arbiter == 0 || key >= WZ_INPUT_ARBITER_KEY_COUNT) {
        return false;
    }
    for (size_t source = 0u; source < WZ_INPUT_ARBITER_SOURCE_COUNT; ++source) {
        if (arbiter->owned[source][key] != 0u) {
            return true;
        }
    }
    return false;
}

bool wz_input_arbiter_release_source(wz_input_arbiter_t* arbiter,
                                     wz_input_source_t source)
{
    if (arbiter == 0 || !wz_input_source_valid(source)) {
        return false;
    }
    for (size_t key = 0u; key < WZ_INPUT_ARBITER_KEY_COUNT; ++key) {
        arbiter->owned[source][key] = 0u;
    }
    return true;
}
