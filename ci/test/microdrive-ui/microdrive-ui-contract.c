/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */
#include "app/wz_ui_layout.h"
#include "app/wz_microdrive_eject_workflow.h"
#include "core/wz_machine_profile.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "Microdrive UI contract failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

typedef struct {
    bool mounted[WZ_UI_MICRODRIVE_COUNT];
    bool dirty[WZ_UI_MICRODRIVE_COUNT];
    size_t default_slot;
    size_t calls;
    size_t last_slot;
    wz_ui_microdrive_action_t last_action;
} test_context_t;

typedef struct {
    wz_byte_t* image;
    size_t length;
    size_t calls;
    bool fail;
} persistence_context_t;

static wz_result_t persist_sector(size_t sector, const wz_byte_t* data,
                                  size_t length, void* opaque)
{
    persistence_context_t* persist = (persistence_context_t*)opaque;
    size_t offset;
    if (persist == NULL || data == NULL ||
        length != WZ_MDR_SECTOR_SIZE ||
        sector > SIZE_MAX / WZ_MDR_SECTOR_SIZE) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    ++persist->calls;
    if (persist->fail) return WZ_RESULT_INVALID_STATE;
    offset = sector * WZ_MDR_SECTOR_SIZE;
    if (offset > persist->length || length > persist->length - offset) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    memcpy(persist->image + offset, data, length);
    return WZ_RESULT_OK;
}

static int check_eject_resolution(void)
{
    wz_machine_t machine = {0};
    wz_byte_t image_bytes[2][WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE] = {{0}};
    wz_mdr_image_t images[2];
    persistence_context_t persistence = {0};
    wz_mdr_transport_t* transport;
    bool ejected = true;

    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    memset(image_bytes[1], 0x11, sizeof(image_bytes[1]));
    for (size_t slot = 0u; slot < 2u; ++slot) {
        REQUIRE(wz_mdr_image_init(&images[slot], image_bytes[slot],
                                  sizeof(image_bytes[slot])) == WZ_RESULT_OK);
        REQUIRE(wz_machine_mount_microdrive(&machine, slot + 1u,
                                             &images[slot]) == WZ_RESULT_OK);
        transport = wz_machine_microdrive_at(&machine, slot);
        REQUIRE(wz_mdr_transport_select_motor(transport,
                    (wz_byte_t)slot) == WZ_RESULT_OK);
        REQUIRE(wz_mdr_transport_set_write_mode(transport, 1u) ==
                WZ_RESULT_OK);
        REQUIRE(wz_mdr_transport_write(transport,
                    slot == 0u ? 0xa5u : 0x77u) == WZ_RESULT_OK);
    }

    persistence.image = image_bytes[0];
    persistence.length = sizeof(image_bytes[0]);
    persistence.fail = true;
    REQUIRE(wz_microdrive_eject_resolve(&machine, 0u,
                WZ_MICRODRIVE_EJECT_CANCEL, persist_sector, &persistence,
                &ejected) == WZ_RESULT_OK);
    REQUIRE(!ejected && persistence.calls == 0u);
    transport = wz_machine_microdrive_at(&machine, 0u);
    REQUIRE(transport->image_present != 0u &&
            wz_mdr_transport_is_dirty(transport) != 0u);

    REQUIRE(wz_microdrive_eject_resolve(&machine, 0u,
                WZ_MICRODRIVE_EJECT_SAVE, persist_sector, &persistence,
                &ejected) == WZ_RESULT_INVALID_STATE);
    REQUIRE(!ejected && persistence.calls == 1u &&
            transport->image_present != 0u &&
            wz_mdr_transport_is_dirty(transport) != 0u);
    persistence.fail = false;
    REQUIRE(wz_microdrive_eject_resolve(&machine, 0u,
                WZ_MICRODRIVE_EJECT_SAVE, persist_sector, &persistence,
                &ejected) == WZ_RESULT_OK);
    REQUIRE(ejected && persistence.calls == 2u &&
            image_bytes[0][WZ_MDR_HEADER_OFFSET] == 0xa5u &&
            wz_machine_microdrive_at(&machine, 0u)->image_present == 0u);

    persistence.image = image_bytes[1];
    persistence.length = sizeof(image_bytes[1]);
    REQUIRE(wz_microdrive_eject_resolve(&machine, 1u,
                WZ_MICRODRIVE_EJECT_DISCARD, persist_sector, &persistence,
                &ejected) == WZ_RESULT_OK);
    REQUIRE(ejected && persistence.calls == 2u &&
            image_bytes[1][WZ_MDR_HEADER_OFFSET] == 0x11u &&
            wz_machine_microdrive_at(&machine, 1u)->image_present == 0u);
    REQUIRE(wz_microdrive_eject_resolve(&machine,
                WZ_MACHINE_MICRODRIVE_COUNT, WZ_MICRODRIVE_EJECT_DISCARD,
                persist_sector, &persistence, &ejected) ==
            WZ_RESULT_INVALID_ARGUMENT);
    wz_machine_destroy(&machine);
    return 0;
}

