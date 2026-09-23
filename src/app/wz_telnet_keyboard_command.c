/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_telnet_keyboard_command.h"

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app/wz_host_output.h"
#include "app/wz_command_registry.h"
#include "app/wz_screenshot_service.h"
#include "app/wz_telnet_keymap.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#define WZ_SCREENSHOT_PATH_CAPACITY 1024u

static bool parse_key_command(const char* command,
                              const char* prefix,
                              size_t* physical_key)
{
    const char* operand;
    size_t length;
    char name[32];

    if (command == NULL || prefix == NULL || physical_key == NULL ||
        strncmp(command, prefix, strlen(prefix)) != 0) {
        return false;
    }
    operand = command + strlen(prefix);
    length = strlen(operand);
    if (length == 0u || length >= sizeof(name)) {
        return false;
    }
    memcpy(name, operand, length + 1u);
    return wz_telnet_keymap_lookup(name, physical_key);
}

bool wz_telnet_keyboard_command_key_down(const char* command,
                                         size_t* physical_key)
{
    return parse_key_command(command, "KEY DOWN ", physical_key);
}

bool wz_telnet_keyboard_command_key_up(const char* command,
                                       size_t* physical_key)
{
    return parse_key_command(command, "KEY UP ", physical_key);
}

bool wz_telnet_keyboard_command_key_press(const char* command,
                                          size_t* physical_key)
{
    return parse_key_command(command, "KEY PRESS ", physical_key);
}

bool wz_telnet_keyboard_command_format_response(
    wz_telnet_keyboard_response_t response,
    char* output,
    size_t output_capacity,
    size_t* output_length)
{
    const char* text;
    int written;
    if (output == NULL || output_length == NULL || output_capacity == 0u)
        return false;
    switch (response) {
    case WZ_TELNET_KEYBOARD_RESPONSE_OK: text = "OK\r\n"; break;
    case WZ_TELNET_KEYBOARD_RESPONSE_BAD_KEY: text = "ERR BAD_KEY\r\n"; break;
    case WZ_TELNET_KEYBOARD_RESPONSE_BAD_STATE: text = "ERR BAD_STATE\r\n"; break;
    default: return false;
    }
    written = snprintf(output, output_capacity, "%s", text);
    if (written < 0 || (size_t)written >= output_capacity) {
        *output_length = 0u;
        return false;
    }
    *output_length = (size_t)written;
    return true;
}

bool wz_telnet_error_format(wz_telnet_error_t error, char* output,
                            size_t output_capacity, size_t* output_length)
{
    static const char* const responses[] = {
        "ERR BAD_COMMAND\r\n", "ERR BAD_COMMAND_ID\r\n",
        "ERR BAD_ARGUMENT\r\n", "ERR LINE_TOO_LONG\r\n",
        "ERR BAD_KEY\r\n", "ERR BAD_MODEL\r\n", "ERR BAD_SPEED\r\n"
    };
    int written;
    if (output == NULL || output_length == NULL || output_capacity == 0u ||
        error > WZ_TELNET_ERROR_BAD_SPEED) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    written = snprintf(output, output_capacity, "%s", responses[error]);
    if (written < 0 || (size_t)written >= output_capacity) {
        *output_length = 0u;
        return false;
    }
    *output_length = (size_t)written;
    return true;
}

bool wz_telnet_help_format(char* output,
                           size_t output_capacity,
                           size_t* output_length)
{
    static const char response[] =
        "HELP\r\n"
        "STATUS\r\n"
        "KEY DOWN <key>\r\n"
        "KEY UP <key>\r\n"
        "KEY PRESS <key>\r\n"
        "RELEASE ALL\r\n"
        "PAUSE\r\n"
        "RESUME\r\n"
        "RESET\r\n"
        "MODEL 48K\r\n"
        "MODEL 128K\r\n"
        "SPEED 25|50|100|200|400|800|UNLIMITED\r\n"
        "SCREENSHOT\r\n"
        "MENU TREE\r\n"
        "DESCRIBE <command-id>\r\n"
        "DO <command-id> [arguments]\r\n"
        "Use MENU TREE and DESCRIBE for the complete registry.\r\n"
        "END\r\n";
    size_t length = sizeof(response) - 1u;
    if (output == NULL || output_length == NULL || output_capacity < length + 1u) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    memcpy(output, response, length + 1u);
    *output_length = length;
    return true;
}

