/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_KEMPSTON_H
#define WZ_CORE_WZ_KEMPSTON_H

#include <stdbool.h>

#include "core/wz_types.h"

#define WZ_KEMPSTON_PORT 0x1fu

typedef enum {
    WZ_KEMPSTON_RIGHT = 0u,
    WZ_KEMPSTON_LEFT,
    WZ_KEMPSTON_DOWN,
    WZ_KEMPSTON_UP,
    WZ_KEMPSTON_FIRE,
    WZ_KEMPSTON_CONTROL_COUNT
} wz_kempston_control_t;

typedef struct {
    bool pressed[WZ_KEMPSTON_CONTROL_COUNT];
} wz_kempston_t;

void wz_kempston_init(wz_kempston_t* joystick);
bool wz_kempston_set(wz_kempston_t* joystick,
                     wz_kempston_control_t control, bool pressed);
bool wz_kempston_port_selected(wz_word_t port);
wz_byte_t wz_kempston_read(const wz_kempston_t* joystick, wz_word_t port);

#endif
