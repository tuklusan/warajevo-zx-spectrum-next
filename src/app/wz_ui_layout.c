/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_ui_layout.h"

#include <stdio.h>
#include <string.h>

#include "app/wz_host_audio_policy.h"

static const char* speed_labels[WZ_SPEED_COUNT] = {
    "25%", "50%", "100%", "200%", "400%", "800%", "Unlimited"
};

static const wz_ui_menu_node_t menus[WZ_UI_MENU_COUNT] = {
    {"file", "File"},
    {"machine", "Machine"},
    {"media", "Media"},
    {"view", "View"},
    {"tools", "Tools"},
    {"settings", "Settings"},
    {"help", "Help"}
};

static const wz_ui_toolbar_item_t toolbar[WZ_UI_TOOLBAR_COUNT] = {
    {"file.open_run", "Open / Run"},
    {"machine.pause_resume", "Pause / Resume"},
    {"machine.reset", "Reset"},
    {"machine.speed", "Emulation Speed"},
    {"media.tape", "Tape"},
    {"snapshot.load", "Load Snapshot"},
    {"snapshot.save", "Save Snapshot"},
    {"media.microdrive.drive1", "MDV 1"},
    {"host.screenshot.save", "Screenshot"},
    {"view.fullscreen", "Fullscreen"},
    {"tools.debugger", "Debugger"}
};

static const wz_ui_toolbar_item_t tape_actions[WZ_UI_TAPE_ACTION_COUNT] = {
    {"media.tape.insert", "Insert..."},
    {"media.tape.eject", "Eject"},
    {"media.tape.loading.normal", "Normal"},
    {"media.tape.loading.instant", "Instant / Trap"},
    {"media.tape.manager", "Open Tape Manager..."}
};

static const wz_ui_toolbar_item_t microdrive_actions[WZ_UI_MICRODRIVE_ACTION_COUNT] = {
    {"media.microdrive.mount.1", "Mount MDV 1..."},
    {"media.microdrive.eject.1", "Eject MDV 1"},
    {"media.microdrive.set_default.1", "Set MDV 1 Default"},
    {"media.microdrive.mount.2", "Mount MDV 2..."},
    {"media.microdrive.eject.2", "Eject MDV 2"},
    {"media.microdrive.set_default.2", "Set MDV 2 Default"},
    {"media.microdrive.mount.3", "Mount MDV 3..."},
    {"media.microdrive.eject.3", "Eject MDV 3"},
    {"media.microdrive.set_default.3", "Set MDV 3 Default"},
    {"media.microdrive.mount.4", "Mount MDV 4..."},
    {"media.microdrive.eject.4", "Eject MDV 4"},
    {"media.microdrive.set_default.4", "Set MDV 4 Default"},
    {"media.microdrive.mount.5", "Mount MDV 5..."},
    {"media.microdrive.eject.5", "Eject MDV 5"},
    {"media.microdrive.set_default.5", "Set MDV 5 Default"},
    {"media.microdrive.mount.6", "Mount MDV 6..."},
    {"media.microdrive.eject.6", "Eject MDV 6"},
    {"media.microdrive.set_default.6", "Set MDV 6 Default"},
    {"media.microdrive.mount.7", "Mount MDV 7..."},
    {"media.microdrive.eject.7", "Eject MDV 7"},
    {"media.microdrive.set_default.7", "Set MDV 7 Default"},
    {"media.microdrive.mount.8", "Mount MDV 8..."},
    {"media.microdrive.eject.8", "Eject MDV 8"},
    {"media.microdrive.set_default.8", "Set MDV 8 Default"}
};

void wz_ui_layout_state_init(wz_ui_layout_state_t* state)
{
    if (state == 0) {
        return;
    }
    state->model_k = 48u;
    state->speed_percent = 100u;
    state->unlimited_speed = false;
    state->paused = false;
    state->audio_muted = false;
    state->tape_mounted = false;
    state->microdrive1_mounted = false;
    for (size_t index = 0u; index < WZ_UI_MICRODRIVE_COUNT; ++index) {
        state->microdrive_mounted[index] = false;
    }
    state->fullscreen = false;
    state->status_panel_visible = false;
    state->networking_mode = "None";
    state->remote_control_enabled = false;
    state->remote_control_client_connected = false;
    state->control_port_available = false;
    state->control_port = 0u;
}