bool wz_telnet_status_format(const wz_telnet_status_snapshot_t* snapshot,
                             char* output,
                             size_t output_capacity,
                             size_t* output_length)
{
    int written;
    if (snapshot == NULL || output == NULL || output_length == NULL ||
        output_capacity == 0u || snapshot->control_port < 30740u ||
        snapshot->control_port > 32787u || snapshot->model == NULL ||
        snapshot->state == NULL || snapshot->speed == NULL ||
        snapshot->audio == NULL || snapshot->networking == NULL) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    written = snprintf(output, output_capacity,
        "STATUS PROTOCOL=1 CONTROL_PORT=%u IPV4=%s IPV6=%s CLIENT=%s MODEL=%s STATE=%s SPEED=%s AUDIO=%s NETWORKING=%s\r\n",
        snapshot->control_port, snapshot->ipv4_up ? "UP" : "DOWN",
        snapshot->ipv6_up ? "UP" : "DOWN",
        snapshot->client_active ? "ACTIVE" : "NONE", snapshot->model,
        snapshot->state, snapshot->speed, snapshot->audio, snapshot->networking);
    if (written < 0 || (size_t)written >= output_capacity) {
        *output_length = 0u;
        return false;
    }
    *output_length = (size_t)written;
    return true;
}

bool wz_telnet_reset_format_response(bool reset_succeeded,
                                     char* output,
                                     size_t output_capacity,
                                     size_t* output_length)
{
    static const char response[] = "OK RESET\r\n";
    size_t length = sizeof(response) - 1u;
    if (!reset_succeeded || output == NULL || output_length == NULL ||
        output_capacity < length + 1u) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    memcpy(output, response, length + 1u);
    *output_length = length;
    return true;
}

bool wz_telnet_pause_resume_parse(const char* command,
                                  wz_telnet_pause_resume_command_t* parsed)
{
    if (command == NULL || parsed == NULL) return false;
    if (strcmp(command, "PAUSE") == 0) {
        *parsed = WZ_TELNET_PAUSE_RESUME_PAUSE;
        return true;
    }
    if (strcmp(command, "RESUME") == 0) {
        *parsed = WZ_TELNET_PAUSE_RESUME_RESUME;
        return true;
    }
    return false;
}

bool wz_telnet_pause_resume_apply(wz_telnet_pause_resume_command_t command,
                                  bool* paused)
{
    if (paused == NULL) return false;
    switch (command) {
    case WZ_TELNET_PAUSE_RESUME_PAUSE: *paused = true; return true;
    case WZ_TELNET_PAUSE_RESUME_RESUME: *paused = false; return true;
    default: return false;
    }
}

bool wz_telnet_pause_resume_format_response(
    wz_telnet_pause_resume_command_t command,
    char* output,
    size_t output_capacity,
    size_t* output_length)
{
    const char* response;
    int written;
    if (output == NULL || output_length == NULL || output_capacity == 0u) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    switch (command) {
    case WZ_TELNET_PAUSE_RESUME_PAUSE: response = "OK PAUSE\r\n"; break;
    case WZ_TELNET_PAUSE_RESUME_RESUME: response = "OK RESUME\r\n"; break;
    default: *output_length = 0u; return false;
    }
    written = snprintf(output, output_capacity, "%s", response);
    if (written < 0 || (size_t)written >= output_capacity) {
        *output_length = 0u;
        return false;
    }
    *output_length = (size_t)written;
    return true;
}

bool wz_telnet_model_parse(const char* command,
                           wz_telnet_model_command_t* parsed)
{
    if (command == NULL || parsed == NULL || strncmp(command, "MODEL ", 6u) != 0)
        return false;
    if (strcmp(command, "MODEL 48K") == 0) *parsed = WZ_TELNET_MODEL_48K;
    else if (strcmp(command, "MODEL 128K") == 0) *parsed = WZ_TELNET_MODEL_128K;
    else *parsed = WZ_TELNET_MODEL_BAD;
    return true;
}

bool wz_telnet_model_apply(wz_telnet_model_command_t command, bool* paused)
{
    if (paused == NULL) return false;
    switch (command) {
    case WZ_TELNET_MODEL_48K:
    case WZ_TELNET_MODEL_128K:
        return true;
    default: return false;
    }
}

bool wz_telnet_model_format_response(wz_telnet_model_command_t command,
                                     char* output,
                                     size_t output_capacity,
                                     size_t* output_length)
{
    const char* response;
    int written;
    if (output == NULL || output_length == NULL || output_capacity == 0u) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    switch (command) {
    case WZ_TELNET_MODEL_48K: response = "OK MODEL 48K\r\n"; break;
    case WZ_TELNET_MODEL_128K: response = "OK MODEL 128K\r\n"; break;
    case WZ_TELNET_MODEL_BAD: response = "ERR BAD_MODEL\r\n"; break;
    default: *output_length = 0u; return false;
    }
    written = snprintf(output, output_capacity, "%s", response);
    if (written < 0 || (size_t)written >= output_capacity) {
        *output_length = 0u;
        return false;
    }
    *output_length = (size_t)written;
    return true;
}