static int check_overview_model(void)
{
    wz_ui_microdrive_overview_entry_t entries[WZ_UI_MICRODRIVE_COUNT];
    wz_ui_layout_microdrive_overview_init(entries);
    REQUIRE(wz_ui_layout_microdrive_overview_count() ==
            WZ_UI_MICRODRIVE_COUNT);
    for (size_t slot = 0u; slot < WZ_UI_MICRODRIVE_COUNT; ++slot) {
        const wz_ui_microdrive_overview_entry_t* entry =
            wz_ui_layout_microdrive_overview_at(entries, slot);
        REQUIRE(entry != NULL && !entry->mounted &&
                entry->validation == WZ_UI_MICRODRIVE_VALIDATION_UNMOUNTED);
        REQUIRE(wz_ui_layout_microdrive_overview_set(entries, slot,
                    UINT64_C(0x1020304050607000) + slot, "CARTRIDGE",
                    WZ_MDR_MIN_SECTORS + slot, (slot & 1u) != 0u,
                    slot == 3u, slot == 5u,
                    WZ_UI_MICRODRIVE_VALIDATION_VALID));
        entry = wz_ui_layout_microdrive_overview_at(entries, slot);
        REQUIRE(entry != NULL && entry->mounted &&
                entry->host_image_identity ==
                    UINT64_C(0x1020304050607000) + slot &&
                strcmp(entry->logical_name, "CARTRIDGE") == 0 &&
                entry->sector_count == WZ_MDR_MIN_SECTORS + slot &&
                entry->write_protected == ((slot & 1u) != 0u) &&
                entry->current_drive == (slot == 3u) &&
                entry->default_drive == (slot == 5u) &&
                entry->validation == WZ_UI_MICRODRIVE_VALIDATION_VALID);
    }
    REQUIRE(wz_ui_layout_microdrive_overview_at(entries,
                WZ_UI_MICRODRIVE_COUNT) == NULL);
    REQUIRE(!wz_ui_layout_microdrive_overview_set(entries,
                WZ_UI_MICRODRIVE_COUNT, 0u, "", 0u, false, false, false,
                WZ_UI_MICRODRIVE_VALIDATION_VALID));
    return 0;
}

static bool action_available(const void* opaque, size_t slot,
                             wz_ui_microdrive_action_t action,
                             const char** reason)
{
    const test_context_t* test = (const test_context_t*)opaque;
    if (reason != NULL) *reason = NULL;
    if (test == NULL || slot >= WZ_UI_MICRODRIVE_COUNT) {
        if (reason != NULL) *reason = "invalid-slot";
        return false;
    }
    if (action == WZ_UI_MICRODRIVE_ACTION_MOUNT &&
        (test->mounted[slot] || test->dirty[slot])) {
        if (reason != NULL) *reason = "slot-must-be-empty";
        return false;
    }
    if (action == WZ_UI_MICRODRIVE_ACTION_EJECT && !test->mounted[slot]) {
        if (reason != NULL) *reason = "slot-empty";
        return false;
    }
    return true;
}

