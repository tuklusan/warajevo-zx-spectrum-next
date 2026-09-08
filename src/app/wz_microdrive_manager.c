/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_microdrive_manager.h"

static const wz_microdrive_manager_operation_t operations[] = {
    {WZ_MICRODRIVE_MANAGER_MOUNT, "media.microdrive.mount", "Mount Microdrive",
     "Mount a cartridge in a selected drive", "drive,path", "file-dialog",
     WZ_COMMAND_HOST_READ, false, false, false},
    {WZ_MICRODRIVE_MANAGER_EJECT, "media.microdrive.eject", "Eject Microdrive",
     "Eject the selected cartridge", "drive", "drive-selector",
     WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false},
    {WZ_MICRODRIVE_MANAGER_SET_DEFAULT, "media.microdrive.set_default", "Set Default Drive",
     "Select the default Microdrive", "drive", "drive-selector",
     WZ_COMMAND_REMOTE_SAFE, false, false, true},
    {WZ_MICRODRIVE_MANAGER_CATALOG, "media.microdrive.catalog", "Catalog Microdrive",
     "Read the logical cartridge directory", "drive", "drive-selector",
     WZ_COMMAND_REMOTE_SAFE, false, false, true},
    {WZ_MICRODRIVE_MANAGER_FORMAT, "media.microdrive.format", "Format Microdrive",
     "Initialize a cartridge with the authentic format", "drive,name", "drive-selector+text",
     WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false},
    {WZ_MICRODRIVE_MANAGER_OPTIMIZE, "media.microdrive.optimize", "Optimize Microdrive",
     "Reorder cartridge sectors without changing logical files", "drive", "drive-selector",
     WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false},
    {WZ_MICRODRIVE_MANAGER_ALLOCATION, "media.microdrive.allocation", "View Sector Allocation",
     "Inspect logical sector allocation", "drive", "drive-selector",
     WZ_COMMAND_REMOTE_SAFE, false, false, true},
    {WZ_MICRODRIVE_MANAGER_RENAME, "media.microdrive.rename", "Rename Cartridge",
     "Change the logical cartridge name", "drive,name", "drive-selector+text",
     WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false},
    {WZ_MICRODRIVE_MANAGER_WRITE_PROTECT, "media.microdrive.write_protect", "Write Protect",
     "Prevent writes to the selected cartridge", "drive", "drive-selector",
     WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false},
    {WZ_MICRODRIVE_MANAGER_WRITE_UNPROTECT, "media.microdrive.write_unprotect", "Write Unprotect",
     "Allow writes to the selected cartridge", "drive", "drive-selector",
     WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false},
    {WZ_MICRODRIVE_MANAGER_FILE_DELETE, "media.microdrive.file.delete", "Delete File",
     "Delete the selected logical file", "drive,file", "drive-selector+file-selector",
     WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false},
    {WZ_MICRODRIVE_MANAGER_FILE_RENAME, "media.microdrive.file.rename", "Rename File",
     "Rename the selected logical file", "drive,file,name", "drive-selector+file-selector+text",
     WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false},
    {WZ_MICRODRIVE_MANAGER_FILE_HIDE, "media.microdrive.file.hide", "Hide File",
     "Hide the selected logical file", "drive,file", "drive-selector+file-selector",
     WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false},
    {WZ_MICRODRIVE_MANAGER_FILE_UNHIDE, "media.microdrive.file.unhide", "Unhide File",
     "Make the selected hidden file visible", "drive,file", "drive-selector+file-selector",
     WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false},
    {WZ_MICRODRIVE_MANAGER_FILE_COPY, "media.microdrive.file.copy", "Copy File",
     "Copy a logical file to another mounted cartridge", "source-drive,file,destination-drive",
     "drive-selector+file-selector+drive-selector", WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false}
};

size_t wz_microdrive_manager_operation_count(void)
{
    return sizeof(operations) / sizeof(operations[0]);
}

const wz_microdrive_manager_operation_t*
wz_microdrive_manager_operation_at(size_t index)
{
    if (index >= wz_microdrive_manager_operation_count()) return 0;
    return &operations[index];
}

wz_result_t wz_microdrive_manager_register_commands(
    wz_command_registry_t* registry,
    wz_command_availability_fn availability,
    wz_command_handler_fn handler,
    const void* context)
{
    size_t index;
    if (registry == 0 || availability == 0 || handler == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (index = 0u; index < wz_microdrive_manager_operation_count(); ++index) {
        const wz_microdrive_manager_operation_t* operation =
            wz_microdrive_manager_operation_at(index);
        wz_command_metadata_t metadata = {
            operation->command_id,
            operation->label,
            operation->description,
            "media.microdrive.manager",
            operation->parameter_schema,
            "wz-command-result",
            "wz_microdrive_manager",
            operation->parameter_acquisition,
            0,
            operation->permission,
            availability,
            handler,
            context,
            operation->affects_machine_state,
            operation->recordable
        };
        if (wz_command_registry_register(registry, metadata) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
    }
    return WZ_RESULT_OK;
}
