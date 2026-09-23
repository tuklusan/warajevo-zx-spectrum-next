/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_ui_layout.h"

#include <stdio.h>
#include <string.h>

#include "app/wz_host_audio_policy.h"

static const char* speed_labels[WZ_SPEED_COUNT] = {
    "25%", "50%", "100%", "200%", "400%", "800%", "Unlimited"
};

static const char* remote_listener_state_label(
    wz_ui_remote_listener_state_t state)
{
    switch (state) {
    case WZ_UI_REMOTE_LISTENER_DOWN:
        return "DOWN";
    case WZ_UI_REMOTE_LISTENER_UP:
        return "UP";
    case WZ_UI_REMOTE_LISTENER_DEGRADED:
        return "DEGRADED";
    case WZ_UI_REMOTE_LISTENER_UNAVAILABLE:
    default:
        return "UNAVAILABLE";
    }
}

static wz_ui_remote_listener_state_t remote_listener_state(
    const wz_control_port_owner_t* owner)
{
    bool ipv4;
    bool ipv6;

    if (owner == 0 || owner->selected_port == 0u) {
        return WZ_UI_REMOTE_LISTENER_UNAVAILABLE;
    }
    ipv4 = owner->ipv4_active;
    ipv6 = owner->ipv6_active;
    if (ipv4 && ipv6) {
        return WZ_UI_REMOTE_LISTENER_UP;
    }
    if (ipv4 || ipv6) {
        return WZ_UI_REMOTE_LISTENER_DEGRADED;
    }
    return WZ_UI_REMOTE_LISTENER_DOWN;
}

static const char* ui_focus_target_label(wz_ui_focus_target_t target)
{
    switch (target) {
    case WZ_UI_FOCUS_MENU: return "MENU";
    case WZ_UI_FOCUS_DIALOG: return "DIALOG";
    case WZ_UI_FOCUS_STATUS: return "STATUS";
    case WZ_UI_FOCUS_VIEWPORT:
    default: return "VIEWPORT";
    }
}

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

