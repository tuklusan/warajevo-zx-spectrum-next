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

static void add_record(wz_byte_t* image, size_t sector_index,
                       wz_byte_t sequence, bool eof, const char* name,
                       const char* text)
{
    wz_byte_t* sector = image + sector_index * WZ_MDR_SECTOR_SIZE;
    wz_byte_t* descriptor = sector + WZ_MDR_IMAGE_DATA_OFFSET;
    size_t length = strlen(text);
    descriptor[0] = eof ? 2u : 0u;
    descriptor[1] = sequence;
    descriptor[2] = (wz_byte_t)length;
    descriptor[3] = 0u;
    memset(descriptor + 4u, ' ', 10u);
    memcpy(descriptor + 4u, name, strlen(name));
    memset(descriptor + 15u, 0, 512u);
    memcpy(descriptor + 15u, text, length);
    descriptor[14] = sum(descriptor, 14u);
    sector[WZ_MDR_SECTOR_SIZE - 1u] = sum(descriptor + 15u, 512u);
}

int main(void)
{
    wz_byte_t source[WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    wz_byte_t destination[WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    wz_mdr_image_t source_image, destination_image;
    wz_microdrive_manager_file_t files[WZ_MICRODRIVE_MANAGER_MAX_FILES];
    wz_microdrive_manager_allocation_t allocation;
    size_t count = 0u;
    REQUIRE(wz_microdrive_manager_format(source, sizeof(source), "SOURCE") == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_format(destination, sizeof(destination), "DEST") == WZ_RESULT_OK);
    add_record(source, 0u, 0u, false, "GAME", "ab");
    add_record(source, 1u, 1u, true, "GAME", "c");
    REQUIRE(wz_mdr_image_init(&source_image, source, sizeof(source)) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_image_init(&destination_image, destination, sizeof(destination)) == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_catalog(&source_image, files,
        WZ_MICRODRIVE_MANAGER_MAX_FILES, &count, &allocation) == WZ_RESULT_OK);
    REQUIRE(count == 1u && strcmp(files[0].name, "GAME") == 0 &&
        files[0].sector_count == 2u && files[0].byte_count == 3u &&
        !files[0].hidden);
    REQUIRE(wz_microdrive_manager_file_set_hidden(source, sizeof(source),
        "GAME", true) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_image_init(&source_image, source, sizeof(source)) == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_catalog(&source_image, files,
        WZ_MICRODRIVE_MANAGER_MAX_FILES, &count, &allocation) == WZ_RESULT_OK);
    REQUIRE(count == 1u && strcmp(files[0].name, "GAME") == 0 && files[0].hidden);
    REQUIRE(wz_microdrive_manager_file_rename(source, sizeof(source),
        "GAME", "PLAY") == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_file_set_hidden(source, sizeof(source),
        "PLAY", false) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_image_init(&source_image, source, sizeof(source)) == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_file_copy(source, sizeof(source), "PLAY",
        destination, sizeof(destination)) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_image_init(&destination_image, destination,
        sizeof(destination)) == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_catalog(&destination_image, files,
        WZ_MICRODRIVE_MANAGER_MAX_FILES, &count, &allocation) == WZ_RESULT_OK);
    REQUIRE(count == 1u && strcmp(files[0].name, "PLAY") == 0 &&
        files[0].sector_count == 2u && files[0].byte_count == 3u);
    REQUIRE(wz_microdrive_manager_file_copy(source, sizeof(source), "PLAY",
        destination, sizeof(destination)) == WZ_RESULT_INVALID_STATE);
    REQUIRE(wz_microdrive_manager_file_delete(source, sizeof(source),
        "PLAY") == WZ_RESULT_OK);
    REQUIRE(wz_mdr_image_init(&source_image, source, sizeof(source)) == WZ_RESULT_OK);
    REQUIRE(wz_microdrive_manager_catalog(&source_image, files,
        WZ_MICRODRIVE_MANAGER_MAX_FILES, &count, &allocation) == WZ_RESULT_OK);
    REQUIRE(count == 0u && allocation.free_sectors == WZ_MDR_MIN_SECTORS);
    return 0;
}
