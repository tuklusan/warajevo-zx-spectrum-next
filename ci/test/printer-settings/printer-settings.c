/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_printer_commands.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Printer settings contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_machine_t machine;
    wz_command_registry_t registry;
    wz_command_metadata_t storage[1];
    wz_printer_command_context_t printer_context;
    const wz_command_metadata_t* printer_command = NULL;
    wz_command_result_t result;
    size_t index;
    uint8_t mode;
    bool settings_root_found = false;

    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    printer_context.machine = &machine;
    REQUIRE(wz_command_registry_init(&registry, storage, 1u) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_bind_owner_thread(&registry) == WZ_RESULT_OK);
    REQUIRE(wz_printer_commands_register(&registry, &printer_context) ==
            WZ_RESULT_OK);
    REQUIRE(wz_command_registry_finalize(&registry) == WZ_RESULT_OK);
    for (index = 0u; index < wz_command_registry_count(&registry); ++index) {
        const wz_command_metadata_t* command =
            wz_command_registry_at(&registry, index);
        if (command != NULL && strcmp(command->id,
                WZ_PRINTER_MODE_COMMAND_ID) == 0) {
            printer_command = command;
        }
    }
    REQUIRE(printer_command != NULL);
    REQUIRE(strcmp(printer_command->menu_group,
                   "settings.peripherals.zx_printer") == 0);
    for (index = 0u; index < wz_command_registry_menu_root_count(); ++index) {
        const wz_command_menu_root_t* root =
            wz_command_registry_menu_root_at(index);
        if (root != NULL && strcmp(root->id, "settings") == 0 &&
            strcmp(root->label, "Settings") == 0) {
            settings_root_found = true;
        }
    }
    REQUIRE(settings_root_found);
    REQUIRE(wz_command_registry_state(&registry,
            WZ_PRINTER_MODE_COMMAND_ID, NULL) == WZ_COMMAND_ENABLED);

    for (mode = (uint8_t)WZ_PRINTER_MODE_NONE;
         mode <= (uint8_t)WZ_PRINTER_MODE_HP_ENLARGED; ++mode) {
        REQUIRE(wz_command_registry_dispatch(&registry,
                WZ_PRINTER_MODE_COMMAND_ID,
                (wz_command_arguments_t){&mode, sizeof(mode)}, &result) ==
                WZ_RESULT_OK);
        REQUIRE(result.status == WZ_COMMAND_RESULT_SUCCESS);
        REQUIRE(wz_machine_printer_mode(&machine) ==
                (wz_printer_mode_t)mode);
    }

    wz_machine_destroy(&machine);
    (void)puts("Printer Settings contract passed");
    return 0;
}