void wz_ui_layout_status_panel(const wz_ui_layout_state_t* state,
                               char* output,
                               size_t capacity)
{
    size_t index;
    size_t used;
    char speed_text[24];

    if (output == 0 || capacity == 0u) {
        return;
    }
    if (state == 0) {
        output[0] = '\0';
        return;
    }
    if (state->unlimited_speed) {
        (void)snprintf(speed_text, sizeof(speed_text), "Unlimited");
    } else {
        (void)snprintf(speed_text, sizeof(speed_text), "%u%%", state->speed_percent);
    }
    used = (size_t)snprintf(output, capacity,
                            "Model: %uK | Speed: %s | State: %s | Audio: %s | Tape: %s | ",
                            state->model_k,
                            speed_text,
                            state->paused ? "paused" : "running",
                            state->audio_muted ? "muted" : "audible",
                            state->tape_mounted ? "mounted" : "none");
    if (used >= capacity) {
        output[capacity - 1u] = '\0';
        return;
    }
    for (index = 0u; index < WZ_UI_MICRODRIVE_COUNT; ++index) {
        int written = snprintf(output + used, capacity - used,
                               "MDV %u: %s%s",
                               (unsigned)(index + 1u),
                               state->microdrive_mounted[index] ? "mounted" : "none",
                               index + 1u == WZ_UI_MICRODRIVE_COUNT ? " | " : ", ");
        if (written < 0 || (size_t)written >= capacity - used) {
            output[capacity - 1u] = '\0';
            return;
        }
        used += (size_t)written;
    }
    (void)snprintf(output + used, capacity - used,
                   "Networking: %s | Remote: %s%s",
                   state->networking_mode == 0 ? "None" : state->networking_mode,
                   state->remote_control_enabled ? "enabled" : "disabled",
                   state->remote_control_client_connected ? ", connected" : "");
    output[capacity - 1u] = '\0';
}

bool wz_ui_layout_toggle_status_panel(wz_ui_layout_state_t* state)
{
    if (state == 0) {
        return false;
    }
    state->status_panel_visible = !state->status_panel_visible;
    return state->status_panel_visible;
}

size_t wz_ui_layout_menu_count(void)
{
    return WZ_UI_MENU_COUNT;
}

const wz_ui_menu_node_t* wz_ui_layout_menu_at(size_t index)
{
    return index < WZ_UI_MENU_COUNT ? &menus[index] : 0;
}

size_t wz_ui_layout_toolbar_count(void)
{
    return WZ_UI_TOOLBAR_COUNT;
}

const wz_ui_toolbar_item_t* wz_ui_layout_toolbar_at(size_t index)
{
    return index < WZ_UI_TOOLBAR_COUNT ? &toolbar[index] : 0;
}

size_t wz_ui_layout_tape_action_count(void)
{
    return WZ_UI_TAPE_ACTION_COUNT;
}

const wz_ui_toolbar_item_t* wz_ui_layout_tape_action_at(size_t index)
{
    return index < WZ_UI_TAPE_ACTION_COUNT ? &tape_actions[index] : 0;
}

size_t wz_ui_layout_microdrive_action_count(void)
{
    return WZ_UI_MICRODRIVE_ACTION_COUNT;
}

const wz_ui_toolbar_item_t* wz_ui_layout_microdrive_action_at(size_t index)
{
    return index < WZ_UI_MICRODRIVE_ACTION_COUNT ? &microdrive_actions[index] : 0;
}

void wz_ui_layout_microdrive_overview_init(
    wz_ui_microdrive_overview_entry_t* entries)
{
    if (entries == 0) {
        return;
    }
    for (size_t index = 0u; index < WZ_UI_MICRODRIVE_COUNT; ++index) {
        entries[index].mounted = false;
        entries[index].host_image_identity = 0u;
        entries[index].logical_name[0] = '\0';
        entries[index].sector_count = 0u;
        entries[index].write_protected = false;
        entries[index].current_drive = false;
        entries[index].default_drive = false;
        entries[index].validation = WZ_UI_MICRODRIVE_VALIDATION_UNMOUNTED;
    }
}

size_t wz_ui_layout_microdrive_overview_count(void)
{
    return WZ_UI_MICRODRIVE_COUNT;
}

const wz_ui_microdrive_overview_entry_t* wz_ui_layout_microdrive_overview_at(
    const wz_ui_microdrive_overview_entry_t* entries,
    size_t index)
{
    return entries != 0 && index < WZ_UI_MICRODRIVE_COUNT
        ? &entries[index] : 0;
}

bool wz_ui_layout_microdrive_overview_set(
    wz_ui_microdrive_overview_entry_t* entries,
    size_t index,
    uint64_t host_image_identity,
    const char* logical_name,
    size_t sector_count,
    bool write_protected,
    bool current_drive,
    bool default_drive,
    wz_ui_microdrive_validation_t validation)
{
    wz_ui_microdrive_overview_entry_t* entry;

    if (entries == 0 || index >= WZ_UI_MICRODRIVE_COUNT ||
        logical_name == 0 || validation < WZ_UI_MICRODRIVE_VALIDATION_UNMOUNTED ||
        validation > WZ_UI_MICRODRIVE_VALIDATION_UNAVAILABLE) {
        return false;
    }
    entry = &entries[index];
    entry->mounted = validation != WZ_UI_MICRODRIVE_VALIDATION_UNMOUNTED;
    entry->host_image_identity = host_image_identity;
    (void)snprintf(entry->logical_name, sizeof(entry->logical_name), "%s",
                   logical_name);
    entry->sector_count = sector_count;
    entry->write_protected = write_protected;
    entry->current_drive = current_drive;
    entry->default_drive = default_drive;
    entry->validation = validation;
    return true;
}

