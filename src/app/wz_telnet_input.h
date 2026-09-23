/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_TELNET_INPUT_H
#define WZ_APP_WZ_TELNET_INPUT_H

#include <stdbool.h>
#include <stddef.h>

#include "app/wz_input_arbiter.h"

bool wz_telnet_input_set_key(wz_input_arbiter_t* arbiter,
                             size_t physical_key, bool pressed);
bool wz_telnet_input_release_all(wz_input_arbiter_t* arbiter);

#endif