bool wz_telnet_speed_parse(const char* command,
                           wz_telnet_speed_command_t* parsed)
{
    static const char* const names[] = {
        "SPEED 25", "SPEED 50", "SPEED 100", "SPEED 200",
        "SPEED 400", "SPEED 800", "SPEED UNLIMITED"
    };
    size_t index;
    if (command == NULL || parsed == NULL || strncmp(command, "SPEED ", 6u) != 0)
        return false;
    for (index = 0u; index < 7u; ++index) {
        if (strcmp(command, names[index]) == 0) {
            *parsed = (wz_telnet_speed_command_t)index;
            return true;
        }
    }
    *parsed = WZ_TELNET_SPEED_BAD;
    return true;
}

bool wz_telnet_speed_apply(wz_telnet_speed_command_t command)
{
    return command >= WZ_TELNET_SPEED_25 && command <= WZ_TELNET_SPEED_UNLIMITED;
}

bool wz_telnet_speed_format_response(wz_telnet_speed_command_t command,
                                     char* output,
                                     size_t output_capacity,
                                     size_t* output_length)
{
    static const char* const responses[] = {
        "OK SPEED 25\r\n", "OK SPEED 50\r\n", "OK SPEED 100\r\n",
        "OK SPEED 200\r\n", "OK SPEED 400\r\n", "OK SPEED 800\r\n",
        "OK SPEED UNLIMITED\r\n", "ERR BAD_SPEED\r\n"
    };
    int written;
    if (output == NULL || output_length == NULL || output_capacity == 0u ||
        command > WZ_TELNET_SPEED_BAD) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    written = snprintf(output, output_capacity, "%s", responses[command]);
    if (written < 0 || (size_t)written >= output_capacity) {
        *output_length = 0u;
        return false;
    }
    *output_length = (size_t)written;
    return true;
}

bool wz_telnet_screenshot_parse(const char* command)
{
    return command != NULL && strcmp(command, "SCREENSHOT") == 0;
}

static bool screenshot_temp_directory(char* output, size_t capacity)
{
    const char* value = getenv("WZSN_SCREENSHOT_DIR");
    if (value != NULL && value[0] != '\0') {
        int written = snprintf(output, capacity, "%s", value);
        return written >= 0 && (size_t)written < capacity;
    }
#if defined(_WIN32)
    DWORD length = GetTempPathA((DWORD)capacity, output);
    return length != 0u && length < capacity;
#else
    value = getenv("TMPDIR");
    if (value == NULL || value[0] == '\0') value = "/tmp";
    return snprintf(output, capacity, "%s", value) >= 0 &&
        strlen(output) < capacity;
#endif
}

static bool screenshot_path(char* output, size_t capacity, unsigned suffix)
{
    char directory[WZ_SCREENSHOT_PATH_CAPACITY];
    time_t now = time(NULL);
    struct tm local_now;
    int written;
#if defined(_WIN32)
    if (!screenshot_temp_directory(directory, sizeof(directory)) ||
        localtime_s(&local_now, &now) != 0) return false;
#else
    if (!screenshot_temp_directory(directory, sizeof(directory))) return false;
    if (localtime_r(&now, &local_now) == NULL) return false;
#endif
    written = snprintf(output, capacity, "%s%sZX-Screen-%04d%02d%02d%02d%02d%02d000%s.png",
        directory, directory[strlen(directory) - 1u] == '/' ||
        directory[strlen(directory) - 1u] == '\\' ? "" : "/",
        local_now.tm_year + 1900, local_now.tm_mon + 1, local_now.tm_mday,
        local_now.tm_hour, local_now.tm_min, local_now.tm_sec,
        suffix == 0u ? "" : "-");
    if (suffix != 0u && written >= 0 && (size_t)written < capacity) {
        written = snprintf(output + written, capacity - (size_t)written,
                           "%u", suffix);
    }
    return written >= 0 && (size_t)written < capacity;
}

