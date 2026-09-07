/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_ui_layout.h"

#include <stdio.h>

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

wz_command_state_t wz_ui_layout_command_state(
    const wz_command_registry_t* registry,
    const char* command_id,
    const char** disabled_reason)
{
    return wz_command_registry_state(registry, command_id, disabled_reason);
}
