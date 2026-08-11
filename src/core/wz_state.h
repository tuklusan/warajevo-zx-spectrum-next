/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_STATE_H
#define WZ_CORE_WZ_STATE_H

#include <stddef.h>

#include "core/wz_types.h"

typedef struct wz_machine wz_machine_t;

typedef struct {
    wz_byte_t* data;
    size_t capacity;
    size_t length;
} wz_state_writer_t;

void wz_state_writer_init(wz_state_writer_t* writer,
                          wz_byte_t* data,
                          size_t capacity);
wz_result_t wz_state_serialize_machine(const wz_machine_t* machine,
                                       wz_state_writer_t* writer);
wz_result_t wz_state_hash_machine(const wz_machine_t* machine,
                                  wz_qword_t* hash);

#endif
