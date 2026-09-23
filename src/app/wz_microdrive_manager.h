/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_MICRODRIVE_MANAGER_H
#define WZ_APP_WZ_MICRODRIVE_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

#include "app/wz_command_registry.h"

typedef enum {
    WZ_MICRODRIVE_MANAGER_MOUNT = 0,
    WZ_MICRODRIVE_MANAGER_EJECT,
    WZ_MICRODRIVE_MANAGER_SET_DEFAULT,
    WZ_MICRODRIVE_MANAGER_CATALOG,
    WZ_MICRODRIVE_MANAGER_FORMAT,
    WZ_MICRODRIVE_MANAGER_OPTIMIZE,
    WZ_MICRODRIVE_MANAGER_ALLOCATION,
    WZ_MICRODRIVE_MANAGER_RENAME,
    WZ_MICRODRIVE_MANAGER_WRITE_PROTECT,
    WZ_MICRODRIVE_MANAGER_WRITE_UNPROTECT,
    WZ_MICRODRIVE_MANAGER_FILE_DELETE,
    WZ_MICRODRIVE_MANAGER_FILE_RENAME,
    WZ_MICRODRIVE_MANAGER_FILE_HIDE,
    WZ_MICRODRIVE_MANAGER_FILE_UNHIDE,
    WZ_MICRODRIVE_MANAGER_FILE_COPY,
    WZ_MICRODRIVE_MANAGER_SECTOR_VIEW,
    WZ_MICRODRIVE_MANAGER_SECTOR_VERIFY,
    WZ_MICRODRIVE_MANAGER_SECTOR_REPAIR,
    WZ_MICRODRIVE_MANAGER_SECTOR_EDIT_DATA,
    WZ_MICRODRIVE_MANAGER_SECTOR_EDIT_RAW
} wz_microdrive_manager_operation_kind_t;

typedef struct {
    wz_microdrive_manager_operation_kind_t kind;
    const char* command_id;
    const char* label;
    const char* description;
    const char* parameter_schema;
    const char* parameter_acquisition;
    wz_command_permission_t permission;
    bool requires_confirmation;
    bool affects_machine_state;
    bool recordable;
} wz_microdrive_manager_operation_t;

size_t wz_microdrive_manager_operation_count(void);
const wz_microdrive_manager_operation_t*
wz_microdrive_manager_operation_at(size_t index);

/* Registers the complete manager surface without owning media or GUI state. */
wz_result_t wz_microdrive_manager_register_commands(
    wz_command_registry_t* registry,
    wz_command_availability_fn availability,
    wz_command_handler_fn handler,
    const void* context);

#endif