const char* wz_ui_layout_microdrive_validation_label(
    wz_ui_microdrive_validation_t validation)
{
    static const char* labels[] = {
        "unmounted", "valid", "invalid", "unavailable"
    };
    return validation <= WZ_UI_MICRODRIVE_VALIDATION_UNAVAILABLE
        ? labels[validation] : 0;
}

void wz_ui_layout_tape_label(bool mounted, char* output, size_t capacity)
{
    if (output == 0 || capacity == 0u) {
        return;
    }
    (void)snprintf(output, capacity, "Tape: %s", mounted ? "mounted" : "none");
    output[capacity - 1u] = '\0';
}

void wz_ui_layout_status_line(const wz_ui_layout_state_t* state,
                              char* output,
                              size_t capacity)
{
    const char* pause_state;
    const char* audio_state;
    const char* control_port;
    char speed_text[24];

    if (output == 0 || capacity == 0u) {
        return;
    }
    if (state == 0) {
        output[0] = '\0';
        return;
    }
    pause_state = state->paused ? "paused" : "running";
    audio_state = state->audio_muted ? "muted" : "audible";
    control_port = state->control_port_available ? "available" : "unavailable";
    if (state->unlimited_speed) {
        (void)snprintf(speed_text, sizeof(speed_text), "Unlimited");
    } else {
        (void)snprintf(speed_text, sizeof(speed_text), "%u%%", state->speed_percent);
    }
    if (state->control_port_available) {
        (void)snprintf(output, capacity,
                       "Model: %uK | Speed: %s | %s | Audio: %s | Tape: %s | MDV 1: %s | Control Port: %u",
                       state->model_k,
                       speed_text,
                       pause_state,
                       audio_state,
                       state->tape_mounted ? "mounted" : "none",
                       state->microdrive1_mounted ? "mounted" : "none",
                       state->control_port);
    } else {
        (void)snprintf(output, capacity,
                       "Model: %uK | Speed: %s | %s | Audio: %s | Tape: %s | MDV 1: %s | Control Port: %s",
                       state->model_k,
                       speed_text,
                       pause_state,
                       audio_state,
                       state->tape_mounted ? "mounted" : "none",
                       state->microdrive1_mounted ? "mounted" : "none",
                       control_port);
    }
    output[capacity - 1u] = '\0';
}

size_t wz_ui_layout_speed_count(void)
{
    return WZ_SPEED_COUNT;
}

const char* wz_ui_layout_speed_label(size_t index)
{
    return index < WZ_SPEED_COUNT ? speed_labels[index] : 0;
}

bool wz_ui_layout_select_speed(wz_ui_layout_state_t* state,
                               wz_speed_policy_t speed)
{
    if (state == 0 || !wz_speed_policy_valid(speed)) {
        return false;
    }
    state->speed_percent = wz_speed_policy_percent(speed);
    state->unlimited_speed = wz_speed_policy_is_unlimited(speed);
    state->audio_muted = !wz_host_audio_enabled(speed);
    return true;
}

wz_result_t wz_ui_layout_activate_toolbar(
    const wz_command_registry_t* registry,
    size_t index,
    wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    const wz_ui_toolbar_item_t* item;

    if (result == 0 || index >= WZ_UI_TOOLBAR_COUNT) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    item = wz_ui_layout_toolbar_at(index);
    if (item == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    return wz_command_registry_dispatch(registry, item->command_id,
                                        arguments, result);
}

wz_result_t wz_ui_layout_activate_tape_action(
    const wz_command_registry_t* registry,
    size_t index,
    wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    const wz_ui_toolbar_item_t* item;

    if (result == 0 || index >= WZ_UI_TAPE_ACTION_COUNT) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    item = wz_ui_layout_tape_action_at(index);
    if (item == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    return wz_command_registry_dispatch(registry, item->command_id,
                                        arguments, result);
}

wz_result_t wz_ui_layout_activate_microdrive_action(
    const wz_command_registry_t* registry,
    size_t index,
    wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    const wz_ui_toolbar_item_t* item;

    if (result == 0 || index >= WZ_UI_MICRODRIVE_ACTION_COUNT) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    item = wz_ui_layout_microdrive_action_at(index);
    if (item == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    return wz_command_registry_dispatch(registry, item->command_id,
                                        arguments, result);
}

wz_command_state_t wz_ui_layout_command_state(
    const wz_command_registry_t* registry,
    const char* command_id,
    const char** disabled_reason)
{
    return wz_command_registry_state(registry, command_id, disabled_reason);
}
