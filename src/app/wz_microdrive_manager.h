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
#include "core/wz_microdrive.h"

#define WZ_MICRODRIVE_MANAGER_MAX_FILES WZ_MDR_MAX_SECTORS
#define WZ_MDR_IMAGE_HEADER_OFFSET 0u
#define WZ_MDR_IMAGE_DATA_OFFSET WZ_MDR_HEADER_SIZE

typedef struct {
    char name[11];
    size_t sector_count;
    size_t byte_count;
    bool hidden;
} wz_microdrive_manager_file_t;

typedef struct {
    size_t total_sectors;
    size_t allocated_sectors;
    size_t free_sectors;
    size_t damaged_sectors;
} wz_microdrive_manager_allocation_t;

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

/* Cartridge-image operations stage into caller-owned, mutable image storage. */
wz_result_t wz_microdrive_manager_catalog(
    const wz_mdr_image_t* image, wz_microdrive_manager_file_t* files,
    size_t file_capacity, size_t* file_count,
    wz_microdrive_manager_allocation_t* allocation);
bool wz_microdrive_manager_validate(const wz_mdr_image_t* image);
wz_result_t wz_microdrive_manager_format(wz_byte_t* data, size_t length,
                                         const char* name);
wz_result_t wz_microdrive_manager_rename(wz_byte_t* data, size_t length,
                                         const char* name);
wz_result_t wz_microdrive_manager_optimize(wz_byte_t* data, size_t length);

#endif
