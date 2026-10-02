/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */
#include "app/wz_microdrive_manager.h"
#include <stdio.h>
#include <string.h>
#define REQUIRE(condition) do { if (!(condition)) { \
    fprintf(stderr, "contract failed at line %d: %s\n", __LINE__, #condition); \
    return 1; } } while (0)
static wz_byte_t sum(const wz_byte_t* bytes, size_t length)
{
    unsigned value = 0u;
    for (size_t i = 0u; i < length; ++i) value += bytes[i];
    return (wz_byte_t)(value % 255u);
}
static void refresh(wz_byte_t* sector)
{
    wz_byte_t* descriptor = sector + WZ_MDR_IMAGE_DATA_OFFSET;
    sector[14] = sum(sector, 14u);
    descriptor[14] = sum(descriptor, 14u);
    sector[WZ_MDR_SECTOR_SIZE - 1u] = sum(descriptor + 15u, 512u);
}
int main(void)
{
    wz_byte_t image_bytes[WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    wz_byte_t replacement[WZ_MDR_SECTOR_SIZE];
    wz_byte_t value = 0x5au;
    wz_mdr_image_t image;
    wz_byte_t* sector;
    REQUIRE(wz_microdrive_manager_format(image_bytes, sizeof(image_bytes), "SECTORS") == WZ_RESULT_OK);
    REQUIRE(wz_mdr_image_init(&image, image_bytes, sizeof(image_bytes)) == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_sector_verify(&image, 0u));
    sector = image_bytes;
    sector[WZ_MDR_SECTOR_SIZE - 1u] ^= 1u;
    REQUIRE(!wz_microdrive_manager_sector_verify(&image, 0u));
    REQUIRE(wz_microdrive_manager_sector_repair(image_bytes, sizeof(image_bytes), 0u) == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_sector_verify(&image, 0u));
    REQUIRE(wz_microdrive_manager_sector_edit_data(image_bytes, sizeof(image_bytes),
        0u, 511u, &value, 1u) == WZ_RESULT_OK);
    REQUIRE(sector[WZ_MDR_IMAGE_DATA_OFFSET + 15u + 511u] == value);
    REQUIRE(wz_microdrive_manager_sector_verify(&image, 0u));
    REQUIRE(wz_microdrive_manager_sector_edit_data(image_bytes, sizeof(image_bytes),
        0u, 512u, &value, 1u) == WZ_RESULT_INVALID_ARGUMENT);
    memcpy(replacement, sector, sizeof(replacement));
    replacement[WZ_MDR_IMAGE_DATA_OFFSET + 15u] = 0xa5u;
    refresh(replacement);
    REQUIRE(wz_microdrive_manager_sector_edit_raw(image_bytes, sizeof(image_bytes),
        0u, replacement, sizeof(replacement)) == WZ_RESULT_OK);
    REQUIRE(sector[WZ_MDR_IMAGE_DATA_OFFSET + 15u] == 0xa5u);
    memcpy(replacement, sector, sizeof(replacement));
    replacement[14] ^= 1u;
    REQUIRE(wz_microdrive_manager_sector_edit_raw(image_bytes, sizeof(image_bytes),
        0u, replacement, sizeof(replacement)) == WZ_RESULT_INVALID_STATE);
    REQUIRE(wz_microdrive_manager_sector_verify(&image, 0u));
    memcpy(replacement, sector, sizeof(replacement));
    replacement[WZ_MDR_IMAGE_DATA_OFFSET + 2u] = 1u;
    replacement[WZ_MDR_IMAGE_DATA_OFFSET + 3u] = 2u;
    refresh(replacement);
    memcpy(sector, replacement, sizeof(replacement));
    REQUIRE(wz_microdrive_manager_sector_repair(image_bytes, sizeof(image_bytes),
        0u) == WZ_RESULT_INVALID_STATE);
    REQUIRE(wz_microdrive_manager_sector_edit_raw(image_bytes, sizeof(image_bytes),
        0u, replacement, sizeof(replacement)) == WZ_RESULT_INVALID_STATE);
    puts("Microdrive advanced sector contract passed");
    return 0;
}
