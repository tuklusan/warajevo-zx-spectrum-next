/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_networking_commands.h"
#include "app/wz_ui_layout.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Networking exclusion contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_machine_t machine;
    wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE] = {0};
    wz_networking_command_context_t context;
    wz_command_registry_t registry;
    wz_command_metadata_t storage[1];
    wz_command_result_t result;
    wz_ui_layout_state_t ui;
    const wz_command_metadata_t* command;
    const char* reason = NULL;
    unsigned cases = 0u;

    (void)memset(&machine, 0, sizeof(machine));
    (void)memset(&context, 0, sizeof(context));
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
    REQUIRE(wz_command_registry_count(&registry) == 1u);
    cases += 6u;

    wz_ui_layout_state_init(&ui);
    REQUIRE(wz_ui_layout_select_networking(&ui, WZ_NETWORKING_INTERFACE1));
    REQUIRE(wz_ui_layout_activate_networking(
                &registry, WZ_NETWORKING_INTERFACE1, &result) == WZ_RESULT_OK);
    REQUIRE(ui.networking_selection == WZ_NETWORKING_INTERFACE1 &&
            machine.networking_mode == WZ_NETWORKING_INTERFACE1);
    cases += 3u;

    REQUIRE(!wz_ui_layout_select_networking(&ui, WZ_NETWORKING_EAR_MIC));
    REQUIRE(ui.networking_selection == WZ_NETWORKING_INTERFACE1);
    REQUIRE(wz_ui_layout_activate_networking(
                &registry, WZ_NETWORKING_EAR_MIC, &result) ==
            WZ_RESULT_UNSUPPORTED_OPERATION);
    REQUIRE(result.status == WZ_COMMAND_RESULT_UNAVAILABLE &&
            strcmp(result.reason,
                   WZ_UI_NETWORKING_EAR_MIC_DISABLED_REASON) == 0);
    REQUIRE(ui.networking_selection == WZ_NETWORKING_INTERFACE1 &&
            machine.networking_mode == WZ_NETWORKING_INTERFACE1);
    cases += 5u;

    command = wz_command_registry_find(&registry, WZ_NETWORKING_COMMAND_ID);
    REQUIRE(command != NULL && command->handler_identity != NULL &&
            strcmp(command->id, WZ_NETWORKING_COMMAND_ID) == 0);
    REQUIRE(wz_command_registry_state(&registry, WZ_NETWORKING_COMMAND_ID,
                                     &reason) == WZ_COMMAND_ENABLED);
    cases += 2u;

    wz_machine_destroy(&machine);
    (void)printf("ui-networking-exclusion cases=%u status=pass\n", cases);
    return 0;
}
