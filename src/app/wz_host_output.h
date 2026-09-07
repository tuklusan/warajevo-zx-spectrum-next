/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_HOST_OUTPUT_H
#define WZ_APP_WZ_HOST_OUTPUT_H

#include <stdbool.h>
#include <stddef.h>

/* Publish complete host output without exposing a partially written file. */
bool wz_host_output_write_atomic(const char* path,
                                 const void* data,
                                 size_t size);

#endif
