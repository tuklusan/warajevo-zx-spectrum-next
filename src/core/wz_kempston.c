/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_kempston.h"

#include <stddef.h>

static const wz_byte_t wz_kempston_masks[WZ_KEMPSTON_CONTROL_COUNT] = {
    0x10u, 0x08u, 0x02u, 0x01u, 0x04u
};

void wz_kempston_init(wz_kempston_t* joystick)
{
    if (joystick != 0) {
        for (size_t index = 0u; index < WZ_KEMPSTON_CONTROL_COUNT; ++index) {
            joystick->pressed[index] = false;
        }
    }
}

bool wz_kempston_set(wz_kempston_t* joystick,
                     wz_kempston_control_t control, bool pressed)
{
    if (joystick == 0 || (size_t)control >= WZ_KEMPSTON_CONTROL_COUNT) {
        return false;
    }
    joystick->pressed[control] = pressed;
    return true;
}

bool wz_kempston_port_selected(wz_word_t port)
{
    return (port & 0xffu) == WZ_KEMPSTON_PORT;
}

wz_byte_t wz_kempston_read(const wz_kempston_t* joystick, wz_word_t port)
{
    wz_byte_t value = 0u;

    if (joystick == 0 || !wz_kempston_port_selected(port)) {
        return value;
    }
    for (size_t index = 0u; index < WZ_KEMPSTON_CONTROL_COUNT; ++index) {
        if (joystick->pressed[index]) {
            value |= wz_kempston_masks[index];
        }
    }
    return value;
}
