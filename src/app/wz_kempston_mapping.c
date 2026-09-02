/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_kempston_mapping.h"

void wz_kempston_mapping_init(wz_kempston_mapping_t* mapping,
                              const unsigned* default_host_controls)
{
    if (mapping == 0) {
        return;
    }
    for (size_t index = 0u; index < WZ_KEMPSTON_MAPPING_CONTROL_COUNT; ++index) {
        mapping->bindings[index].control = (wz_kempston_control_t)index;
        mapping->bindings[index].host_control =
            default_host_controls == 0 ? 0u : default_host_controls[index];
        mapping->bindings[index].bound = default_host_controls != 0;
    }
}

bool wz_kempston_mapping_bind(wz_kempston_mapping_t* mapping,
                               wz_kempston_control_t control,
                               unsigned host_control)
{
    if (mapping == 0 || (size_t)control >= WZ_KEMPSTON_MAPPING_CONTROL_COUNT) {
        return false;
    }
    mapping->bindings[control].host_control = host_control;
    mapping->bindings[control].bound = true;
    return true;
}

bool wz_kempston_mapping_unbind(wz_kempston_mapping_t* mapping,
                                wz_kempston_control_t control)
{
    if (mapping == 0 || (size_t)control >= WZ_KEMPSTON_MAPPING_CONTROL_COUNT) {
        return false;
    }
    mapping->bindings[control].bound = false;
    return true;
}

bool wz_kempston_mapping_apply(const wz_kempston_mapping_t* mapping,
                               wz_kempston_t* joystick,
                               unsigned host_control, bool pressed)
{
    bool applied = false;

    if (mapping == 0 || joystick == 0) {
        return false;
    }
    for (size_t index = 0u; index < WZ_KEMPSTON_MAPPING_CONTROL_COUNT; ++index) {
        if (mapping->bindings[index].bound &&
            mapping->bindings[index].host_control == host_control) {
            if (!wz_kempston_set(joystick, mapping->bindings[index].control, pressed)) {
                return false;
            }
            applied = true;
        }
    }
    return applied;
}