wz_telnet_screenshot_result_t wz_telnet_screenshot_save(
    const wz_presentation_snapshot_t* snapshot,
    char* path, size_t path_capacity)
{
    const wz_raster_buffer_t* raster;
    size_t required;
    size_t written;
    wz_byte_t* png;
    unsigned suffix;
    if (path == NULL || path_capacity == 0u) return WZ_TELNET_SCREENSHOT_CANNOT_CREATE_FILE;
    path[0] = '\0';
    if (snapshot == NULL || wz_presentation_snapshot_sequence(snapshot) == 0u ||
        (raster = wz_presentation_snapshot_raster(snapshot)) == NULL) {
        return WZ_TELNET_SCREENSHOT_NO_RASTER;
    }
    required = wz_screenshot_png_required_size(raster);
    if (required == 0u) return WZ_TELNET_SCREENSHOT_ENCODE_FAILED;
    png = (wz_byte_t*)malloc(required);
    if (png == NULL) return WZ_TELNET_SCREENSHOT_ENCODE_FAILED;
    if (wz_screenshot_png_encode(raster, png, required, &written) != WZ_RESULT_OK) {
        free(png);
        return WZ_TELNET_SCREENSHOT_ENCODE_FAILED;
    }
    for (suffix = 0u; suffix < 1000u; ++suffix) {
        if (!screenshot_path(path, path_capacity, suffix)) {
            free(png);
            return WZ_TELNET_SCREENSHOT_CANNOT_CREATE_FILE;
        }
        if (wz_host_output_write_exclusive(path, png, written)) {
            free(png);
            return WZ_TELNET_SCREENSHOT_OK;
        }
    }
    free(png);
    path[0] = '\0';
    return WZ_TELNET_SCREENSHOT_WRITE_FAILED;
}

bool wz_telnet_screenshot_format_response(
    wz_telnet_screenshot_result_t result, const char* path,
    char* output, size_t output_capacity, size_t* output_length)
{
    const char* reason;
    int written;
    if (output == NULL || output_length == NULL || output_capacity == 0u) return false;
    if (result == WZ_TELNET_SCREENSHOT_OK) {
        if (path == NULL || path[0] == '\0') return false;
        written = snprintf(output, output_capacity, "OK SCREENSHOT %s\r\n", path);
    } else {
        switch (result) {
        case WZ_TELNET_SCREENSHOT_NO_RASTER: reason = "no-raster"; break;
        case WZ_TELNET_SCREENSHOT_CANNOT_CREATE_FILE: reason = "cannot-create-file"; break;
        case WZ_TELNET_SCREENSHOT_ENCODE_FAILED: reason = "encode-failed"; break;
        case WZ_TELNET_SCREENSHOT_WRITE_FAILED: reason = "write-failed"; break;
        default: return false;
        }
        written = snprintf(output, output_capacity, "ERR SCREENSHOT %s\r\n", reason);
    }
    if (written < 0 || (size_t)written >= output_capacity) {
        *output_length = 0u;
        return false;
    }
    *output_length = (size_t)written;
    return true;
}

bool wz_telnet_menu_parse(const char* command)
{
    return command != NULL && strcmp(command, "MENU") == 0;
}

bool wz_telnet_menu_format_root(char* output, size_t output_capacity,
                                size_t* output_length)
{
    size_t used = 0u;
    if (output == NULL || output_length == NULL || output_capacity == 0u) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    for (size_t index = 0u; index < wz_command_registry_menu_root_count(); ++index) {
        const wz_command_menu_root_t* root =
            wz_command_registry_menu_root_at(index);
        int written = snprintf(output + used, output_capacity - used,
                               "MENU %s LABEL=\"%s\"\r\n",
                               root->id, root->label);
        if (written < 0 || (size_t)written >= output_capacity - used) {
            *output_length = 0u;
            return false;
        }
        used += (size_t)written;
    }
    if (used >= output_capacity) {
        *output_length = 0u;
        return false;
    }
    {
        int written = snprintf(output + used, output_capacity - used, "END\r\n");
        if (written < 0 || (size_t)written >= output_capacity - used) {
            *output_length = 0u;
            return false;
        }
        used += (size_t)written;
    }
    *output_length = used;
    return true;
}

static const char* menu_permission_label(wz_command_permission_t permission)
{
    static const char* const labels[] = {
        "REMOTE_SAFE", "HOST_READ", "HOST_WRITE", "MEDIA_DESTRUCTIVE",
        "APPLICATION_CONTROL", "LOCAL_ONLY"
    };
    return permission <= WZ_COMMAND_LOCAL_ONLY ? labels[permission] : "UNKNOWN";
}

static const char* discovery_field(const char* value)
{
    const unsigned char* cursor = (const unsigned char*)value;
    if (value == NULL) return "UNAVAILABLE";
    if (value[0] == '/' || value[0] == '\\' ||
        (isalpha(cursor[0]) && cursor[1] == ':' &&
         (cursor[2] == '/' || cursor[2] == '\\'))) {
        return "[REDACTED]";
    }
    while (*cursor != '\0') {
        if (*cursor == '\r' || *cursor == '\n' ||
            (*cursor == '/' && cursor[1] == '/')) {
            return "[REDACTED]";
        }
        ++cursor;
    }
    return value;
}

