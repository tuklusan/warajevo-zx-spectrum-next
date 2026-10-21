/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/wz_machine.h"
#include "core/wz_state.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "MDR trailer contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

int main(void)
{
    static wz_byte_t minimum_image[WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    static wz_byte_t protected_image[WZ_MDR_MAX_SECTORS * WZ_MDR_SECTOR_SIZE +
                                      WZ_MDR_WRITE_PROTECTION_TRAILER_SIZE];
    static wz_byte_t state_bytes[WZ_STATE_MACHINE_LENGTH];
    static wz_machine_t machine;
    static wz_machine_t restored;
    wz_mdr_image_t image;
    wz_mdr_image_t invalid_image;
    wz_mdr_transport_t transport;
    const wz_mdr_transport_t* restored_transport;
    wz_state_writer_t writer;
    size_t sector_count = 0u;
    const wz_byte_t* sector = NULL;
    bool machine_initialized = false;
    bool restored_initialized = false;
    int result = 1;

    memset(minimum_image, 0x5au, sizeof(minimum_image));
    memset(protected_image, 0x3cu, sizeof(protected_image));
    protected_image[sizeof(protected_image) - 1u] = 0u;

    REQUIRE(wz_mdr_image_sector_count(sizeof(minimum_image), &sector_count));
    REQUIRE(sector_count == WZ_MDR_MIN_SECTORS);
    REQUIRE(wz_mdr_image_sector_count(sizeof(minimum_image), NULL));
    REQUIRE(wz_mdr_image_init(&image, minimum_image, sizeof(minimum_image)) ==
            WZ_RESULT_OK);
    REQUIRE(image.length == sizeof(minimum_image));
    REQUIRE(image.sector_count == WZ_MDR_MIN_SECTORS);
    REQUIRE(!wz_mdr_image_is_write_protected(&image));
    REQUIRE(wz_mdr_image_sector(&image, WZ_MDR_MIN_SECTORS - 1u, &sector) ==
            WZ_RESULT_OK);
    REQUIRE(sector == minimum_image +
            (WZ_MDR_MIN_SECTORS - 1u) * WZ_MDR_SECTOR_SIZE);

    sector_count = 17u;
    REQUIRE(!wz_mdr_image_sector_count(
        (WZ_MDR_MAX_SECTORS - 1u) * WZ_MDR_SECTOR_SIZE + 1u,
        &sector_count));
    REQUIRE(sector_count == 17u);
    invalid_image = image;
    REQUIRE(wz_mdr_image_init(&invalid_image, protected_image,
        sizeof(protected_image) + 1u) == WZ_RESULT_PARSE_ERROR);
    REQUIRE(invalid_image.data == image.data &&
            invalid_image.length == image.length &&
            invalid_image.identity == image.identity);

    REQUIRE(wz_mdr_image_sector_count(sizeof(protected_image), &sector_count));
    REQUIRE(sector_count == WZ_MDR_MAX_SECTORS);
    REQUIRE(wz_mdr_image_init(&image, protected_image,
        sizeof(protected_image)) == WZ_RESULT_OK);
    REQUIRE(image.length == sizeof(protected_image));
    REQUIRE(image.sector_count == WZ_MDR_MAX_SECTORS);
    REQUIRE(!wz_mdr_image_is_write_protected(&image));

    wz_mdr_transport_init(&transport);
    REQUIRE(wz_mdr_transport_mount(&transport, &image) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_select_motor(&transport, 0u) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_set_write_mode(&transport, 1u) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_write(&transport, 0xa5u) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_is_dirty(&transport) != 0u);
    REQUIRE(protected_image[WZ_MDR_HEADER_OFFSET] == 0x3cu);

    protected_image[sizeof(protected_image) - 1u] = 1u;
    REQUIRE(wz_mdr_image_init(&image, protected_image,
        sizeof(protected_image)) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_image_is_write_protected(&image));
    wz_mdr_transport_init(&transport);
    REQUIRE(wz_mdr_transport_mount(&transport, &image) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_select_motor(&transport, 0u) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_set_write_mode(&transport, 1u) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_write(&transport, 0xa5u) == WZ_RESULT_INVALID_STATE);
    REQUIRE(wz_mdr_transport_is_dirty(&transport) == 0u);

    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    machine_initialized = true;
    REQUIRE(wz_machine_init(&restored, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    restored_initialized = true;
    REQUIRE(wz_machine_mount_microdrive(&machine, 1u, &image) == WZ_RESULT_OK);
    writer.data = state_bytes;
    writer.capacity = sizeof(state_bytes);
    writer.length = 0u;
    REQUIRE(wz_state_serialize_machine(&machine, &writer) == WZ_RESULT_OK &&
            writer.length <= writer.capacity);
    REQUIRE(wz_state_deserialize_machine(&restored, state_bytes,
                                         writer.length) == WZ_RESULT_OK);
    restored_transport = wz_machine_microdrive_at_const(&restored, 0u);
    REQUIRE(restored_transport != NULL);
    REQUIRE(restored_transport->image_length == sizeof(protected_image));
    REQUIRE(restored_transport->image_sector_count == WZ_MDR_MAX_SECTORS);
    REQUIRE(restored_transport->image_identity == image.identity);
    result = 0;

cleanup:
    if (restored_initialized) wz_machine_destroy(&restored);
    if (machine_initialized) wz_machine_destroy(&machine);
    if (result == 0) {
        puts("PASS MDR optional write-protection trailer parsing");
    }
    return result;
}
