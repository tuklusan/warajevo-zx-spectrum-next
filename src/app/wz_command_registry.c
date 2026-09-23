/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_command_registry.h"
#include "app/wz_host_thread.h"

#include <string.h>

static const wz_command_menu_root_t menu_roots[] = {
    {"file", "File"},
    {"machine", "Machine"},
    {"media", "Media"},
    {"view", "View"},
    {"tools", "Tools"},
    {"settings", "Settings"},
    {"help", "Help"}
};

static bool valid_id(const char* id)
{
    bool segment_has_character = false;
    const unsigned char* cursor = (const unsigned char*)id;

    if (id == 0 || *id == '\0' || *id == '.') {
        return false;
    }
    while (*cursor != '\0') {
        if (*cursor == '.') {
            if (!segment_has_character) {
                return false;
            }
            segment_has_character = false;
        } else if ((*cursor >= 'a' && *cursor <= 'z') ||
                   (*cursor >= '0' && *cursor <= '9') ||
                   *cursor == '_') {
            segment_has_character = true;
        } else {
            return false;
        }
        ++cursor;
    }
    return segment_has_character;
}

static bool valid_permission(wz_command_permission_t permission)
{
    return permission >= WZ_COMMAND_REMOTE_SAFE &&
           permission <= WZ_COMMAND_LOCAL_ONLY;
}

bool wz_command_permission_is_remote_safe(wz_command_permission_t permission)
{
    return permission == WZ_COMMAND_REMOTE_SAFE;
}

static void initialize_result(wz_command_result_t* result)
{
    result->status = WZ_COMMAND_RESULT_FAILED;
    result->result = WZ_RESULT_INVALID_ARGUMENT;
    result->reason = 0;
    result->message[0] = '\0';
}

static void set_rejection(
    wz_command_result_t* result,
    wz_command_result_status_t status,
    wz_result_t code,
    const char* reason)
{
    result->status = status;
    result->result = code;
    result->reason = reason;
}