static bool menu_tree_append(char* output, size_t capacity, size_t* used,
                             const char* format, ...)
{
    va_list arguments;
    int written;
    if (*used >= capacity) return false;
    va_start(arguments, format);
    written = vsnprintf(output + *used, capacity - *used, format, arguments);
    va_end(arguments);
    if (written < 0 || (size_t)written >= capacity - *used) return false;
    *used += (size_t)written;
    return true;
}

bool wz_telnet_menu_tree_parse(const char* command)
{
    return command != NULL && strcmp(command, "MENU TREE") == 0;
}

bool wz_telnet_menu_tree_format(const wz_command_registry_t* registry,
                                char* output, size_t output_capacity,
                                size_t* output_length)
{
    size_t used = 0u;
    if (registry == NULL || output == NULL || output_length == NULL ||
        output_capacity == 0u || !registry->finalized) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    for (size_t index = 0u; index < wz_command_registry_menu_root_count(); ++index) {
        const wz_command_menu_root_t* root =
            wz_command_registry_menu_root_at(index);
        if (!menu_tree_append(output, output_capacity, &used,
                "ITEM %s PARENT=ROOT TYPE=MENU STATE=ENABLED REMOTE=ALLOWED CLASS=REMOTE_SAFE LABEL=\"%s\"\r\n",
                root->id, root->label)) {
            *output_length = 0u;
            return false;
        }
    }
    for (size_t index = 0u; index < wz_command_registry_count(registry); ++index) {
        const wz_command_metadata_t* metadata =
            wz_command_registry_at(registry, index);
        const char* reason = NULL;
        wz_command_state_t state = wz_command_registry_state(
            registry, metadata->id, &reason);
        const char* parent = metadata->menu_group == NULL ? "ROOT" :
            metadata->menu_group;
        const char* remote = metadata->permission == WZ_COMMAND_REMOTE_SAFE ?
            "ALLOWED" : "DENIED";
        if (!menu_tree_append(output, output_capacity, &used,
                "ITEM %s PARENT=%s TYPE=COMMAND STATE=%s REMOTE=%s CLASS=%s LABEL=\"%s\"",
                metadata->id, discovery_field(parent),
                state == WZ_COMMAND_ENABLED ? "ENABLED" : "DISABLED",
                remote, menu_permission_label(metadata->permission),
                discovery_field(metadata->label))) {
            *output_length = 0u;
            return false;
        }
        if (state == WZ_COMMAND_DISABLED && reason != NULL &&
            !menu_tree_append(output, output_capacity, &used,
                              " REASON=%s", reason)) {
            *output_length = 0u;
            return false;
        }
        if (!menu_tree_append(output, output_capacity, &used, "\r\n")) {
            *output_length = 0u;
            return false;
        }
    }
    if (!menu_tree_append(output, output_capacity, &used, "END\r\n")) {
        *output_length = 0u;
        return false;
    }
    *output_length = used;
    return true;
}

bool wz_telnet_menu_id_parse(const char* command, char* id, size_t id_capacity)
{
    const char* value;
    size_t length;
    if (command == NULL || id == NULL || id_capacity == 0u ||
        strncmp(command, "MENU ", 5u) != 0) {
        return false;
    }
    value = command + 5u;
    length = strlen(value);
    if (length == 0u || length + 1u > id_capacity || strchr(value, ' ') != NULL ||
        strchr(value, '\r') != NULL || strchr(value, '\n') != NULL) {
        return false;
    }
    memcpy(id, value, length + 1u);
    return true;
}