static const wz_ui_networking_option_t networking_options[
    WZ_UI_NETWORKING_OPTION_COUNT] = {
    {WZ_NETWORKING_NONE, WZ_UI_NETWORKING_COMMAND_ID, "None", true, 0},
    {WZ_NETWORKING_INTERFACE1, WZ_UI_NETWORKING_COMMAND_ID, "Interface-1",
     true, 0},
    {WZ_NETWORKING_EAR_MIC, WZ_UI_NETWORKING_COMMAND_ID, "Ear+Mic", false,
     WZ_UI_NETWORKING_EAR_MIC_DISABLED_REASON}
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
    state->networking_selection = WZ_NETWORKING_NONE;
    state->networking_mode = "None";
    state->remote_control_enabled = false;
    state->remote_control_client_connected = false;
    state->control_port_available = false;
    state->control_port = 0u;
    state->remote_listener_state = WZ_UI_REMOTE_LISTENER_UNAVAILABLE;
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

void wz_ui_layout_apply_control_port(
    wz_ui_layout_state_t* state,
    const wz_control_port_owner_t* owner)
{
    if (state == 0) {
        return;
    }
    state->control_port_available = owner != 0 && owner->selected_port != 0u;
    state->control_port = state->control_port_available
        ? (unsigned)owner->selected_port : 0u;
    state->remote_listener_state = remote_listener_state(owner);
    state->remote_control_enabled = owner != 0;
}

void wz_ui_layout_sync_remote_control(
    wz_ui_layout_state_t* state,
    wz_ui_remote_control_status_t* remote_status,
    const wz_control_port_owner_t* owner,
    bool active_client)
{
    wz_ui_remote_listener_state_t listener;

    if (state == 0) {
        return;
    }
    wz_ui_layout_apply_control_port(state, owner);
    state->remote_control_client_connected = active_client;
    if (remote_status == 0) {
        return;
    }
    listener = state->remote_listener_state;
    remote_status->service_enabled_by_architecture = owner != 0;
    remote_status->selected_control_port_available =
        owner != 0 && owner->selected_port != 0u;
    remote_status->selected_control_port = remote_status->selected_control_port_available
        ? (unsigned)owner->selected_port : 0u;
    remote_status->ipv4_listener_up = owner != 0 && owner->ipv4_active;
    remote_status->ipv6_listener_up = owner != 0 && owner->ipv6_active;
    remote_status->listener_state = listener;
    remote_status->active_client = active_client;
}

void wz_ui_remote_control_status_init(wz_ui_remote_control_status_t* status)
{
    if (status == 0) {
        return;
    }
    status->service_enabled_by_architecture = false;
    status->base_control_port = 30740u;
    status->probe_first_port = 30740u;
    status->probe_last_port = 32787u;
    status->selected_control_port_available = false;
    status->selected_control_port = 0u;
    status->ipv4_listener_up = false;
    status->ipv6_listener_up = false;
    status->listener_state = WZ_UI_REMOTE_LISTENER_UNAVAILABLE;
    status->active_client = false;
    status->plaintext_no_authentication = true;
    status->permission_summary = "default-deny host-file/destructive/quit policy";
}

void wz_ui_remote_control_status_page(
    const wz_ui_remote_control_status_t* status,
    char* output,
    size_t capacity)
{
    const char* selected;
    const char* permission_summary;

    if (output == 0 || capacity == 0u) {
        return;
    }
    if (status == 0) {
        output[0] = '\0';
        return;
    }
    selected = status->selected_control_port_available ? "" : "unavailable";
    permission_summary = status->permission_summary == 0
        ? "unspecified"
        : status->permission_summary;
    if (status->selected_control_port_available) {
        (void)snprintf(output, capacity,
                       "Telnet Keyboard & Remote Control | "
                       "Service: %s | Base Control Port: %u | "
                       "Probe range: %u-%u | Selected Control Port: %u | "
                       "IPv4 listener: %s | IPv6 listener: %s | "
                       "Listener: %s | Active client: %s | "
                       "Warning: %s | Permissions: %s",
                       status->service_enabled_by_architecture ? "enabled" : "disabled",
                       status->base_control_port,
                       status->probe_first_port,
                       status->probe_last_port,
                       status->selected_control_port,
                       status->ipv4_listener_up ? "UP" : "DOWN",
                       status->ipv6_listener_up ? "UP" : "DOWN",
                       remote_listener_state_label(status->listener_state),
                       status->active_client ? "ACTIVE" : "NONE",
                       status->plaintext_no_authentication ?
                           "plaintext/no-authentication" : "none",
                       permission_summary);
    } else {
        (void)snprintf(output, capacity,
                       "Telnet Keyboard & Remote Control | "
                       "Service: %s | Base Control Port: %u | "
                       "Probe range: %u-%u | Selected Control Port: %s | "
                       "IPv4 listener: %s | IPv6 listener: %s | "
                       "Listener: %s | Active client: %s | "
                       "Warning: %s | Permissions: %s",
                       status->service_enabled_by_architecture ? "enabled" : "disabled",
                       status->base_control_port,
                       status->probe_first_port,
                       status->probe_last_port,
                       selected,
                       status->ipv4_listener_up ? "UP" : "DOWN",
                       status->ipv6_listener_up ? "UP" : "DOWN",
                       remote_listener_state_label(status->listener_state),
                       status->active_client ? "ACTIVE" : "NONE",
                       status->plaintext_no_authentication ?
                           "plaintext/no-authentication" : "none",
                       permission_summary);
    }
    output[capacity - 1u] = '\0';
}

void wz_ui_keyboard_status_init(wz_ui_keyboard_status_t* status)
{
    if (status == 0) {
        return;
    }
    status->focus_target = WZ_UI_FOCUS_VIEWPORT;
    status->keyboard_operable = true;
    status->visible_focus = true;
    status->non_color_status = true;
}

void wz_ui_keyboard_status_page(const wz_ui_keyboard_status_t* status,
                               char* output,
                               size_t capacity)
{
    if (output == 0 || capacity == 0u) {
        return;
    }
    if (status == 0) {
        output[0] = '\0';
        return;
    }
    (void)snprintf(output, capacity,
                   "Keyboard workflows: %s | Focus: %s | Visible focus: %s | "
                   "Status signaling: %s",
                   status->keyboard_operable ? "OPERABLE" : "UNAVAILABLE",
                   ui_focus_target_label(status->focus_target),
                   status->visible_focus ? "YES" : "NO",
                   status->non_color_status ? "TEXTUAL" : "COLOR-ONLY");
    output[capacity - 1u] = '\0';
}

void wz_ui_accessibility_descriptor_init(
    wz_ui_accessibility_descriptor_t* descriptor)
{
    if (descriptor == 0) {
        return;
    }
    descriptor->toolkit = "Nuklear";
    descriptor->toolkit_revision =
        "e3e18dc1e4d3de935095d372aaa211f12183befb";
    descriptor->project_semantics_enabled = true;
    descriptor->native_adapter_available = false;
    descriptor->native_adapter_state = "unavailable-not-claimed";
}

void wz_ui_accessibility_descriptor_page(
    const wz_ui_accessibility_descriptor_t* descriptor,
    char* output,
    size_t capacity)
{
    if (output == 0 || capacity == 0u) {
        return;
    }
    if (descriptor == 0) {
        output[0] = '\0';
        return;
    }
    (void)snprintf(output, capacity,
                   "Accessibility toolkit: %s | Revision: %s | "
                   "Project semantics: %s | Native adapter: %s (%s)",
                   descriptor->toolkit == 0 ? "unspecified" : descriptor->toolkit,
                   descriptor->toolkit_revision == 0 ? "unspecified" :
                       descriptor->toolkit_revision,
                   descriptor->project_semantics_enabled ? "ENABLED" : "DISABLED",
                   descriptor->native_adapter_available ? "AVAILABLE" : "UNAVAILABLE",
                   descriptor->native_adapter_state == 0 ? "unspecified" :
                       descriptor->native_adapter_state);
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
    return wz_command_registry_menu_root_count();
}

const wz_ui_menu_node_t* wz_ui_layout_menu_at(size_t index)
{
    return (const wz_ui_menu_node_t*)wz_command_registry_menu_root_at(index);
}

size_t wz_ui_layout_toolbar_count(void)
{
    return WZ_UI_TOOLBAR_COUNT;
}

const wz_ui_toolbar_item_t* wz_ui_layout_toolbar_at(size_t index)
{
    return index < WZ_UI_TOOLBAR_COUNT ? &toolbar[index] : 0;
}

bool wz_ui_layout_toolbar_hit_test(float x, float y,
                                   float viewport_width,
                                   size_t* toolbar_index)
{
    float toolbar_top = WZ_UI_MENU_BAR_HEIGHT;
    float toolbar_bottom = toolbar_top + WZ_UI_TOOLBAR_HEIGHT;
    if (toolbar_index == 0 || viewport_width <= 0.0f || x < 0.0f ||
        x >= viewport_width || y < toolbar_top || y >= toolbar_bottom) {
        return false;
    }
    *toolbar_index = (size_t)(x / viewport_width *
                              (float)WZ_UI_TOOLBAR_COUNT);
    return *toolbar_index < WZ_UI_TOOLBAR_COUNT;
}

bool wz_ui_layout_menu_hit_test(float x, float y, float viewport_width,
                               size_t* menu_index)
{
    const size_t menu_count = wz_ui_layout_menu_count();
    if (menu_index == 0 || viewport_width <= 0.0f || x < 0.0f ||
        x >= viewport_width || y < 0.0f || y >= WZ_UI_MENU_BAR_HEIGHT ||
        menu_count == 0u) {
        return false;
    }
    *menu_index = (size_t)(x / viewport_width *
                           (float)menu_count);
    return *menu_index < menu_count;
}

size_t wz_ui_layout_menu_command_count(
    const wz_command_registry_t* registry, size_t menu_index)
{
    const wz_ui_menu_node_t* menu = wz_ui_layout_menu_at(menu_index);
    size_t count = 0u;
    size_t index;
    if (registry == 0 || menu == 0) {
        return 0u;
    }
    for (index = 0u; index < wz_command_registry_count(registry); ++index) {
        const wz_command_metadata_t* command =
            wz_command_registry_at(registry, index);
        if (command != 0 && command->menu_group != 0 &&
            strcmp(command->menu_group, menu->id) == 0) {
            ++count;
        }
    }
    return count;
}

const wz_command_metadata_t* wz_ui_layout_menu_command_at(
    const wz_command_registry_t* registry, size_t menu_index,
    size_t command_index)
{
    const wz_ui_menu_node_t* menu = wz_ui_layout_menu_at(menu_index);
    size_t index;
    size_t matched = 0u;
    if (registry == 0 || menu == 0) {
        return 0;
    }
    for (index = 0u; index < wz_command_registry_count(registry); ++index) {
        const wz_command_metadata_t* command =
            wz_command_registry_at(registry, index);
        if (command != 0 && command->menu_group != 0 &&
            strcmp(command->menu_group, menu->id) == 0) {
            if (matched == command_index) {
                return command;
            }
            ++matched;
        }
    }
    return 0;
}

bool wz_ui_layout_menu_command_hit_test(
    const wz_command_registry_t* registry, size_t menu_index,
    float x, float y, float viewport_width, size_t* command_index)
{
    const float menu_width = 240.0f;
    const float item_height = 24.0f;
    float left;
    float right;
    const size_t menu_count = wz_ui_layout_menu_count();
    size_t count;
    size_t index;
    if (command_index == 0 || viewport_width <= 0.0f || x < 0.0f ||
        x >= viewport_width || y < WZ_UI_MENU_BAR_HEIGHT ||
        menu_count == 0u || menu_index >= menu_count) {
        return false;
    }
    left = viewport_width * (float)menu_index / (float)menu_count;
    right = left + menu_width;
    if (right > viewport_width) {
        right = viewport_width;
    }
    count = wz_ui_layout_menu_command_count(registry, menu_index);
    if (x < left || x >= right ||
        y >= WZ_UI_MENU_BAR_HEIGHT + (float)count * item_height) {
        return false;
    }
    index = (size_t)((y - WZ_UI_MENU_BAR_HEIGHT) / item_height);
    if (index >= count) {
        return false;
    }
    *command_index = index;
    return true;
}

wz_result_t wz_ui_layout_activate_menu_command(
    const wz_command_registry_t* registry, size_t menu_index,
    size_t command_index, wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    const wz_command_metadata_t* command;
    if (result == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    command = wz_ui_layout_menu_command_at(registry, menu_index,
                                           command_index);
    if (command == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    return wz_command_registry_dispatch(registry, command->id, arguments,
                                        result);
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

size_t wz_ui_layout_networking_option_count(void)
{
    return WZ_UI_NETWORKING_OPTION_COUNT;
}

const wz_ui_networking_option_t* wz_ui_layout_networking_option_at(
    size_t index)
{
    return index < WZ_UI_NETWORKING_OPTION_COUNT
        ? &networking_options[index] : 0;
}

bool wz_ui_layout_select_networking(wz_ui_layout_state_t* state,
                                    wz_networking_mode_t mode)
{
    size_t index;

    if (state == 0) {
        return false;
    }
    for (index = 0u; index < WZ_UI_NETWORKING_OPTION_COUNT; ++index) {
        const wz_ui_networking_option_t* option =
            &networking_options[index];
        if (option->mode == mode) {
            if (!option->available) {
                return false;
            }
            state->networking_selection = mode;
            state->networking_mode = option->label;
            return true;
        }
    }
    return false;
}

wz_result_t wz_ui_layout_activate_networking(
    const wz_command_registry_t* registry,
    wz_networking_mode_t mode,
    wz_command_result_t* result)
{
    size_t index;

    if (result == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (index = 0u; index < WZ_UI_NETWORKING_OPTION_COUNT; ++index) {
        const wz_ui_networking_option_t* option =
            &networking_options[index];
        if (option->mode == mode) {
            const uint8_t encoded_mode = (uint8_t)mode;
            if (!option->available) {
                result->status = WZ_COMMAND_RESULT_UNAVAILABLE;
                result->result = WZ_RESULT_UNSUPPORTED_OPERATION;
                result->reason = option->disabled_reason;
                return result->result;
            }
            return wz_command_registry_dispatch(
                registry, option->command_id,
                (wz_command_arguments_t){&encoded_mode, sizeof(encoded_mode)},
                result);
        }
    }
    result->status = WZ_COMMAND_RESULT_REJECTED;
    result->result = WZ_RESULT_INVALID_ARGUMENT;
    result->reason = "unknown-networking-mode";
    return result->result;
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
    const char* listener;
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
    listener = remote_listener_state_label(state->remote_listener_state);
    if (state->unlimited_speed) {
        (void)snprintf(speed_text, sizeof(speed_text), "Unlimited");
    } else {
        (void)snprintf(speed_text, sizeof(speed_text), "%u%%", state->speed_percent);
    }
    if (state->control_port_available) {
        (void)snprintf(output, capacity,
                       "Model: %uK | Speed: %s | %s | Audio: %s | Tape: %s | MDV 1: %s | Control Port: %u | Telnet: %s/%s",
                       state->model_k,
                       speed_text,
                       pause_state,
                       audio_state,
                       state->tape_mounted ? "mounted" : "none",
                       state->microdrive1_mounted ? "mounted" : "none",
                       state->control_port,
                       listener,
                       state->remote_control_client_connected ? "ACTIVE" : "NONE");
    } else {
        (void)snprintf(output, capacity,
                       "Model: %uK | Speed: %s | %s | Audio: %s | Tape: %s | MDV 1: %s | Control Port: %s | Telnet: %s/%s",
                       state->model_k,
                       speed_text,
                       pause_state,
                       audio_state,
                       state->tape_mounted ? "mounted" : "none",
                       state->microdrive1_mounted ? "mounted" : "none",
                       control_port,
                       listener,
                       state->remote_control_client_connected ? "ACTIVE" : "NONE");
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
