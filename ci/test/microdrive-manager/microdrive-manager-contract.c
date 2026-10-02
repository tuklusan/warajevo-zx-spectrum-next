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

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "Microdrive manager contract failed at %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static wz_byte_t checksum(const wz_byte_t* bytes, size_t length)
{
    unsigned sum = 0u;
    size_t index;
    for (index = 0u; index < length; ++index) sum += bytes[index];
    return (wz_byte_t)(sum % 255u);
}

static void add_record(wz_byte_t* sector, wz_byte_t sequence,
                       const char* name, const char* content, size_t length)
{
    wz_byte_t* descriptor = sector + WZ_MDR_IMAGE_DATA_OFFSET;
    if (strlen(name) > 10u || length > 512u) return;
    descriptor[0] = sequence == 1u ? 2u : 0u;
    descriptor[1] = sequence;
    descriptor[2] = (wz_byte_t)length;
    descriptor[3] = 0u;
    memset(descriptor + 4u, ' ', 10u);
    memcpy(descriptor + 4u, name, strlen(name));
    memcpy(descriptor + 15u, content, length);
    descriptor[14] = checksum(descriptor, 14u);
    sector[WZ_MDR_SECTOR_SIZE - 1u] =
        checksum(descriptor + 15u, 512u);
}

int main(void)
{
    wz_byte_t bytes[WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    wz_mdr_image_t image;
    wz_microdrive_manager_file_t files[WZ_MICRODRIVE_MANAGER_MAX_FILES];
    wz_microdrive_manager_allocation_t allocation;
    size_t file_count = 0u;
    wz_byte_t* first = bytes;
    wz_byte_t* second = bytes + WZ_MDR_SECTOR_SIZE;

    REQUIRE(wz_microdrive_manager_format(bytes, sizeof(bytes), "TEST") ==
            WZ_RESULT_OK);
    REQUIRE(memcmp(bytes + WZ_MDR_IMAGE_HEADER_OFFSET + 4u, "TEST    ", 8u) == 0);
    REQUIRE(wz_mdr_image_init(&image, bytes, sizeof(bytes)) == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_validate(&image));
    REQUIRE(wz_microdrive_manager_catalog(&image, files,
        WZ_MICRODRIVE_MANAGER_MAX_FILES, &file_count, &allocation) ==
        WZ_RESULT_OK);
    REQUIRE(file_count == 0u && allocation.total_sectors == WZ_MDR_MIN_SECTORS &&
        allocation.free_sectors == WZ_MDR_MIN_SECTORS &&
        allocation.allocated_sectors == 0u && allocation.damaged_sectors == 0u);

    add_record(first, 0u, "GAME", "ABCDEFG", 7u);
    add_record(second, 1u, "GAME", "12345", 5u);
    REQUIRE(wz_mdr_image_init(&image, bytes, sizeof(bytes)) == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_catalog(&image, files,
        WZ_MICRODRIVE_MANAGER_MAX_FILES, &file_count, &allocation) ==
        WZ_RESULT_OK);
    REQUIRE(file_count == 1u && strcmp(files[0].name, "GAME") == 0 &&
        files[0].sector_count == 2u && files[0].byte_count == 12u &&
        allocation.allocated_sectors == 2u && allocation.free_sectors == 7u);

    REQUIRE(wz_microdrive_manager_rename(bytes, sizeof(bytes), "ARCHIVE") ==
            WZ_RESULT_OK);
    REQUIRE(wz_mdr_image_init(&image, bytes, sizeof(bytes)) == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_catalog(&image, files,
        WZ_MICRODRIVE_MANAGER_MAX_FILES, &file_count, &allocation) ==
        WZ_RESULT_OK);
    REQUIRE(file_count == 1u && strcmp(files[0].name, "ARCHIVE") == 0);

    REQUIRE(wz_microdrive_manager_optimize(bytes, sizeof(bytes)) ==
            WZ_RESULT_OK);
    REQUIRE(bytes[WZ_MDR_IMAGE_HEADER_OFFSET + 1u] == 1u && bytes[(WZ_MDR_MIN_SECTORS - 1u) *
        WZ_MDR_SECTOR_SIZE + WZ_MDR_IMAGE_HEADER_OFFSET + 1u] == WZ_MDR_MIN_SECTORS);
    REQUIRE(wz_microdrive_manager_format(bytes, sizeof(bytes),
        "NAME-IS-TOO-LONG") == WZ_RESULT_INVALID_ARGUMENT);
    bytes[WZ_MDR_IMAGE_HEADER_OFFSET + WZ_MDR_HEADER_SIZE - 1u] ^= 1u;
    REQUIRE(wz_mdr_image_init(&image, bytes, sizeof(bytes)) == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_catalog(&image, files,
        WZ_MICRODRIVE_MANAGER_MAX_FILES, &file_count, &allocation) ==
        WZ_RESULT_OK);
    REQUIRE(allocation.damaged_sectors == 1u);
    bytes[WZ_MDR_SECTOR_SIZE + WZ_MDR_SECTOR_SIZE - 1u] ^= 1u;
    REQUIRE(wz_mdr_image_init(&image, bytes, sizeof(bytes)) == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_catalog(&image, files,
        WZ_MICRODRIVE_MANAGER_MAX_FILES, &file_count, &allocation) ==
        WZ_RESULT_OK);
    REQUIRE(allocation.damaged_sectors == 2u);
    REQUIRE(wz_microdrive_manager_optimize(bytes, sizeof(bytes)) ==
        WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(!wz_microdrive_manager_validate(&image));
    puts("Microdrive manager contract passed");
    return 0;
}