bool wz_telnet_menu_id_format(const wz_command_registry_t* registry,
                              const char* id, char* output,
                              size_t output_capacity, size_t* output_length)
{
    const wz_command_metadata_t* metadata;
    const char* reason = NULL;
    size_t used = 0u;
    if (registry == NULL || id == NULL || output == NULL ||
        output_length == NULL || output_capacity == 0u || !registry->finalized) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    metadata = wz_command_registry_find(registry, id);
    if (metadata != NULL) {
        wz_command_state_t state = wz_command_registry_state(registry, id, &reason);
        const char* remote = metadata->permission == WZ_COMMAND_REMOTE_SAFE ?
            "ALLOWED" : "DENIED";
        if (!menu_tree_append(output, output_capacity, &used,
                "COMMAND %s TYPE=COMMAND STATE=%s REMOTE=%s CLASS=%s LABEL=\"%s\" DESCRIPTION=\"%s\"",
                metadata->id,
                state == WZ_COMMAND_ENABLED ? "ENABLED" : "DISABLED",
                remote, menu_permission_label(metadata->permission),
                discovery_field(metadata->label), discovery_field(metadata->description))) {
            *output_length = 0u;
            return false;
        }
        if (state == WZ_COMMAND_DISABLED && reason != NULL &&
            !menu_tree_append(output, output_capacity, &used, " REASON=%s", reason)) {
            *output_length = 0u;
            return false;
        }
        if (!menu_tree_append(output, output_capacity, &used, "\r\nEND\r\n")) {
            *output_length = 0u;
            return false;
        }
        *output_length = used;
        return true;
    }
    for (size_t index = 0u; index < wz_command_registry_menu_root_count(); ++index) {
        const wz_command_menu_root_t* root =
            wz_command_registry_menu_root_at(index);
        if (strcmp(root->id, id) == 0) {
            if (!menu_tree_append(output, output_capacity, &used,
                    "MENU %s TYPE=MENU LABEL=\"%s\"\r\n", root->id, root->label)) {
                *output_length = 0u;
                return false;
            }
            for (size_t child = 0u; child < wz_command_registry_count(registry); ++child) {
                const wz_command_metadata_t* child_metadata =
                    wz_command_registry_at(registry, child);
                if (child_metadata->menu_group != NULL &&
                    strcmp(child_metadata->menu_group, root->id) == 0) {
                    wz_command_state_t child_state = wz_command_registry_state(
                        registry, child_metadata->id, &reason);
                    if (!menu_tree_append(output, output_capacity, &used,
                            "ITEM %s PARENT=%s TYPE=COMMAND STATE=%s REMOTE=%s CLASS=%s LABEL=\"%s\"\r\n",
                            child_metadata->id, root->id,
                            child_state == WZ_COMMAND_ENABLED ? "ENABLED" : "DISABLED",
                            child_metadata->permission == WZ_COMMAND_REMOTE_SAFE ? "ALLOWED" : "DENIED",
                            menu_permission_label(child_metadata->permission), discovery_field(child_metadata->label))) {
                        *output_length = 0u;
                        return false;
                    }
                    if (child_state == WZ_COMMAND_DISABLED && reason != NULL) {
                        /* The reason is emitted on the record boundary below. */
                        size_t line_end = used - 2u;
                        used = line_end;
                        if (!menu_tree_append(output, output_capacity, &used,
                                " REASON=%s\r\n", reason)) {
                            *output_length = 0u;
                            return false;
                        }
                    }
                }
            }
            if (!menu_tree_append(output, output_capacity, &used, "END\r\n")) {
                *output_length = 0u;
                return false;
            }
            *output_length = used;
            return true;
        }
    }
    if (output_capacity < sizeof("ERR BAD_COMMAND_ID\r\n")) {
        *output_length = 0u;
        return false;
    }
    (void)snprintf(output, output_capacity, "ERR BAD_COMMAND_ID\r\n");
    *output_length = strlen("ERR BAD_COMMAND_ID\r\n");
    return true;
}

static bool menu_contains_insensitive(const char* value, const char* query)
{
    size_t query_length;
    if (value == NULL || query == NULL) return false;
    query_length = strlen(query);
    if (query_length == 0u) return true;
    for (; *value != '\0'; ++value) {
        const char* left = value;
        const char* right = query;
        while (*left != '\0' && *right != '\0' &&
               tolower((unsigned char)*left) == tolower((unsigned char)*right)) {
            ++left;
            ++right;
        }
        if (*right == '\0') return true;
    }
    return false;
}

bool wz_telnet_menu_find_parse(const char* command, char* text, size_t text_capacity)
{
    const char* value;
    size_t length;
    if (command == NULL || text == NULL || text_capacity == 0u ||
        strncmp(command, "MENU FIND ", 10u) != 0) return false;
    value = command + 10u;
    length = strlen(value);
    if (length == 0u || length + 1u > text_capacity || strchr(value, '\r') != NULL ||
        strchr(value, '\n') != NULL) return false;
    memcpy(text, value, length + 1u);
    return true;
}

