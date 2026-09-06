/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_DIAGNOSTIC_BLOCK_H
#define WZ_APP_WZ_DIAGNOSTIC_BLOCK_H

#include <stddef.h>

#include "core/wz_machine.h"

/* Caller-owned memory boundary for the canonical native diagnostic block. */
wz_result_t wz_diagnostic_block_save(const wz_machine_t* machine,
                                     wz_byte_t* data,
                                     size_t capacity,
                                     size_t* length);
wz_result_t wz_diagnostic_block_load(wz_machine_t* machine,
                                     const wz_byte_t* data,
                                     size_t length);

#endif
