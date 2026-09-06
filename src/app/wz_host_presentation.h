/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_HOST_PRESENTATION_H
#define WZ_APP_WZ_HOST_PRESENTATION_H

#include <stddef.h>

typedef struct {
    size_t x;
    size_t y;
    size_t width;
    size_t height;
} wz_host_presentation_rect_t;

/* The default host view includes the complete canonical 448x312 raster. */
#define WZ_HOST_PRESENTATION_WIDTH WZ_RASTER_CANONICAL_WIDTH
#define WZ_HOST_PRESENTATION_HEIGHT WZ_RASTER_CANONICAL_HEIGHT

wz_host_presentation_rect_t wz_host_presentation_default_rect(void);

#endif
