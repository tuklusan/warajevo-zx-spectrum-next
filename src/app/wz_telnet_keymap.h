/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_TELNET_KEYMAP_H
#define WZ_APP_WZ_TELNET_KEYMAP_H

#include <stdbool.h>
#include <stddef.h>

bool wz_telnet_keymap_lookup(const char* name, size_t* physical_key);

#endif
