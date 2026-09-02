/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_INPUT_TYPES_H
#define WZ_CORE_WZ_INPUT_TYPES_H

#include "core/wz_types.h"

typedef struct {
    unsigned char source;
    unsigned char key;
    unsigned char pressed;
} wz_input_event_t;

#endif
