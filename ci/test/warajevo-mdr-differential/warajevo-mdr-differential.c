/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/wz_machine.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Warajevo MDR differential failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

#define REFERENCE_HEADER_OFFSET 0u
#define REFERENCE_HEADER_SIZE 15u
#define REFERENCE_DATA_OFFSET 15u
#define REFERENCE_DATA_SIZE 528u
#define REFERENCE_SECTOR_SIZE 543u

static wz_byte_t checksum(const wz_byte_t* bytes, size_t length)
{
    unsigned sum = 0u;
    size_t index;

    for (index = 0u; index < length; ++index) {
        sum += bytes[index];
    }
    return (wz_byte_t)(sum % 255u);
}

static wz_byte_t image_byte(size_t drive, size_t sector, size_t offset)
{
    return (wz_byte_t)((drive * 29u + sector * 17u + offset * 3u) & 0xffu);
}

static void make_image(wz_byte_t* bytes, size_t drive)
{
    size_t sector;

    for (sector = 0u; sector < WZ_MDR_MIN_SECTORS; ++sector) {
        wz_byte_t* record = bytes + sector * REFERENCE_SECTOR_SIZE;
        wz_byte_t* header = record + REFERENCE_HEADER_OFFSET;
        wz_byte_t* data = record + REFERENCE_DATA_OFFSET;
        size_t offset;

        memset(record, 0, REFERENCE_SECTOR_SIZE);
        header[0] = 1u;
        header[1] = (wz_byte_t)(sector + 1u);
        memcpy(header + 4u, "WZN-DIFF  ", 10u);
        header[14] = checksum(header, 14u);

        data[0] = 0u;
        data[1] = (wz_byte_t)sector;
        data[2] = 0u;
        data[3] = 2u;
        memcpy(data + 4u, "RECORD    ", 10u);
        data[14] = checksum(data, 14u);
        for (offset = 0u; offset < 512u; ++offset) {
            data[15u + offset] = image_byte(drive, sector, offset);
        }
        data[527] = checksum(data + 15u, 512u);
    }
}

static bool select_motor(wz_machine_t* machine, size_t slot,
                         wz_master_tick_t* tick)
{
    size_t step;

    for (step = 0u; step < WZ_MACHINE_MICRODRIVE_COUNT; ++step) {
        const wz_byte_t data =
            step == WZ_MACHINE_MICRODRIVE_COUNT - 1u - slot ? 0u : 1u;
        if (wz_machine_interface1_control_write(
                machine, (wz_byte_t)(0x06u | data), (*tick)++) !=
                WZ_RESULT_OK ||
            wz_machine_interface1_control_write(
                machine, (wz_byte_t)(0x04u | data), (*tick)++) !=
                WZ_RESULT_OK) {
            return false;
        }
    }
    return wz_machine_interface1_active_motor(machine) == slot;
}

int main(void)
{
    static wz_machine_t machine;
    static wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE];
    static wz_byte_t image_bytes[WZ_MACHINE_MICRODRIVE_COUNT]
        [WZ_MDR_MIN_SECTORS * REFERENCE_SECTOR_SIZE];
    wz_mdr_image_t images[WZ_MACHINE_MICRODRIVE_COUNT];
    wz_master_tick_t tick = 0u;
    wz_byte_t value;
    size_t drive, index;
    bool initialized = false;
    int result = 1;

    REQUIRE(WZ_MDR_HEADER_OFFSET == REFERENCE_HEADER_OFFSET);
    REQUIRE(WZ_MDR_HEADER_SIZE == REFERENCE_HEADER_SIZE);
    REQUIRE(WZ_MDR_DATA_OFFSET == REFERENCE_DATA_OFFSET);
    REQUIRE(WZ_MDR_DATA_SIZE == REFERENCE_DATA_SIZE);
    REQUIRE(WZ_MDR_SECTOR_SIZE == REFERENCE_SECTOR_SIZE);
    memset(interface1_rom, 0, sizeof(interface1_rom));
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    initialized = true;
    REQUIRE(wz_machine_load_interface1_rom(&machine, interface1_rom,
            sizeof(interface1_rom), WZ_INTERFACE1_ROM_OLD) == WZ_RESULT_OK);
    REQUIRE(wz_machine_set_networking_mode(&machine,
            WZ_NETWORKING_INTERFACE1) == WZ_RESULT_OK);

    for (drive = 0u; drive < WZ_MACHINE_MICRODRIVE_COUNT; ++drive) {
        make_image(image_bytes[drive], drive);
        REQUIRE(wz_mdr_image_init(&images[drive], image_bytes[drive],
                sizeof(image_bytes[drive])) == WZ_RESULT_OK);
        REQUIRE(wz_machine_mount_microdrive(&machine, drive + 1u,
                &images[drive]) == WZ_RESULT_OK);
    }

    /* MDRIVE.ASM NEXT loads 15 header bytes, then 528 record bytes. */
    for (drive = 0u; drive < WZ_MACHINE_MICRODRIVE_COUNT; ++drive) {
        REQUIRE(select_motor(&machine, drive, &tick));
        REQUIRE(wz_machine_interface1_data_read(&machine, &value) ==
                WZ_RESULT_OK);
        REQUIRE(value == image_bytes[drive][REFERENCE_HEADER_OFFSET]);
    }
    REQUIRE(select_motor(&machine, 0u, &tick));
    for (index = 1u; index < WZ_MDR_MIN_SECTORS * REFERENCE_SECTOR_SIZE;
         ++index) {
        const size_t sector = index / REFERENCE_SECTOR_SIZE;
        const size_t offset = index % REFERENCE_SECTOR_SIZE;
        REQUIRE(wz_machine_interface1_data_read(&machine, &value) ==
                WZ_RESULT_OK);
        REQUIRE(value == image_bytes[0][index]);
        if (offset == REFERENCE_DATA_OFFSET) {
            const wz_mdr_transport_t* transport =
                wz_machine_microdrive_at_const(&machine, 0u);
            REQUIRE(transport != 0 && transport->phase == WZ_MDR_PHASE_DATA &&
                    transport->offset == REFERENCE_DATA_OFFSET + 1u);
        } else if (offset == REFERENCE_HEADER_OFFSET && sector != 0u) {
            const wz_mdr_transport_t* transport =
                wz_machine_microdrive_at_const(&machine, 0u);
            REQUIRE(transport != 0 && transport->sector == sector &&
                    transport->phase == WZ_MDR_PHASE_HEADER &&
                    transport->offset == REFERENCE_HEADER_OFFSET + 1u);
        }
        ++tick;
    }
    REQUIRE(wz_machine_interface1_data_read(&machine, &value) ==
            WZ_RESULT_OK);
    REQUIRE(value == image_bytes[0][REFERENCE_HEADER_OFFSET]);

    result = 0;

cleanup:
    if (initialized) {
        wz_machine_destroy(&machine);
    }
    if (result == 0) {
        (void)puts("PASS preserved Warajevo Microdrive sector layout and stream behavior");
    }
    return result;
}
