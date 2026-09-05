/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_microdrive.h"

#include <stdint.h>

wz_result_t wz_mdr_image_init(wz_mdr_image_t* image,
                              const wz_byte_t* data,
                              size_t length)
{
    size_t sector_count;

    if (image == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length % WZ_MDR_SECTOR_SIZE != 0u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    sector_count = length / WZ_MDR_SECTOR_SIZE;
    if (sector_count < WZ_MDR_MIN_SECTORS || sector_count > WZ_MDR_MAX_SECTORS) {
        return WZ_RESULT_PARSE_ERROR;
    }
    image->data = data;
    image->length = length;
    image->sector_count = sector_count;
    return WZ_RESULT_OK;
}

wz_result_t wz_mdr_image_sector(const wz_mdr_image_t* image,
                                size_t sector,
                                const wz_byte_t** data)
{
    if (image == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (image->data == 0 || image->sector_count < WZ_MDR_MIN_SECTORS ||
        image->sector_count > WZ_MDR_MAX_SECTORS ||
        image->length != image->sector_count * WZ_MDR_SECTOR_SIZE ||
        sector >= image->sector_count) {
        return WZ_RESULT_INVALID_STATE;
    }
    *data = image->data + sector * WZ_MDR_SECTOR_SIZE;
    return WZ_RESULT_OK;
}
