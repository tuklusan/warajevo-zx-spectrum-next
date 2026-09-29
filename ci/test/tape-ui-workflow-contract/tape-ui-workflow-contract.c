/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "app/wz_tape_insert_action.h"
#include "app/wz_tape_media_commands.h"
#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"
#include "core/wz_tape.h"

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "tape UI workflow contract failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

typedef struct {
    wz_machine_t* machine;
    size_t load_calls;
    size_t release_calls;
    const wz_byte_t* bytes;
    size_t length;
} fixture_t;

static bool load_tape(const char* path, void* opaque)
{
    fixture_t* fixture = (fixture_t*)opaque;
    size_t count = 0u;
    wz_tape_segment_t* segments;
    wz_result_t parsed;
    (void)path;
    ++fixture->load_calls;
    parsed = wz_tape_parse_standard_tap(fixture->bytes, fixture->length,
        fixture->machine->profile->master_ticks_per_cpu_tstate, NULL, 0u,
        &count);
    if (parsed != WZ_RESULT_BUFFER_TOO_SMALL || count == 0u ||
        count > SIZE_MAX / sizeof(*segments)) return false;
    segments = (wz_tape_segment_t*)malloc(count * sizeof(*segments));
    if (segments == NULL) return false;
    parsed = wz_tape_parse_standard_tap(fixture->bytes, fixture->length,
        fixture->machine->profile->master_ticks_per_cpu_tstate, segments,
        count, &count);
    if (parsed == WZ_RESULT_OK) {
        parsed = wz_machine_mount_tape(fixture->machine, segments, count);
    }
    free(segments);
    return parsed == WZ_RESULT_OK;
}

static void release_tape(void* opaque)
{
    fixture_t* fixture = (fixture_t*)opaque;
    ++fixture->release_calls;
}

int main(void)
{
    static const wz_tape_segment_t original[] = {{19u, 1u, 0u}};
    static const wz_byte_t invalid_tap[] = {2u, 0u, 0xffu, 0u};
    static const wz_byte_t valid_tap[] = {2u, 0u, 0xffu, 0xffu};
    static const char path[] = "selected.tap";
    wz_machine_t machine = {0};
    wz_tape_t saved_tape;
    wz_tape_state_t saved_state;
    wz_byte_t saved_mounted;
    wz_command_metadata_t storage[3];
    wz_command_registry_t registry;
    wz_tape_media_command_context_t context = {0};
    wz_command_result_t result = {0};
    fixture_t fixture = {0};
    const char* reason = NULL;

    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_command_registry_init(&registry, storage,
            sizeof(storage) / sizeof(storage[0])) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_bind_owner_thread(&registry) == WZ_RESULT_OK);
    fixture.machine = &machine;
    fixture.bytes = valid_tap;
    fixture.length = sizeof(valid_tap);
    context.machine = &machine;
    context.load = load_tape;
    context.release = release_tape;
    context.context = &fixture;
    REQUIRE(wz_tape_media_commands_register(&registry, &context) ==
            WZ_RESULT_OK);
    REQUIRE(wz_command_registry_finalize(&registry) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_state(&registry, WZ_TAPE_EJECT_COMMAND_ID,
            &reason) == WZ_COMMAND_DISABLED);
    REQUIRE(reason != NULL && strcmp(reason, "no-tape-mounted") == 0);

    REQUIRE(wz_machine_mount_tape(&machine, original, 1u) == WZ_RESULT_OK);
    machine.tape_state.segment_elapsed = 7u;
    machine.tape_state.motor_on = true;
    saved_tape = machine.tape;
    saved_state = machine.tape_state;
    saved_mounted = machine.tape_mounted;
    REQUIRE(wz_tape_insert_action_dispatch(WZ_FILE_DIALOG_CANCELLED, NULL,
            &registry, &result) == WZ_TAPE_INSERT_ACTION_CANCELLED);
    REQUIRE(fixture.load_calls == 0u && machine.tape.segments == saved_tape.segments &&
        machine.tape_state.segment_elapsed == saved_state.segment_elapsed &&
        machine.tape_state.motor_on == saved_state.motor_on &&
        machine.tape_mounted == saved_mounted);

    fixture.bytes = invalid_tap;
    fixture.length = sizeof(invalid_tap);
    REQUIRE(wz_tape_insert_action_dispatch(WZ_FILE_DIALOG_SELECTED, path,
            &registry, &result) == WZ_TAPE_INSERT_ACTION_FAILED);
    REQUIRE(result.reason != NULL && strcmp(result.reason, "tape-load-failed") == 0);
    REQUIRE(fixture.load_calls == 1u && fixture.release_calls == 0u &&
        machine.tape.segments == saved_tape.segments &&
        machine.tape_state.segment_elapsed == saved_state.segment_elapsed &&
        machine.tape_state.motor_on == saved_state.motor_on &&
        machine.tape_mounted == saved_mounted);

    fixture.bytes = valid_tap;
    fixture.length = sizeof(valid_tap);
    REQUIRE(wz_tape_insert_action_dispatch(WZ_FILE_DIALOG_SELECTED, path,
            &registry, &result) == WZ_TAPE_INSERT_ACTION_INSERTED);
    REQUIRE(fixture.load_calls == 2u && machine.tape_mounted != 0u &&
        machine.tape_state.tape == &machine.tape &&
        machine.tape_state.segment_elapsed == 0u);
    REQUIRE(wz_command_registry_state(&registry, WZ_TAPE_EJECT_COMMAND_ID,
            &reason) == WZ_COMMAND_ENABLED);
    wz_machine_destroy(&machine);
    puts("PASS: chooser cancel, malformed replacement rollback, and eject availability");
    return 0;
}
