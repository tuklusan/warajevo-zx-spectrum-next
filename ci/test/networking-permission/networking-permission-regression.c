/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_networking_commands.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "networking permission regression failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_machine_t machine;
    wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE] = {0};
    wz_byte_t image_bytes[WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE] = {0};
    wz_mdr_image_t image;
    wz_networking_command_context_t context;
    wz_command_registry_t registry;
    wz_command_metadata_t storage[1];
    wz_command_result_t result;
    const char* reason = NULL;
    const uint8_t interface1 = (uint8_t)WZ_NETWORKING_INTERFACE1;
    const uint8_t none = (uint8_t)WZ_NETWORKING_NONE;

    memset(&machine, 0, sizeof(machine));
    memset(&context, 0, sizeof(context));
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_load_interface1_rom(
                &machine, interface1_rom, sizeof(interface1_rom),
                WZ_INTERFACE1_ROM_OLD) == WZ_RESULT_OK);
    context.machine = &machine;
    REQUIRE(wz_command_registry_init(&registry, storage, 1u) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_bind_owner_thread(&registry) == WZ_RESULT_OK);
    REQUIRE(wz_networking_commands_register(&registry, &context) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_finalize(&registry) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_remote_permission(
                &registry, WZ_NETWORKING_COMMAND_ID,
                (wz_command_arguments_t){&interface1, sizeof(interface1)}) ==
            WZ_COMMAND_REMOTE_SAFE);
    REQUIRE(wz_command_registry_dispatch(
                &registry, WZ_NETWORKING_COMMAND_ID,
                (wz_command_arguments_t){&interface1, sizeof(interface1)},
                &result) == WZ_RESULT_OK);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_INTERFACE1);

    REQUIRE(wz_mdr_image_init(&image, image_bytes, sizeof(image_bytes)) ==
            WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_mount(&machine.microdrive, &image) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_select_motor(&machine.microdrive, 0u) ==
            WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_set_write_mode(&machine.microdrive, 1u) ==
            WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_write(&machine.microdrive, 0xa5u) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_is_dirty(&machine.microdrive) != 0u);
    REQUIRE(wz_command_registry_state(
                &registry, WZ_NETWORKING_COMMAND_ID, &reason) ==
            WZ_COMMAND_DISABLED);
    REQUIRE(reason != NULL && strcmp(reason,
            "dirty-microdrive-requires-local-resolution") == 0);
    REQUIRE(wz_command_registry_remote_permission(
                &registry, WZ_NETWORKING_COMMAND_ID,
                (wz_command_arguments_t){&none, sizeof(none)}) ==
            WZ_COMMAND_HOST_WRITE);
    REQUIRE(wz_command_registry_dispatch(
                &registry, WZ_NETWORKING_COMMAND_ID,
                (wz_command_arguments_t){&none, sizeof(none)}, &result) !=
            WZ_RESULT_OK);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_INTERFACE1);
    REQUIRE(wz_mdr_transport_is_dirty(&machine.microdrive) != 0u);

    wz_machine_destroy(&machine);
    puts("Networking permission and dirty-media regression passed");
    return 0;
}
