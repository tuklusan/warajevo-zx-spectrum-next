/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_COMMAND_REGISTRY_H
#define WZ_APP_WZ_COMMAND_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>

#include "core/wz_types.h"

#define WZ_COMMAND_REASON_CAPACITY 48u
#define WZ_COMMAND_MESSAGE_CAPACITY 128u

typedef enum {
    WZ_COMMAND_REMOTE_SAFE = 0,
    WZ_COMMAND_HOST_READ,
    WZ_COMMAND_HOST_WRITE,
    WZ_COMMAND_MEDIA_DESTRUCTIVE,
    WZ_COMMAND_APPLICATION_CONTROL,
    WZ_COMMAND_LOCAL_ONLY
} wz_command_permission_t;

typedef enum {
    WZ_COMMAND_ENABLED = 0,
    WZ_COMMAND_DISABLED
} wz_command_state_t;

typedef enum {
    WZ_COMMAND_RESULT_SUCCESS = 0,
    WZ_COMMAND_RESULT_REJECTED,
    WZ_COMMAND_RESULT_UNAVAILABLE,
    WZ_COMMAND_RESULT_FAILED
} wz_command_result_status_t;

typedef struct {
    const void* data;
    size_t size;
} wz_command_arguments_t;

typedef struct {
    wz_command_result_status_t status;
    wz_result_t result;
    const char* reason;
    char message[WZ_COMMAND_MESSAGE_CAPACITY];
} wz_command_result_t;

typedef bool (*wz_command_availability_fn)(
    const void* context,
    const char** reason);

typedef wz_result_t (*wz_command_handler_fn)(
    const void* context,
    wz_command_arguments_t arguments,
    wz_command_result_t* result);

typedef struct {
    const char* id;
    const char* label;
    const char* description;
    const char* menu_group;
    const char* parameter_schema;
    const char* result_schema;
    const char* handler_identity;
    const char* parameter_acquisition;
    const char* keyboard_shortcut;
    wz_command_permission_t permission;
    wz_command_availability_fn availability;
    wz_command_handler_fn handler;
    const void* handler_context;
    bool affects_machine_state;
    bool recordable;
} wz_command_metadata_t;

typedef struct {
    wz_command_metadata_t* storage;
    size_t capacity;
    size_t count;
    bool finalized;
} wz_command_registry_t;

wz_result_t wz_command_registry_init(
    wz_command_registry_t* registry,
    wz_command_metadata_t* storage,
    size_t capacity);
wz_result_t wz_command_registry_register(
    wz_command_registry_t* registry,
    wz_command_metadata_t metadata);
wz_result_t wz_command_registry_finalize(wz_command_registry_t* registry);
const wz_command_metadata_t* wz_command_registry_find(
    const wz_command_registry_t* registry,
    const char* id);
size_t wz_command_registry_count(const wz_command_registry_t* registry);
const wz_command_metadata_t* wz_command_registry_at(
    const wz_command_registry_t* registry,
    size_t index);
wz_result_t wz_command_registry_dispatch(
    const wz_command_registry_t* registry,
    const char* id,
    wz_command_arguments_t arguments,
    wz_command_result_t* result);

#endif
