/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app/wz_tape_loading_commands.h"
#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "tape loading mode command failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_machine_t machine = {0};
    wz_command_metadata_t storage[4];
    wz_command_registry_t registry;
    wz_tape_loading_command_context_t context = {0};
    wz_command_result_t result;
    static const char instant[] = "instant";
    static const char uppercase_normal[] = "NORMAL";
    static const char invalid[] = "turbo";

    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    context.machine = &machine;
    REQUIRE(wz_command_registry_init(&registry, storage,
                                     sizeof(storage) / sizeof(storage[0])) ==
            WZ_RESULT_OK);
    REQUIRE(wz_command_registry_bind_owner_thread(&registry) == WZ_RESULT_OK);
    REQUIRE(wz_tape_loading_commands_register(&registry, &context) ==
            WZ_RESULT_OK);
    REQUIRE(wz_command_registry_finalize(&registry) == WZ_RESULT_OK);
    REQUIRE(wz_machine_tape_loading_mode(&machine) == WZ_TAPE_LOADING_NORMAL);
    REQUIRE(wz_command_registry_remote_permission(&registry,
            WZ_TAPE_LOADING_MODE_COMMAND_ID,
            (wz_command_arguments_t){instant, sizeof(instant) - 1u}) ==
            WZ_COMMAND_REMOTE_SAFE);

    REQUIRE(wz_command_registry_dispatch(&registry,
            WZ_TAPE_LOADING_MODE_COMMAND_ID,
            (wz_command_arguments_t){instant, sizeof(instant) - 1u},
            &result) == WZ_RESULT_OK);
    REQUIRE(wz_machine_tape_loading_mode(&machine) ==
            WZ_TAPE_LOADING_INSTANT_TRAP);
    REQUIRE(strcmp(result.message, "Instant / Trap") == 0);

    REQUIRE(wz_command_registry_dispatch(&registry,
            WZ_TAPE_LOADING_NORMAL_COMMAND_ID,
            (wz_command_arguments_t){NULL, 0u}, &result) == WZ_RESULT_OK);
    REQUIRE(wz_machine_tape_loading_mode(&machine) == WZ_TAPE_LOADING_NORMAL);

    REQUIRE(wz_command_registry_dispatch(&registry,
            WZ_TAPE_LOADING_INSTANT_COMMAND_ID,
            (wz_command_arguments_t){NULL, 0u}, &result) == WZ_RESULT_OK);
    REQUIRE(wz_machine_tape_loading_mode(&machine) ==
            WZ_TAPE_LOADING_INSTANT_TRAP);
    REQUIRE(wz_command_registry_dispatch(&registry,
            WZ_TAPE_LOADING_MODE_COMMAND_ID,
            (wz_command_arguments_t){uppercase_normal,
                sizeof(uppercase_normal) - 1u}, &result) == WZ_RESULT_OK);
    REQUIRE(wz_machine_tape_loading_mode(&machine) == WZ_TAPE_LOADING_NORMAL);

    REQUIRE(wz_command_registry_dispatch(&registry,
            WZ_TAPE_LOADING_MODE_COMMAND_ID,
            (wz_command_arguments_t){invalid, sizeof(invalid) - 1u},
            &result) == WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(result.status == WZ_COMMAND_RESULT_REJECTED);
    REQUIRE(wz_machine_tape_loading_mode(&machine) == WZ_TAPE_LOADING_NORMAL);

    wz_machine_destroy(&machine);
    puts("PASS default, explicit, fixed-choice, and rejected tape loading modes");
    return 0;
}
