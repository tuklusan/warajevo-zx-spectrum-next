/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_telnet_input.h"

bool wz_telnet_input_set_key(wz_input_arbiter_t* arbiter,
                             size_t physical_key, bool pressed)
{
    return wz_input_arbiter_set(arbiter, WZ_INPUT_SOURCE_TELNET,
                                 physical_key, pressed);
}

bool wz_telnet_input_release_all(wz_input_arbiter_t* arbiter)
{
    return wz_input_arbiter_release_source(arbiter, WZ_INPUT_SOURCE_TELNET);
}
