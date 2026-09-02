/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_host_presentation.h"

wz_host_presentation_rect_t wz_host_presentation_default_rect(void)
{
    return (wz_host_presentation_rect_t){
        .x = 0u,
        .y = 0u,
        .width = WZ_HOST_PRESENTATION_WIDTH,
        .height = WZ_HOST_PRESENTATION_HEIGHT,
    };
}
