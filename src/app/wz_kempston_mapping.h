/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_KEMPSTON_MAPPING_H
#define WZ_APP_WZ_KEMPSTON_MAPPING_H

#include <stdbool.h>
#include <stddef.h>

#include "core/wz_kempston.h"

#define WZ_KEMPSTON_MAPPING_CONTROL_COUNT WZ_KEMPSTON_CONTROL_COUNT

typedef struct {
    unsigned host_control;
    wz_kempston_control_t control;
    bool bound;
} wz_kempston_binding_t;

typedef struct {
    wz_kempston_binding_t bindings[WZ_KEMPSTON_MAPPING_CONTROL_COUNT];
} wz_kempston_mapping_t;

void wz_kempston_mapping_init(wz_kempston_mapping_t* mapping,
                              const unsigned* default_host_controls);
bool wz_kempston_mapping_bind(wz_kempston_mapping_t* mapping,
                               wz_kempston_control_t control,
                               unsigned host_control);
bool wz_kempston_mapping_unbind(wz_kempston_mapping_t* mapping,
                                wz_kempston_control_t control);
bool wz_kempston_mapping_apply(const wz_kempston_mapping_t* mapping,
                               wz_kempston_t* joystick,
                               unsigned host_control, bool pressed);

#endif
