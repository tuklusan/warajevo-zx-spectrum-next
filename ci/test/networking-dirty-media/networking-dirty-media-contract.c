/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_microdrive_eject_workflow.h"
#include "app/wz_networking_commands.h"
#include "app/wz_ui_layout.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned cases;

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, \
            "Networking dirty-media contract failed at line %d: %s\n", \
            __LINE__, #condition); \
        return 1; \
    } \
    ++cases; \
} while (0)

typedef struct {
    wz_byte_t* image;
    size_t image_length;
    size_t calls;
    bool fail;
} persistence_context_t;

static wz_result_t persist_sector(size_t sector, const wz_byte_t* data,
                                  size_t length, void* opaque)
{
    persistence_context_t* persistence = (persistence_context_t*)opaque;
    size_t offset;
    if (persistence == NULL || data == NULL ||
        length != WZ_MDR_SECTOR_SIZE ||
        sector > SIZE_MAX / WZ_MDR_SECTOR_SIZE) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    ++persistence->calls;
    if (persistence->fail) return WZ_RESULT_INVALID_STATE;
    offset = sector * WZ_MDR_SECTOR_SIZE;
    if (offset > persistence->image_length ||
        length > persistence->image_length - offset) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    memcpy(persistence->image + offset, data, length);
    return WZ_RESULT_OK;
}

static int dispatch_mode(wz_command_registry_t* registry,
                         wz_networking_mode_t mode,
                         wz_command_result_t* result)
{
    const uint8_t encoded_mode = (uint8_t)mode;
    return wz_command_registry_dispatch(registry,
        WZ_NETWORKING_COMMAND_ID,
        (wz_command_arguments_t){&encoded_mode, sizeof(encoded_mode)}, result);
}

int main(void)
{
    wz_machine_t machine = {0};
    wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE] = {0};
    wz_byte_t image_bytes[WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE] = {0};
    wz_mdr_image_t image;
    wz_networking_command_context_t networking_context = {0};
    wz_command_registry_t registry;
    wz_command_metadata_t command_storage[1];
    wz_command_result_t result = {0};
    persistence_context_t persistence = {0};
    wz_mdr_transport_t* transport;
    const char* reason = NULL;
    bool ejected = true;

    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_load_interface1_rom(&machine, interface1_rom,
        sizeof(interface1_rom), WZ_INTERFACE1_ROM_OLD) == WZ_RESULT_OK);
    networking_context.machine = &machine;
    REQUIRE(wz_command_registry_init(&registry, command_storage, 1u) ==
            WZ_RESULT_OK);
    REQUIRE(wz_command_registry_bind_owner_thread(&registry) == WZ_RESULT_OK);
    REQUIRE(wz_networking_commands_register(&registry, &networking_context) ==
            WZ_RESULT_OK);
    REQUIRE(wz_command_registry_finalize(&registry) == WZ_RESULT_OK);
    REQUIRE(dispatch_mode(&registry, WZ_NETWORKING_INTERFACE1, &result) ==
            WZ_RESULT_OK);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_INTERFACE1);

    REQUIRE(wz_mdr_image_init(&image, image_bytes, sizeof(image_bytes)) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_mount_microdrive(&machine, 1u, &image) == WZ_RESULT_OK);
    transport = wz_machine_microdrive_at(&machine, 0u);
    REQUIRE(transport != NULL);
    REQUIRE(wz_mdr_transport_select_motor(transport, 0u) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_set_write_mode(transport, 1u) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_write(transport, 0xa5u) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_is_dirty(transport) != 0u);
    REQUIRE(wz_command_registry_state(&registry,
        WZ_NETWORKING_COMMAND_ID, &reason) == WZ_COMMAND_DISABLED);
    REQUIRE(reason != NULL && strcmp(reason,
        "dirty-microdrive-requires-local-resolution") == 0);
    REQUIRE(wz_ui_layout_activate_networking(&registry, WZ_NETWORKING_NONE,
        &result) != WZ_RESULT_OK);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_INTERFACE1 &&
            wz_mdr_transport_is_dirty(transport) != 0u &&
            transport->image_present != 0u);

    REQUIRE(wz_microdrive_eject_resolve(&machine, 0u,
        WZ_MICRODRIVE_EJECT_CANCEL, persist_sector, &persistence, &ejected) ==
            WZ_RESULT_OK);
    REQUIRE(!ejected && wz_mdr_transport_is_dirty(transport) != 0u);
    REQUIRE(wz_command_registry_state(&registry,
        WZ_NETWORKING_COMMAND_ID, &reason) == WZ_COMMAND_DISABLED);

    persistence.image = image_bytes;
    persistence.image_length = sizeof(image_bytes);
    persistence.fail = true;
    REQUIRE(wz_microdrive_eject_resolve(&machine, 0u,
        WZ_MICRODRIVE_EJECT_SAVE, persist_sector, &persistence, &ejected) ==
            WZ_RESULT_INVALID_STATE);
    REQUIRE(!ejected && wz_mdr_transport_is_dirty(transport) != 0u &&
            image_bytes[WZ_MDR_HEADER_OFFSET] == 0u);
    REQUIRE(wz_command_registry_state(&registry,
        WZ_NETWORKING_COMMAND_ID, &reason) == WZ_COMMAND_DISABLED);

    persistence.fail = false;
    REQUIRE(wz_microdrive_eject_resolve(&machine, 0u,
        WZ_MICRODRIVE_EJECT_SAVE, persist_sector, &persistence, &ejected) ==
            WZ_RESULT_OK);
    REQUIRE(ejected && persistence.calls == 2u);
    REQUIRE(image_bytes[WZ_MDR_HEADER_OFFSET] == 0xa5u);
    REQUIRE(wz_command_registry_state(&registry,
        WZ_NETWORKING_COMMAND_ID, &reason) == WZ_COMMAND_ENABLED);
    REQUIRE(dispatch_mode(&registry, WZ_NETWORKING_NONE, &result) ==
            WZ_RESULT_OK);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_NONE &&
            !wz_machine_microdrives_are_dirty(&machine));

    REQUIRE(dispatch_mode(&registry, WZ_NETWORKING_INTERFACE1, &result) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_mount_microdrive(&machine, 1u, &image) == WZ_RESULT_OK);
    transport = wz_machine_microdrive_at(&machine, 0u);
    REQUIRE(wz_mdr_transport_select_motor(transport, 0u) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_set_write_mode(transport, 1u) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_write(transport, 0x5au) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_is_dirty(transport) != 0u);
    REQUIRE(wz_microdrive_eject_resolve(&machine, 0u,
        WZ_MICRODRIVE_EJECT_DISCARD, persist_sector, &persistence, &ejected) ==
            WZ_RESULT_OK);
    REQUIRE(ejected && image_bytes[WZ_MDR_HEADER_OFFSET] == 0xa5u);
    REQUIRE(wz_command_registry_state(&registry,
        WZ_NETWORKING_COMMAND_ID, &reason) == WZ_COMMAND_ENABLED);
    REQUIRE(dispatch_mode(&registry, WZ_NETWORKING_NONE, &result) ==
            WZ_RESULT_OK);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_NONE &&
            !wz_machine_microdrives_are_dirty(&machine));

    wz_machine_destroy(&machine);
    (void)printf("networking-dirty-media cases=%u status=pass\n", cases);
    return 0;
}
