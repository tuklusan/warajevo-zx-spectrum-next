/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "core/wz_machine.h"
#include "core/wz_state.h"

static unsigned cases;

#define REQUIRE(condition) do { \
    ++cases; \
    if (!(condition)) { \
        (void)fprintf(stderr, "Networking mode guard failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

typedef struct {
    size_t calls;
    size_t fail_slot;
} flush_context_t;

static wz_result_t flush_slot(size_t slot, size_t sector,
                              const wz_byte_t* data, size_t length,
                              void* opaque)
{
    flush_context_t* context = (flush_context_t*)opaque;
    if (context == NULL || slot >= WZ_MACHINE_MICRODRIVE_COUNT ||
        sector >= WZ_MDR_MAX_SECTORS || data == NULL ||
        length != WZ_MDR_SECTOR_SIZE) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    ++context->calls;
    return slot == context->fail_slot ? WZ_RESULT_INVALID_STATE : WZ_RESULT_OK;
}

static bool all_drives_dirty(const wz_machine_t* machine)
{
    for (size_t slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
        const wz_mdr_transport_t* transport =
            wz_machine_microdrive_at_const(machine, slot);
        if (transport == NULL || transport->dirty == 0u) {
            return false;
        }
    }
    return true;
}

static bool initialize_dirty_machine(
    wz_machine_t* machine,
    wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE],
    wz_byte_t images[WZ_MACHINE_MICRODRIVE_COUNT]
        [WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE],
    wz_mdr_image_t parsed_images[WZ_MACHINE_MICRODRIVE_COUNT])
{
    if (wz_machine_load_interface1_rom(machine, interface1_rom,
            WZ_INTERFACE1_ROM_SIZE, WZ_INTERFACE1_ROM_NEW) != WZ_RESULT_OK ||
        wz_machine_set_networking_mode(machine,
            WZ_NETWORKING_INTERFACE1) != WZ_RESULT_OK) {
        return false;
    }
    for (size_t slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
        wz_mdr_transport_t* transport;
        memset(images[slot], (int)(0x30u + slot),
               WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE);
        if (wz_mdr_image_init(&parsed_images[slot], images[slot],
                WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE) != WZ_RESULT_OK ||
            wz_machine_mount_microdrive(machine, slot + 1u,
                &parsed_images[slot]) != WZ_RESULT_OK) {
            return false;
        }
        transport = wz_machine_microdrive_at(machine, slot);
        if (transport == NULL ||
            wz_mdr_transport_select_motor(transport, (wz_byte_t)slot) !=
                WZ_RESULT_OK ||
            wz_mdr_transport_set_write_mode(transport, 1u) != WZ_RESULT_OK ||
            wz_mdr_transport_write(transport, (wz_byte_t)(0xa0u + slot)) !=
                WZ_RESULT_OK) {
            return false;
        }
    }
    return all_drives_dirty(machine);
}

int main(void)
{
    static wz_machine_t machine;
    static wz_machine_t discard_machine;
    static wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE];
    static wz_byte_t images[WZ_MACHINE_MICRODRIVE_COUNT]
        [WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    static wz_byte_t discard_images[WZ_MACHINE_MICRODRIVE_COUNT]
        [WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    static wz_mdr_image_t parsed_images[WZ_MACHINE_MICRODRIVE_COUNT];
    static wz_mdr_image_t parsed_discard_images[WZ_MACHINE_MICRODRIVE_COUNT];
    flush_context_t flush = {0u, 3u};
    wz_qword_t before = 0u;
    wz_qword_t after = 0u;
    bool machine_initialized = false;
    bool discard_machine_initialized = false;
    int result = 1;

    for (size_t index = 0u; index < sizeof(interface1_rom); ++index) {
        interface1_rom[index] = (wz_byte_t)(index * 7u + 3u);
    }
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    machine_initialized = true;
    REQUIRE(initialize_dirty_machine(&machine, interface1_rom, images,
        parsed_images));
    REQUIRE(wz_state_hash_machine(&machine, &before) == WZ_RESULT_OK);
    REQUIRE(wz_machine_reconfigure_networking_mode(&machine,
        WZ_NETWORKING_NONE) == WZ_RESULT_INVALID_STATE);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_INTERFACE1 &&
            all_drives_dirty(&machine));
    REQUIRE(wz_state_hash_machine(&machine, &after) == WZ_RESULT_OK &&
            before == after);

    REQUIRE(wz_machine_reconfigure_networking_mode_with_mdr_bank_resolution(
        &machine, WZ_NETWORKING_NONE, flush_slot, &flush, false) ==
            WZ_RESULT_INVALID_STATE);
    REQUIRE(flush.calls == 4u &&
            machine.networking_mode == WZ_NETWORKING_INTERFACE1 &&
            all_drives_dirty(&machine));
    REQUIRE(wz_state_hash_machine(&machine, &after) == WZ_RESULT_OK &&
            before == after);

    flush.calls = 0u;
    flush.fail_slot = WZ_MACHINE_MICRODRIVE_COUNT;
    REQUIRE(wz_machine_reconfigure_networking_mode_with_mdr_bank_resolution(
        &machine, WZ_NETWORKING_NONE, flush_slot, &flush, false) ==
            WZ_RESULT_OK);
    REQUIRE(flush.calls == WZ_MACHINE_MICRODRIVE_COUNT &&
            machine.networking_mode == WZ_NETWORKING_NONE &&
            !wz_machine_microdrives_are_dirty(&machine));

    REQUIRE(wz_machine_init(&discard_machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    discard_machine_initialized = true;
    REQUIRE(initialize_dirty_machine(&discard_machine, interface1_rom,
        discard_images, parsed_discard_images));
    REQUIRE(wz_machine_reconfigure_networking_mode_with_mdr_bank_resolution(
        &discard_machine, WZ_NETWORKING_NONE, NULL, NULL, true) ==
            WZ_RESULT_OK);
    REQUIRE(discard_machine.networking_mode == WZ_NETWORKING_NONE &&
            !wz_machine_microdrives_are_dirty(&discard_machine));

    result = 0;

cleanup:
    if (discard_machine_initialized) wz_machine_destroy(&discard_machine);
    if (machine_initialized) wz_machine_destroy(&machine);
    if (result == 0) {
        (void)printf("PASS dirty Microdrive networking-mode guard; cases=%u\n",
                     cases);
    }
    return result;
}
