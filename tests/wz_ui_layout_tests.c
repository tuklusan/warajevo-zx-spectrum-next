/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "app/wz_ui_layout.h"

static bool unavailable(const void* context, const char** reason)
{
    (void)context;
    if (reason != 0) {
        *reason = "requires-media";
    }
    return false;
}

static wz_result_t handler(const void* context,
                           wz_command_arguments_t arguments,
                           wz_command_result_t* result)
{
    (void)context;
    (void)arguments;
    (void)result;
    return WZ_RESULT_OK;
}

static const void* activated_context;
static const void* activated_data;
static size_t activated_size;

static wz_result_t screenshot_handler(const void* context,
                                      wz_command_arguments_t arguments,
                                      wz_command_result_t* result)
{
    activated_context = context;
    activated_data = arguments.data;
    activated_size = arguments.size;
    result->status = WZ_COMMAND_RESULT_SUCCESS;
    return WZ_RESULT_OK;
}

int main(void)
{
    static const char* expected_menus[] = {
        "file", "machine", "media", "view", "tools", "settings", "help"
    };
    static const char* expected_toolbar[] = {
        "file.open_run", "machine.pause_resume", "machine.reset",
        "machine.speed", "media.tape", "snapshot.load", "snapshot.save",
        "media.microdrive.drive1", "host.screenshot.save", "view.fullscreen",
        "tools.debugger"
    };
    static const char* expected_toolbar_labels[] = {
        "Open / Run", "Pause / Resume", "Reset", "Emulation Speed",
        "Tape", "Load Snapshot", "Save Snapshot", "MDV 1", "Screenshot",
        "Fullscreen", "Debugger"
    };
    static const char* expected_tape_actions[] = {
        "media.tape.insert", "media.tape.eject",
        "media.tape.loading.normal", "media.tape.loading.instant",
        "media.tape.manager"
    };
    static const char* expected_tape_labels[] = {
        "Insert...", "Eject", "Normal", "Instant / Trap",
        "Open Tape Manager..."
    };
    wz_ui_layout_state_t state;
    char panel[512];
    char status[WZ_UI_STATUS_CAPACITY];
    size_t index;
    wz_command_metadata_t metadata;
    wz_command_metadata_t storage[1];
    wz_command_registry_t registry;
    const char* reason;
    const char screenshot_context = 'g';
    const char screenshot_argument[] = "gui-destination";
    char tape_label[32];

    if (wz_ui_layout_menu_count() != WZ_UI_MENU_COUNT ||
        wz_ui_layout_toolbar_count() != WZ_UI_TOOLBAR_COUNT) {
        return 1;
    }
    metadata = (wz_command_metadata_t){
        .id = "host.screenshot.save",
        .label = "Screenshot",
        .description = "Save the Spectrum display",
        .handler_identity = "gui.screenshot.save",
        .permission = WZ_COMMAND_HOST_WRITE,
        .handler = screenshot_handler,
        .handler_context = &screenshot_context,
    };
    if (wz_command_registry_init(&registry, storage, 1u) != WZ_RESULT_OK ||
        wz_command_registry_register(&registry, metadata) != WZ_RESULT_OK ||
        wz_command_registry_finalize(&registry) != WZ_RESULT_OK ||
        wz_ui_layout_activate_toolbar(
            &registry, 8u,
            (wz_command_arguments_t){screenshot_argument,
                                     sizeof(screenshot_argument) - 1u},
            &(wz_command_result_t){0}) != WZ_RESULT_OK ||
        activated_context != &screenshot_context ||
        activated_data != screenshot_argument ||
        activated_size != sizeof(screenshot_argument) - 1u ||
        wz_ui_layout_activate_toolbar(&registry, WZ_UI_TOOLBAR_COUNT,
                                      (wz_command_arguments_t){0, 0u},
                                      &(wz_command_result_t){0}) !=
            WZ_RESULT_INVALID_ARGUMENT) {
        return 1;
    }
    for (index = 0u; index < WZ_UI_MENU_COUNT; ++index) {
        const wz_ui_menu_node_t* item = wz_ui_layout_menu_at(index);
        if (item == 0 || strcmp(item->id, expected_menus[index]) != 0) {
            return 1;
        }
    }
    for (index = 0u; index < WZ_UI_TOOLBAR_COUNT; ++index) {
        const wz_ui_toolbar_item_t* item = wz_ui_layout_toolbar_at(index);
        if (item == 0 || strcmp(item->command_id, expected_toolbar[index]) != 0 ||
            strcmp(item->label, expected_toolbar_labels[index]) != 0) {
            return 1;
        }
    }
    if (wz_ui_layout_menu_at(WZ_UI_MENU_COUNT) != 0 ||
        wz_ui_layout_toolbar_at(WZ_UI_TOOLBAR_COUNT) != 0) {
        return 1;
    }
    if (wz_ui_layout_tape_action_count() != WZ_UI_TAPE_ACTION_COUNT) {
        return 1;
    }
    for (index = 0u; index < WZ_UI_TAPE_ACTION_COUNT; ++index) {
        const wz_ui_toolbar_item_t* item = wz_ui_layout_tape_action_at(index);
        if (item == 0 || strcmp(item->command_id, expected_tape_actions[index]) != 0 ||
            strcmp(item->label, expected_tape_labels[index]) != 0) {
            return 1;
        }
    }
    if (wz_ui_layout_tape_action_at(WZ_UI_TAPE_ACTION_COUNT) != 0) {
        return 1;
    }
    wz_ui_layout_tape_label(false, tape_label, sizeof(tape_label));
    if (strcmp(tape_label, "Tape: none") != 0) {
        return 1;
    }
    wz_ui_layout_tape_label(true, tape_label, sizeof(tape_label));
    if (strcmp(tape_label, "Tape: mounted") != 0) {
        return 1;
    }
    if (wz_ui_layout_speed_count() != WZ_SPEED_COUNT ||
        strcmp(wz_ui_layout_speed_label(0u), "25%") != 0 ||
        strcmp(wz_ui_layout_speed_label(6u), "Unlimited") != 0 ||
        wz_ui_layout_speed_label(WZ_SPEED_COUNT) != 0) {
        return 1;
    }
    wz_ui_layout_state_init(&state);
    wz_ui_layout_status_panel(&state, panel, sizeof(panel));
    if (wz_ui_layout_toggle_status_panel(&state) != true ||
        !state.status_panel_visible ||
        wz_ui_layout_toggle_status_panel(&state) != false ||
        state.status_panel_visible ||
        wz_ui_layout_toggle_status_panel(NULL) ||
        panel[0] == '\0') {
        return 1;
    }
    state.microdrive_mounted[7] = true;
    state.networking_mode = "Interface-1";
    state.remote_control_enabled = true;
    state.remote_control_client_connected = true;
    wz_ui_layout_status_panel(&state, panel, sizeof(panel));
    if (strstr(panel, "MDV 8: mounted") == 0 ||
        strstr(panel, "Networking: Interface-1") == 0 ||
        strstr(panel, "Remote: enabled, connected") == 0) {
        return 1;
    }
    if (!wz_ui_layout_select_speed(&state, WZ_SPEED_400) ||
        state.speed_percent != 400u || !state.audio_muted ||
        !wz_ui_layout_select_speed(&state, WZ_SPEED_100) ||
        state.speed_percent != 100u || state.audio_muted ||
        !wz_ui_layout_select_speed(&state, WZ_SPEED_UNLIMITED) ||
        !state.unlimited_speed || !state.audio_muted ||
        wz_ui_layout_select_speed(&state, (wz_speed_policy_t)WZ_SPEED_COUNT)) {
        return 1;
    }
    if (!wz_ui_layout_select_speed(&state, WZ_SPEED_100)) {
        return 1;
    }
    metadata = (wz_command_metadata_t){
        .id = "media.tape.insert",
        .label = "Insert Tape",
        .description = "Insert tape",
        .handler_identity = "test.handler",
        .permission = WZ_COMMAND_REMOTE_SAFE,
        .availability = unavailable,
        .handler = handler,
    };
    if (wz_command_registry_init(&registry, storage, 1u) != WZ_RESULT_OK ||
        wz_command_registry_register(&registry, metadata) != WZ_RESULT_OK ||
        wz_command_registry_finalize(&registry) != WZ_RESULT_OK ||
        wz_ui_layout_activate_tape_action(
            &registry, 0u, (wz_command_arguments_t){0, 0u},
            &(wz_command_result_t){0}) != WZ_RESULT_OK ||
        wz_ui_layout_command_state(&registry, "media.tape.insert", &reason) !=
            WZ_COMMAND_DISABLED || reason == 0 || strcmp(reason, "requires-media") != 0 ||
        wz_ui_layout_command_state(&registry, "missing.command", &reason) !=
            WZ_COMMAND_DISABLED || strcmp(reason, "unknown-command") != 0 ||
        wz_ui_layout_command_state(NULL, "media.tape.insert", &reason) !=
            WZ_COMMAND_DISABLED || strcmp(reason, "registry-unavailable") != 0) {
        return 1;
    }
    wz_ui_layout_status_line(&state, status, sizeof(status));
    if (strstr(status, "Model: 48K") == 0 ||
        strstr(status, "Speed: 100%") == 0 ||
        strstr(status, "Control Port: unavailable") == 0) {
        return 1;
    }
    state.control_port_available = true;
    state.control_port = 30740u;
    state.paused = true;
    state.audio_muted = true;
    wz_ui_layout_status_line(&state, status, sizeof(status));
    if (strstr(status, "paused") == 0 ||
        strstr(status, "Control Port: 30740") == 0) {
        return 1;
    }
    puts("wz_ui_layout contract passed");
    return 0;
}
