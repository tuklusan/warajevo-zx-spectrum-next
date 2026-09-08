/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <assert.h>
#include <string.h>

#include "app/wz_microdrive_manager.h"

static bool available(const void* context, const char** reason)
{
    (void)context;
    if (reason != 0) *reason = 0;
    return true;
}

static wz_result_t handle(const void* context, wz_command_arguments_t arguments,
                          wz_command_result_t* result)
{
    (void)context;
    (void)arguments;
    if (result != 0) result->status = WZ_COMMAND_RESULT_SUCCESS;
    return WZ_RESULT_OK;
}

int main(void)
{
    wz_command_metadata_t storage[15];
    wz_command_registry_t registry;
    const wz_microdrive_manager_operation_t* format;
    const wz_microdrive_manager_operation_t* catalog;
    const wz_command_metadata_t* metadata;

    assert(wz_microdrive_manager_operation_count() == 15u);
    format = wz_microdrive_manager_operation_at(WZ_MICRODRIVE_MANAGER_FORMAT);
    catalog = wz_microdrive_manager_operation_at(WZ_MICRODRIVE_MANAGER_CATALOG);
    assert(format != 0 && format->requires_confirmation);
    assert(format->permission == WZ_COMMAND_MEDIA_DESTRUCTIVE);
    assert(catalog != 0 && !catalog->requires_confirmation);
    assert(catalog->permission == WZ_COMMAND_REMOTE_SAFE);
    assert(wz_microdrive_manager_operation_at(15u) == 0);

    assert(wz_command_registry_init(&registry, storage, 15u) == WZ_RESULT_OK);
    assert(wz_microdrive_manager_register_commands(&registry, available, handle, 0) == WZ_RESULT_OK);
    assert(wz_command_registry_finalize(&registry) == WZ_RESULT_OK);
    assert(wz_command_registry_count(&registry) == 15u);
    metadata = wz_command_registry_find(&registry, "media.microdrive.rename");
    assert(metadata != 0);
    assert(strcmp(metadata->menu_group, "media.microdrive.manager") == 0);
    assert(metadata->permission == WZ_COMMAND_MEDIA_DESTRUCTIVE);
    metadata = wz_command_registry_find(&registry, "media.microdrive.file.copy");
    assert(metadata != 0);
    assert(strcmp(metadata->parameter_schema, "source-drive,file,destination-drive") == 0);
    assert(metadata->permission == WZ_COMMAND_MEDIA_DESTRUCTIVE);
    assert(wz_command_registry_dispatch(&registry, "media.microdrive.catalog",
                                        (wz_command_arguments_t){0, 0}, 0) == WZ_RESULT_INVALID_ARGUMENT);
    return 0;
}