bool wz_telnet_menu_find_format(const wz_command_registry_t* registry,
                                const char* text, char* output,
                                size_t output_capacity, size_t* output_length)
{
    size_t used = 0u;
    if (registry == NULL || text == NULL || output == NULL || output_length == NULL ||
        output_capacity == 0u || text[0] == '\0' || !registry->finalized) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    for (size_t index = 0u; index < wz_command_registry_count(registry); ++index) {
        const wz_command_metadata_t* metadata = wz_command_registry_at(registry, index);
        if (menu_contains_insensitive(metadata->id, text) ||
            menu_contains_insensitive(metadata->label, text) ||
            menu_contains_insensitive(metadata->description, text)) {
            const char* reason = NULL;
            wz_command_state_t state = wz_command_registry_state(registry, metadata->id, &reason);
            if (!menu_tree_append(output, output_capacity, &used,
                    "ITEM %s STATE=%s REMOTE=%s CLASS=%s LABEL=\"%s\"",
                    metadata->id, state == WZ_COMMAND_ENABLED ? "ENABLED" : "DISABLED",
                    metadata->permission == WZ_COMMAND_REMOTE_SAFE ? "ALLOWED" : "DENIED",
                    menu_permission_label(metadata->permission), discovery_field(metadata->label))) {
                *output_length = 0u;
                return false;
            }
            if (state == WZ_COMMAND_DISABLED && reason != NULL &&
                !menu_tree_append(output, output_capacity, &used, " REASON=%s", reason)) {
                *output_length = 0u;
                return false;
            }
            if (!menu_tree_append(output, output_capacity, &used, "\r\n")) {
                *output_length = 0u;
                return false;
            }
        }
    }
    if (!menu_tree_append(output, output_capacity, &used, "END\r\n")) {
        *output_length = 0u;
        return false;
    }
    *output_length = used;
    return true;
}

bool wz_telnet_describe_parse(const char* command, char* id, size_t id_capacity)
{
    const char* value;
    size_t length;
    if (command == NULL || id == NULL || id_capacity == 0u ||
        strncmp(command, "DESCRIBE ", 9u) != 0) return false;
    value = command + 9u;
    length = strlen(value);
    if (length == 0u || length + 1u > id_capacity || strchr(value, ' ') != NULL ||
        strchr(value, '\r') != NULL || strchr(value, '\n') != NULL) return false;
    memcpy(id, value, length + 1u);
    return true;
}

bool wz_telnet_describe_format(const wz_command_registry_t* registry,
                               const char* id, char* output,
                               size_t output_capacity, size_t* output_length)
{
    const wz_command_metadata_t* metadata;
    const char* reason = NULL;
    size_t used = 0u;
    if (registry == NULL || id == NULL || output == NULL || output_length == NULL ||
        output_capacity == 0u || !registry->finalized) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    metadata = wz_command_registry_find(registry, id);
    if (metadata == NULL) {
        if (!menu_tree_append(output, output_capacity, &used, "ERR BAD_COMMAND_ID\r\n")) {
            *output_length = 0u;
            return false;
        }
        *output_length = used;
        return true;
    }
    if (!menu_tree_append(output, output_capacity, &used,
            "DESCRIBE %s\r\nID=%s\r\nLABEL=\"%s\"\r\nDESCRIPTION=\"%s\"\r\nPARAMETERS=%s\r\n",
            metadata->id, metadata->id, discovery_field(metadata->label),
            discovery_field(metadata->description), discovery_field(
                metadata->parameter_schema == NULL ? "NONE" : metadata->parameter_schema)) ||
        !menu_tree_append(output, output_capacity, &used,
            "ENABLED=%s\r\nREMOTE_CLASS=%s\r\nREMOTE_ALLOWED=%s\r\nAFFECTS_MACHINE_STATE=%s\r\nRECORDABLE=%s\r\n",
            wz_command_registry_state(registry, metadata->id, &reason) == WZ_COMMAND_ENABLED ? "YES" : "NO",
            menu_permission_label(metadata->permission),
            metadata->permission == WZ_COMMAND_REMOTE_SAFE ? "YES" : "NO",
            metadata->affects_machine_state ? "YES" : "NO",
            metadata->recordable ? "YES" : "NO")) {
        *output_length = 0u;
        return false;
    }
    if (reason != NULL && !menu_tree_append(output, output_capacity, &used,
            "DISABLED_REASON=%s\r\n", reason)) {
        *output_length = 0u;
        return false;
    }
    if (!menu_tree_append(output, output_capacity, &used, "END\r\n")) {
        *output_length = 0u;
        return false;
    }
    *output_length = used;
    return true;
}

bool wz_telnet_do_parse(const char* command, char* id, size_t id_capacity,
                        char* arguments, size_t arguments_capacity)
{
    const char* value;
    const char* separator;
    size_t id_length;
    size_t argument_length;
    if (command == NULL || id == NULL || arguments == NULL || id_capacity == 0u ||
        arguments_capacity == 0u || strncmp(command, "DO ", 3u) != 0) return false;
    value = command + 3u;
    separator = strchr(value, ' ');
    id_length = separator == NULL ? strlen(value) : (size_t)(separator - value);
    if (id_length == 0u || id_length + 1u > id_capacity) return false;
    memcpy(id, value, id_length);
    id[id_length] = '\0';
    if (separator == NULL) {
        arguments[0] = '\0';
        return true;
    }
    while (*separator == ' ') ++separator;
    argument_length = strlen(separator);
    if (argument_length + 1u > arguments_capacity || strchr(separator, '\r') != NULL ||
        strchr(separator, '\n') != NULL) return false;
    memcpy(arguments, separator, argument_length + 1u);
    return true;
}

