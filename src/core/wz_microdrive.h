/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_MICRODRIVE_H
#define WZ_CORE_WZ_MICRODRIVE_H

#include <stddef.h>

#include "core/wz_types.h"

#define WZ_MDR_SECTOR_SIZE 543u
#define WZ_MDR_MIN_SECTORS 9u
#define WZ_MDR_MAX_SECTORS 254u

typedef struct {
    const wz_byte_t* data;
    size_t length;
    size_t sector_count;
} wz_mdr_image_t;

wz_result_t wz_mdr_image_init(wz_mdr_image_t* image,
                              const wz_byte_t* data,
                              size_t length);
wz_result_t wz_mdr_image_sector(const wz_mdr_image_t* image,
                                size_t sector,
                                const wz_byte_t** data);

#endif
