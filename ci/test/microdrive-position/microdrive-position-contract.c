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
        (void)fprintf(stderr, "Microdrive position contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

static wz_byte_t fixture_byte(size_t slot, size_t sector, size_t offset)
{
    return (wz_byte_t)((slot * 37u + sector * 11u + offset) & 0xffu);
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

static bool read_data(wz_machine_t* machine, wz_master_tick_t* tick,
                      wz_byte_t* value)
{
    wz_bus_request_t request;

    wz_bus_request_init(&request, WZ_BUS_IO_READ, *tick, 0x12e7u, 0xffu, 4u);
    *tick += 4u;
    if (wz_machine_bus_request(machine, &request) != WZ_RESULT_OK ||
        request.source != WZ_BUS_SOURCE_INPUT) {
        return false;
    }
    *value = request.value;
    return true;
}

int main(void)
{
    static wz_machine_t machine;
    static wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE];
    static wz_byte_t image_data[WZ_MACHINE_MICRODRIVE_COUNT]
        [WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    wz_mdr_image_t images[WZ_MACHINE_MICRODRIVE_COUNT];
    const wz_mdr_transport_t* transport;
    wz_byte_t value = 0u;
    wz_master_tick_t tick = 0u;
    size_t slot, sector, offset, remaining;
    bool machine_initialized = false;
    int result = 1;

    memset(interface1_rom, 0, sizeof(interface1_rom));
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    machine_initialized = true;
    REQUIRE(wz_machine_load_interface1_rom(&machine, interface1_rom,
            sizeof(interface1_rom), WZ_INTERFACE1_ROM_OLD) == WZ_RESULT_OK);
    REQUIRE(wz_machine_set_networking_mode(&machine,
            WZ_NETWORKING_INTERFACE1) == WZ_RESULT_OK);

    for (slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
        for (sector = 0u; sector < WZ_MDR_MIN_SECTORS; ++sector) {
            for (offset = 0u; offset < WZ_MDR_SECTOR_SIZE; ++offset) {
                image_data[slot][sector * WZ_MDR_SECTOR_SIZE + offset] =
                    fixture_byte(slot, sector, offset);
            }
        }
        REQUIRE(wz_mdr_image_init(&images[slot], image_data[slot],
                sizeof(image_data[slot])) == WZ_RESULT_OK);
        REQUIRE(wz_machine_mount_microdrive(&machine, slot + 1u,
                &images[slot]) == WZ_RESULT_OK);
    }

    /* Every motor selects its own image and begins at its first header byte. */
    for (slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
        REQUIRE(select_motor(&machine, slot, &tick));
        REQUIRE(read_data(&machine, &tick, &value));
        REQUIRE(value == fixture_byte(slot, 0u, WZ_MDR_HEADER_OFFSET));
        transport = wz_machine_microdrive_at_const(&machine, slot);
        REQUIRE(transport != NULL && transport->sector == 0u &&
                transport->phase == WZ_MDR_PHASE_HEADER &&
                transport->offset == WZ_MDR_HEADER_OFFSET + 1u);
    }

    /* An unselected transport cannot advance; switching back preserves position. */
    REQUIRE(wz_mdr_transport_read(
        wz_machine_microdrive_at(&machine, 0u), &value) ==
        WZ_RESULT_INVALID_STATE);
    REQUIRE(select_motor(&machine, 0u, &tick));
    REQUIRE(read_data(&machine, &tick, &value));
    REQUIRE(value == fixture_byte(0u, 0u, WZ_MDR_HEADER_OFFSET + 1u));
    transport = wz_machine_microdrive_at_const(&machine, 0u);
    REQUIRE(transport->offset == WZ_MDR_HEADER_OFFSET + 2u);

    /* The 15 stored header bytes are followed by exactly 528 data bytes. */
    for (offset = WZ_MDR_HEADER_OFFSET + 2u;
         offset < WZ_MDR_DATA_OFFSET; ++offset) {
        REQUIRE(read_data(&machine, &tick, &value));
        REQUIRE(value == fixture_byte(0u, 0u, offset));
    }
    transport = wz_machine_microdrive_at_const(&machine, 0u);
    REQUIRE(transport->phase == WZ_MDR_PHASE_HEADER &&
            transport->offset == WZ_MDR_DATA_OFFSET);
    REQUIRE(read_data(&machine, &tick, &value));
    REQUIRE(value == fixture_byte(0u, 0u, WZ_MDR_DATA_OFFSET));
    transport = wz_machine_microdrive_at_const(&machine, 0u);
    REQUIRE(transport->phase == WZ_MDR_PHASE_DATA &&
            transport->offset == WZ_MDR_DATA_OFFSET + 1u);
    for (offset = WZ_MDR_DATA_OFFSET + 1u;
         offset < WZ_MDR_SECTOR_SIZE; ++offset) {
        REQUIRE(read_data(&machine, &tick, &value));
        REQUIRE(value == fixture_byte(0u, 0u, offset));
    }
    transport = wz_machine_microdrive_at_const(&machine, 0u);
    REQUIRE(transport->sector == 0u &&
            transport->offset == WZ_MDR_SECTOR_SIZE);
    REQUIRE(read_data(&machine, &tick, &value));
    REQUIRE(value == fixture_byte(0u, 1u, WZ_MDR_HEADER_OFFSET));
    transport = wz_machine_microdrive_at_const(&machine, 0u);
    REQUIRE(transport->sector == 1u &&
            transport->phase == WZ_MDR_PHASE_HEADER &&
            transport->offset == WZ_MDR_HEADER_OFFSET + 1u);

    /* The other motor's byte offset remains independent of drive zero. */
    REQUIRE(select_motor(&machine, 1u, &tick));
    REQUIRE(read_data(&machine, &tick, &value));
    REQUIRE(value == fixture_byte(1u, 0u, WZ_MDR_HEADER_OFFSET + 1u));
    transport = wz_machine_microdrive_at_const(&machine, 1u);
    REQUIRE(transport->sector == 0u &&
            transport->offset == WZ_MDR_HEADER_OFFSET + 2u);
    REQUIRE(select_motor(&machine, 0u, &tick));
    transport = wz_machine_microdrive_at_const(&machine, 0u);
    REQUIRE(transport->sector == 1u &&
            transport->offset == WZ_MDR_HEADER_OFFSET + 1u);

    /* Consume the remaining stream and prove it wraps to sector zero. */
    remaining = WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE -
        (transport->sector * WZ_MDR_SECTOR_SIZE + transport->offset);
    while (remaining-- > 0u) {
        transport = wz_machine_microdrive_at_const(&machine, 0u);
        sector = transport->sector;
        offset = transport->offset;
        if (offset == WZ_MDR_SECTOR_SIZE) {
            sector = (sector + 1u) % WZ_MDR_MIN_SECTORS;
            offset = WZ_MDR_HEADER_OFFSET;
        }
        REQUIRE(read_data(&machine, &tick, &value));
        REQUIRE(value == fixture_byte(0u, sector, offset));
    }
    transport = wz_machine_microdrive_at_const(&machine, 0u);
    REQUIRE(transport->sector == WZ_MDR_MIN_SECTORS - 1u &&
            transport->offset == WZ_MDR_SECTOR_SIZE);
    REQUIRE(read_data(&machine, &tick, &value));
    REQUIRE(value == fixture_byte(0u, 0u, WZ_MDR_HEADER_OFFSET));
    transport = wz_machine_microdrive_at_const(&machine, 0u);
    REQUIRE(transport->sector == 0u &&
            transport->phase == WZ_MDR_PHASE_HEADER &&
            transport->offset == WZ_MDR_HEADER_OFFSET + 1u);

    result = 0;

cleanup:
    if (machine_initialized) {
        wz_machine_destroy(&machine);
    }
    if (result == 0) {
        (void)puts("PASS Microdrive motor selection, stream position, and sector wrap");
    }
    return result;
}