static bool telnet_do_result_fields_valid(const char* fields)
{
    const unsigned char* cursor = (const unsigned char*)fields;
    if (fields == NULL || *fields == '\0') return true;
    while (*cursor != '\0') {
        if (*cursor < 0x20u || *cursor > 0x7eu) return false;
        ++cursor;
    }
    return true;
}

bool wz_telnet_alias_to_do(const char* alias, char* output,
                           size_t output_capacity)
{
    wz_telnet_speed_command_t speed;
    const char* speed_value = NULL;
    int written;
    if (alias == NULL || output == NULL || output_capacity == 0u) return false;
    if (strcmp(alias, "RESET") == 0) {
        written = snprintf(output, output_capacity, "DO machine.reset");
    } else if (strcmp(alias, "SCREENSHOT") == 0) {
        written = snprintf(output, output_capacity, "DO host.screenshot.temp");
    } else if (wz_telnet_speed_parse(alias, &speed) &&
               speed != WZ_TELNET_SPEED_BAD) {
        switch (speed) {
        case WZ_TELNET_SPEED_25: speed_value = "25"; break;
        case WZ_TELNET_SPEED_50: speed_value = "50"; break;
        case WZ_TELNET_SPEED_100: speed_value = "100"; break;
        case WZ_TELNET_SPEED_200: speed_value = "200"; break;
        case WZ_TELNET_SPEED_400: speed_value = "400"; break;
        case WZ_TELNET_SPEED_800: speed_value = "800"; break;
        case WZ_TELNET_SPEED_UNLIMITED: speed_value = "UNLIMITED"; break;
        default: return false;
        }
        written = snprintf(output, output_capacity, "DO machine.speed.set %s",
                           speed_value);
    } else {
        return false;
    }
    return written >= 0 && (size_t)written < output_capacity;
}

bool wz_telnet_do_format(const wz_command_registry_t* registry,
                         const char* id, const char* arguments,
                         char* output, size_t output_capacity,
                         size_t* output_length)
{
    const wz_command_metadata_t* metadata;
    const char* reason = NULL;
    wz_command_result_t result;
    wz_command_arguments_t command_arguments;
    wz_command_permission_t remote_permission;
    size_t used = 0u;
    if (registry == NULL || id == NULL || arguments == NULL || output == NULL ||
        output_length == NULL || output_capacity == 0u || !registry->finalized) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    metadata = wz_command_registry_find(registry, id);
    if (metadata == NULL) {
        return menu_tree_append(output, output_capacity, &used,
                                "ERR BAD_COMMAND_ID\r\n") &&
            (*output_length = used, true);
    }
    if (wz_command_registry_state(registry, id, &reason) == WZ_COMMAND_DISABLED) {
        if (!menu_tree_append(output, output_capacity, &used,
                "ERR BAD_STATE %s %s\r\n", id,
                reason == NULL ? "command-unavailable" : reason)) {
            *output_length = 0u;
            return false;
        }
        *output_length = used;
        return true;
    }
    command_arguments.data = arguments;
    command_arguments.size = strlen(arguments);
    remote_permission = wz_command_registry_remote_permission(
        registry, id, command_arguments);
    if (!wz_command_permission_is_remote_safe(remote_permission)) {
        if (!menu_tree_append(output, output_capacity, &used,
                "DENIED %s %s\r\n", id, menu_permission_label(remote_permission))) {
            *output_length = 0u;
            return false;
        }
        *output_length = used;
        return true;
    }
    if (wz_command_registry_dispatch(registry, id, command_arguments, &result) != WZ_RESULT_OK) {
        if (!menu_tree_append(output, output_capacity, &used,
                "ERR DO %s %s\r\n", id,
                result.reason == NULL ? "dispatch-failed" : result.reason)) {
            *output_length = 0u;
            return false;
        }
        *output_length = used;
        return true;
    }
    if (!telnet_do_result_fields_valid(result.message)) {
        *output_length = 0u;
        return false;
    }
    if (result.message[0] != '\0' &&
        !menu_tree_append(output, output_capacity, &used, "OK DO %s %s\r\n",
                          id, result.message)) {
        *output_length = 0u;
        return false;
    }
    if (result.message[0] == '\0' &&
        !menu_tree_append(output, output_capacity, &used, "OK DO %s\r\n", id)) {
        *output_length = 0u;
        return false;
    }
    *output_length = used;
    return true;
}
