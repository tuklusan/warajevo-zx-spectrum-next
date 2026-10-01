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
        fprintf(stderr, "Microdrive state contract failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    static wz_machine_t machine;
    static wz_machine_t restored;
    static wz_byte_t image_data[WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    static wz_byte_t serialized[WZ_STATE_MACHINE_LENGTH];
    static wz_byte_t dirty_sector[WZ_MDR_SECTOR_SIZE];
    wz_mdr_image_t image;
    wz_state_writer_t writer;

    memset(image_data, 0x5au, sizeof(image_data));
    memset(dirty_sector, 0xa5u, sizeof(dirty_sector));
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_init(&restored, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_mdr_image_init(&image, image_data, sizeof(image_data)) ==
            WZ_RESULT_OK);
    for (size_t slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
        wz_mdr_transport_t* transport = wz_machine_microdrive_at(&machine, slot);
        REQUIRE(transport != NULL);
        REQUIRE(wz_machine_mount_microdrive(&machine, slot + 1u, &image) ==
                WZ_RESULT_OK);
        memset(transport->buffer, (int)(0x20u + slot),
               sizeof(transport->buffer));
    }
    {
        wz_mdr_transport_t* dirty = wz_machine_microdrive_at(&machine, 3u);
        dirty->dirty = 1u;
        memcpy(dirty->buffer, dirty_sector, sizeof(dirty_sector));
    }
    writer.data = serialized;
    writer.capacity = sizeof(serialized);
    writer.length = 0u;
    REQUIRE(wz_state_serialize_machine(&machine, &writer) == WZ_RESULT_OK);
    REQUIRE(writer.length == WZ_STATE_MACHINE_LENGTH);
    REQUIRE(wz_state_deserialize_machine(&restored, serialized, writer.length) ==
            WZ_RESULT_OK);
    for (size_t slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
        const wz_mdr_transport_t* transport =
            wz_machine_microdrive_at_const(&restored, slot);
        REQUIRE(transport != NULL);
        if (slot == 3u) {
            REQUIRE(transport->dirty == 1u);
            REQUIRE(memcmp(transport->buffer, dirty_sector,
                           sizeof(dirty_sector)) == 0);
        } else {
            REQUIRE(transport->dirty == 0u);
            for (size_t index = 0u; index < sizeof(transport->buffer); ++index) {
                REQUIRE(transport->buffer[index] == 0u);
            }
        }
    }
    wz_machine_destroy(&restored);
    wz_machine_destroy(&machine);
    puts("Microdrive canonical state contract passed");
    return 0;
}
