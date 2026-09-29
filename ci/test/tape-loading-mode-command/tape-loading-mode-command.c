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
#include "app/wz_tape_media_commands.h"
#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "tape loading mode command failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

typedef struct {
    wz_machine_t* machine;
    wz_tape_segment_t segment;
    size_t release_count;
    size_t manager_open_count;
    char loaded_path[64];
} tape_fixture_t;

static bool load_tape(const char* path, void* opaque)
{
    tape_fixture_t* fixture = (tape_fixture_t*)opaque;
    size_t length = strlen(path);
    if (length >= sizeof(fixture->loaded_path) || strcmp(path, "broken.tap") == 0) {
        return false;
    }
    memcpy(fixture->loaded_path, path, length + 1u);
    return wz_machine_mount_tape(fixture->machine, &fixture->segment, 1u) ==
        WZ_RESULT_OK;
}

static void release_tape(void* opaque)
{
    tape_fixture_t* fixture = (tape_fixture_t*)opaque;
    ++fixture->release_count;
}

static void open_tape_manager(void* opaque)
{
    tape_fixture_t* fixture = (tape_fixture_t*)opaque;
    ++fixture->manager_open_count;
}

int main(void)
{
    wz_machine_t machine = {0};
    wz_command_metadata_t storage[6];
    wz_command_registry_t registry;
    wz_tape_loading_command_context_t context = {0};
    wz_tape_media_command_context_t media_context = {0};
    tape_fixture_t fixture = {0};
    wz_command_result_t result;
    static const char instant[] = "instant";
    static const char uppercase_normal[] = "NORMAL";
    static const char invalid[] = "turbo";
    static const char tape_path[] = "fixture.tap";
    static const char broken_path[] = "broken.tap";
    const char* reason = NULL;

    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    context.machine = &machine;
    REQUIRE(wz_command_registry_init(&registry, storage,
                                     sizeof(storage) / sizeof(storage[0])) ==
            WZ_RESULT_OK);
    REQUIRE(wz_command_registry_bind_owner_thread(&registry) == WZ_RESULT_OK);
    REQUIRE(wz_tape_loading_commands_register(&registry, &context) ==
            WZ_RESULT_OK);
    fixture.machine = &machine;
    fixture.segment.duration = 16u;
    fixture.segment.ear_level = 0u;
    fixture.segment.motor_stop_after = 0u;
    media_context.machine = &machine;
    media_context.load = load_tape;
    media_context.release = release_tape;
    media_context.open_manager = open_tape_manager;
    media_context.context = &fixture;
    REQUIRE(wz_tape_media_commands_register(&registry, &media_context) ==
            WZ_RESULT_OK);
    REQUIRE(wz_command_registry_finalize(&registry) == WZ_RESULT_OK);
    REQUIRE(wz_machine_tape_loading_mode(&machine) == WZ_TAPE_LOADING_NORMAL);
    REQUIRE(wz_command_registry_state(&registry, WZ_TAPE_EJECT_COMMAND_ID,
            &reason) == WZ_COMMAND_DISABLED);
    REQUIRE(strcmp(reason, "no-tape-mounted") == 0);
    REQUIRE(wz_command_registry_remote_permission(&registry,
            WZ_TAPE_INSERT_COMMAND_ID,
            (wz_command_arguments_t){tape_path, sizeof(tape_path) - 1u}) ==
            WZ_COMMAND_LOCAL_ONLY);
    REQUIRE(wz_command_registry_remote_permission(&registry,
            WZ_TAPE_EJECT_COMMAND_ID,
            (wz_command_arguments_t){NULL, 0u}) ==
            WZ_COMMAND_MEDIA_DESTRUCTIVE);
    REQUIRE(wz_command_registry_dispatch(&registry, WZ_TAPE_INSERT_COMMAND_ID,
            (wz_command_arguments_t){tape_path, sizeof(tape_path) - 1u},
            &result) == WZ_RESULT_OK);
    REQUIRE(machine.tape_mounted != 0u);
    REQUIRE(strcmp(fixture.loaded_path, tape_path) == 0);
    REQUIRE(wz_command_registry_state(&registry, WZ_TAPE_EJECT_COMMAND_ID,
            &reason) == WZ_COMMAND_ENABLED);
    REQUIRE(wz_command_registry_dispatch(&registry,
            WZ_TAPE_MANAGER_COMMAND_ID,
            (wz_command_arguments_t){NULL, 0u}, &result) == WZ_RESULT_OK);
    REQUIRE(fixture.manager_open_count == 1u);
    REQUIRE(wz_command_registry_dispatch(&registry, WZ_TAPE_EJECT_COMMAND_ID,
            (wz_command_arguments_t){NULL, 0u}, &result) == WZ_RESULT_OK);
    REQUIRE(machine.tape_mounted == 0u);
    REQUIRE(fixture.release_count == 1u);
    REQUIRE(wz_command_registry_dispatch(&registry, WZ_TAPE_INSERT_COMMAND_ID,
            (wz_command_arguments_t){broken_path, sizeof(broken_path) - 1u},
            &result) == WZ_RESULT_PARSE_ERROR);
    REQUIRE(machine.tape_mounted == 0u);
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