wz_result_t wz_command_registry_init(
    wz_command_registry_t* registry,
    wz_command_metadata_t* storage,
    size_t capacity)
{
    if (registry == 0 || storage == 0 || capacity == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    registry->storage = storage;
    registry->capacity = capacity;
    registry->count = 0u;
    registry->finalized = false;
    registry->owner_thread_bound = false;
    return WZ_RESULT_OK;
}

wz_result_t wz_command_registry_bind_owner_thread(
    wz_command_registry_t* registry)
{
    wz_host_thread_id_t current_thread_id;
    if (registry == 0 || registry->storage == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (registry->finalized) return WZ_RESULT_INVALID_STATE;
    current_thread_id = wz_host_thread_current_id();
    if (registry->owner_thread_bound &&
        !wz_host_thread_id_equal(registry->owner_thread_id,
                                 current_thread_id)) {
        return WZ_RESULT_INVALID_STATE;
    }
    registry->owner_thread_id = current_thread_id;
    registry->owner_thread_bound = true;
    return WZ_RESULT_OK;
}

bool wz_command_registry_is_owner_thread(
    const wz_command_registry_t* registry)
{
    return registry != 0 && registry->owner_thread_bound &&
        wz_host_thread_id_equal(registry->owner_thread_id,
                                wz_host_thread_current_id());
}

wz_result_t wz_command_registry_register(
    wz_command_registry_t* registry,
    wz_command_metadata_t metadata)
{
    if (registry == 0 || registry->storage == 0 || metadata.id == 0 ||
        metadata.label == 0 || metadata.description == 0 ||
        metadata.handler_identity == 0 || metadata.handler == 0 ||
        !valid_id(metadata.id) || !valid_permission(metadata.permission)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (registry->finalized) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (registry->count >= registry->capacity) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    if (wz_command_registry_find(registry, metadata.id) != 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    registry->storage[registry->count] = metadata;
    ++registry->count;
    return WZ_RESULT_OK;
}

wz_result_t wz_command_registry_finalize(wz_command_registry_t* registry)
{
    if (registry == 0 || registry->storage == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    registry->finalized = true;
    return WZ_RESULT_OK;
}

const wz_command_metadata_t* wz_command_registry_find(
    const wz_command_registry_t* registry,
    const char* id)
{
    size_t index;

    if (registry == 0 || registry->storage == 0 || id == 0) {
        return 0;
    }
    for (index = 0u; index < registry->count; ++index) {
        /* Stable command IDs are interned by registry owners; avoid rescanning
         * the bytes on the repeated presentation lookup fast path. */
        if (registry->storage[index].id == id ||
            strcmp(registry->storage[index].id, id) == 0) {
            return &registry->storage[index];
        }
    }
    return 0;
}

size_t wz_command_registry_count(const wz_command_registry_t* registry)
{
    return registry == 0 ? 0u : registry->count;
}

const wz_command_metadata_t* wz_command_registry_at(
    const wz_command_registry_t* registry,
    size_t index)
{
    if (registry == 0 || registry->storage == 0 || index >= registry->count) {
        return 0;
    }
    return &registry->storage[index];
}

wz_command_state_t wz_command_registry_state(
    const wz_command_registry_t* registry,
    const char* id,
    const char** reason)
{
    const wz_command_metadata_t* metadata;
    const char* availability_reason = 0;

    if (reason != 0) {
        *reason = "command-unavailable";
    }
    if (registry == 0 || !registry->finalized || id == 0) {
        if (reason != 0) {
            *reason = "registry-unavailable";
        }
        return WZ_COMMAND_DISABLED;
    }
    metadata = wz_command_registry_find(registry, id);
    if (metadata == 0) {
        if (reason != 0) {
            *reason = "unknown-command";
        }
        return WZ_COMMAND_DISABLED;
    }
    if (metadata->availability != 0 &&
        !metadata->availability(metadata->handler_context, &availability_reason)) {
        if (reason != 0) {
            *reason = availability_reason == 0 ?
                "command-unavailable" : availability_reason;
        }
        return WZ_COMMAND_DISABLED;
    }
    if (reason != 0) {
        *reason = 0;
    }
    return WZ_COMMAND_ENABLED;
}

wz_command_permission_t wz_command_registry_remote_permission(
    const wz_command_registry_t* registry,
    const char* id,
    wz_command_arguments_t arguments)
{
    const wz_command_metadata_t* metadata;

    if (registry == 0 || !registry->finalized || id == 0 ||
        (arguments.size != 0u && arguments.data == 0)) {
        return WZ_COMMAND_LOCAL_ONLY;
    }
    metadata = wz_command_registry_find(registry, id);
    if (metadata == 0) {
        return WZ_COMMAND_LOCAL_ONLY;
    }
    return metadata->remote_permission == 0 ?
        metadata->permission :
        metadata->remote_permission(metadata->handler_context, arguments);
}

wz_result_t wz_command_registry_dispatch(
    const wz_command_registry_t* registry,
    const char* id,
    wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    const wz_command_metadata_t* metadata;
    const char* reason = 0;

    if (result == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    initialize_result(result);
    if (registry == 0 || !registry->finalized || id == 0 ||
        (arguments.size != 0u && arguments.data == 0)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (!wz_command_registry_is_owner_thread(registry)) {
        set_rejection(result, WZ_COMMAND_RESULT_REJECTED,
                      WZ_RESULT_INVALID_STATE, "wrong-thread");
        return WZ_RESULT_INVALID_STATE;
    }
    metadata = wz_command_registry_find(registry, id);
    if (metadata == 0) {
        set_rejection(result, WZ_COMMAND_RESULT_REJECTED,
                      WZ_RESULT_NOT_FOUND, "unknown-command");
        return WZ_RESULT_NOT_FOUND;
    }
    if (metadata->availability != 0 &&
        !metadata->availability(metadata->handler_context, &reason)) {
        set_rejection(result, WZ_COMMAND_RESULT_UNAVAILABLE,
                      WZ_RESULT_INVALID_STATE,
                      reason == 0 ? "command-unavailable" : reason);
        return WZ_RESULT_INVALID_STATE;
    }
    result->result = metadata->handler(
        metadata->handler_context, arguments, result);
    if (result->result == WZ_RESULT_OK) {
        result->status = WZ_COMMAND_RESULT_SUCCESS;
    } else if (result->status == WZ_COMMAND_RESULT_SUCCESS) {
        result->status = WZ_COMMAND_RESULT_FAILED;
    }
    return result->result;
}

size_t wz_command_registry_menu_root_count(void)
{
    return sizeof(menu_roots) / sizeof(menu_roots[0]);
}

const wz_command_menu_root_t* wz_command_registry_menu_root_at(size_t index)
{
    return index < wz_command_registry_menu_root_count() ? &menu_roots[index] : 0;
}