static wz_result_t action_handler(const void* opaque, size_t slot,
                                  wz_ui_microdrive_action_t action,
                                  wz_command_arguments_t arguments,
                                  wz_command_result_t* result)
{
    test_context_t* test = (test_context_t*)opaque;
    if (test == NULL || result == NULL || arguments.size != 0u ||
        slot >= WZ_UI_MICRODRIVE_COUNT) return WZ_RESULT_INVALID_ARGUMENT;
    ++test->calls;
    test->last_slot = slot;
    test->last_action = action;
    if (action == WZ_UI_MICRODRIVE_ACTION_MOUNT) {
        test->mounted[slot] = true;
        (void)snprintf(result->message, sizeof(result->message), "mounted");
    } else if (action == WZ_UI_MICRODRIVE_ACTION_EJECT) {
        (void)snprintf(result->message, sizeof(result->message),
                       "confirmation-required");
    } else if (action == WZ_UI_MICRODRIVE_ACTION_SET_DEFAULT) {
        test->default_slot = slot;
        (void)snprintf(result->message, sizeof(result->message), "default-set");
    } else {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    return WZ_RESULT_OK;
}

int main(void)
{
    wz_command_registry_t registry;
    wz_command_metadata_t storage[WZ_UI_MICRODRIVE_COUNT *
                                  WZ_UI_MICRODRIVE_OPERATION_COUNT];
    wz_ui_microdrive_command_context_t contexts[
        WZ_UI_MICRODRIVE_COUNT * WZ_UI_MICRODRIVE_OPERATION_COUNT];
    test_context_t test = {0};
    wz_command_result_t result;
    size_t initial_count;

    REQUIRE(check_eject_resolution() == 0);
    REQUIRE(check_overview_model() == 0);

    REQUIRE(wz_command_registry_init(&registry, storage,
        sizeof(storage) / sizeof(storage[0])) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_bind_owner_thread(&registry) == WZ_RESULT_OK);
    REQUIRE(wz_ui_layout_register_microdrive_commands(&registry, contexts,
        sizeof(contexts) / sizeof(contexts[0]), &test,
        action_available, action_handler) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_count(&registry) ==
        WZ_UI_MICRODRIVE_COUNT * WZ_UI_MICRODRIVE_OPERATION_COUNT);
    REQUIRE(wz_command_registry_finalize(&registry) == WZ_RESULT_OK);

    initial_count = 0u;
    for (size_t slot = 0u; slot < WZ_UI_MICRODRIVE_COUNT; ++slot) {
        const size_t mount_index = slot * WZ_UI_MICRODRIVE_OPERATION_COUNT;
        const size_t eject_index = mount_index + 1u;
        const size_t default_index = mount_index + 2u;
        const wz_ui_toolbar_item_t* mount =
            wz_ui_layout_microdrive_action_at(mount_index);
        const wz_ui_toolbar_item_t* eject =
            wz_ui_layout_microdrive_action_at(eject_index);
        const wz_ui_toolbar_item_t* set_default =
            wz_ui_layout_microdrive_action_at(default_index);
        const wz_command_metadata_t* eject_metadata;

        REQUIRE(mount != NULL && eject != NULL && set_default != NULL);
        REQUIRE(wz_command_registry_state(&registry, mount->command_id, NULL) ==
                WZ_COMMAND_ENABLED);
        REQUIRE(wz_command_registry_state(&registry, eject->command_id, NULL) ==
                WZ_COMMAND_DISABLED);
        REQUIRE(wz_ui_layout_activate_microdrive_action(&registry, mount_index,
            (wz_command_arguments_t){NULL, 0u}, &result) == WZ_RESULT_OK);
        REQUIRE(test.calls == ++initial_count && test.last_slot == slot &&
                test.last_action == WZ_UI_MICRODRIVE_ACTION_MOUNT);

        REQUIRE(wz_command_registry_state(&registry, mount->command_id, NULL) ==
                WZ_COMMAND_DISABLED);
        test.dirty[slot] = true;
        REQUIRE(wz_ui_layout_activate_microdrive_action(&registry, eject_index,
            (wz_command_arguments_t){NULL, 0u}, &result) == WZ_RESULT_OK);
        REQUIRE(test.calls == ++initial_count);
        REQUIRE(strcmp(result.message, "confirmation-required") == 0);
        REQUIRE(test.last_slot == slot &&
                test.last_action == WZ_UI_MICRODRIVE_ACTION_EJECT);
        eject_metadata = wz_command_registry_find(&registry, eject->command_id);
        REQUIRE(eject_metadata != NULL &&
                eject_metadata->permission == WZ_COMMAND_MEDIA_DESTRUCTIVE &&
                !wz_command_permission_is_remote_safe(
                    wz_command_registry_remote_permission(&registry,
                        eject->command_id, (wz_command_arguments_t){NULL, 0u})));

        REQUIRE(wz_ui_layout_activate_microdrive_action(&registry, default_index,
            (wz_command_arguments_t){NULL, 0u}, &result) == WZ_RESULT_OK);
        REQUIRE(test.calls == ++initial_count);
        REQUIRE(test.default_slot == slot && test.last_slot == slot &&
                test.last_action == WZ_UI_MICRODRIVE_ACTION_SET_DEFAULT);
        test.mounted[slot] = false;
        test.dirty[slot] = false;
    }
    REQUIRE(initial_count == WZ_UI_MICRODRIVE_COUNT *
            WZ_UI_MICRODRIVE_OPERATION_COUNT);
    puts("microdrive-ui-actions=24 registry=pass confirmation=required overview=8");
    return 0;
}
