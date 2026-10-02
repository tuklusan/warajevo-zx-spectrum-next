/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_microdrive_manager.h"

#include <stdlib.h>
#include <string.h>

static wz_byte_t checksum(const wz_byte_t* data, size_t length)
{
    unsigned sum = 0u;
    size_t index;
    for (index = 0u; index < length; ++index) sum += data[index];
    return (wz_byte_t)(sum % 255u);
}

static bool valid_sector(const wz_byte_t* sector)
{
    const wz_byte_t* header = sector + WZ_MDR_IMAGE_HEADER_OFFSET;
    const wz_byte_t* descriptor = sector + WZ_MDR_IMAGE_DATA_OFFSET;
    return (header[0] & 1u) != 0u &&
        header[WZ_MDR_HEADER_SIZE - 1u] == checksum(header, 14u) &&
        (descriptor[0] & 1u) == 0u &&
        descriptor[14] == checksum(descriptor, 14u) &&
        sector[WZ_MDR_SECTOR_SIZE - 1u] ==
            checksum(descriptor + 15u, 512u);
}

static bool blank_name(const wz_byte_t* name)
{
    size_t index;
    for (index = 0u; index < 10u; ++index) {
        if (name[index] != ' ' && name[index] != 0u) return false;
    }
    return true;
}

static bool manager_name(const char* name)
{
    size_t index, length;
    if (name == NULL) return false;
    length = strlen(name);
    if (length == 0u || length > 10u) return false;
    for (index = 0u; index < length; ++index) {
        unsigned char value = (unsigned char)name[index];
        if (value < 0x20u || value > 0x7eu || value == '"') return false;
    }
    return true;
}

static void store_name(wz_byte_t* target, const char* name)
{
    size_t length = strlen(name);
    memset(target, ' ', 10u);
    memcpy(target, name, length);
}

static bool same_name(const char* left, const wz_byte_t* right)
{
    size_t index, left_length = strlen(left);
    for (index = 0u; index < 10u; ++index) {
        unsigned char a = index < left_length
            ? (unsigned char)left[index] : ' ';
        unsigned char b = right[index];
        if (a == 0u) a = ' ';
        if (b == 0u) b = ' ';
        if (a >= 'A' && a <= 'Z') a = (unsigned char)(a + ('a' - 'A'));
        if (b >= 'A' && b <= 'Z') b = (unsigned char)(b + ('a' - 'A'));
        if (a != b) return false;
    }
    return true;
}

static void update_checksums(wz_byte_t* sector)
{
    wz_byte_t* header = sector + WZ_MDR_IMAGE_HEADER_OFFSET;
    wz_byte_t* descriptor = sector + WZ_MDR_IMAGE_DATA_OFFSET;
    header[WZ_MDR_HEADER_SIZE - 1u] = checksum(header, 14u);
    descriptor[14] = checksum(descriptor, 14u);
    sector[WZ_MDR_SECTOR_SIZE - 1u] =
        checksum(descriptor + 15u, 512u);
}

static bool image_shape(const wz_byte_t* data, size_t length)
{
    size_t count;
    if (data == NULL || length % WZ_MDR_SECTOR_SIZE != 0u) return false;
    count = length / WZ_MDR_SECTOR_SIZE;
    return count >= WZ_MDR_MIN_SECTORS && count <= WZ_MDR_MAX_SECTORS;
}

static bool validate_all(const wz_byte_t* data, size_t length)
{
    size_t index, count;
    if (!image_shape(data, length)) return false;
    count = length / WZ_MDR_SECTOR_SIZE;
    for (index = 0u; index < count; ++index) {
        if (!valid_sector(data + index * WZ_MDR_SECTOR_SIZE)) return false;
    }
    return true;
}

static size_t read_length(const wz_byte_t* descriptor)
{
    return (size_t)descriptor[2] | ((size_t)descriptor[3] << 8u);
}

static void write_length(wz_byte_t* descriptor, size_t length)
{
    descriptor[2] = (wz_byte_t)(length & 0xffu);
    descriptor[3] = (wz_byte_t)((length >> 8u) & 0xffu);
}

static wz_microdrive_manager_file_t* find_file(
    wz_microdrive_manager_file_t* files, size_t count,
    const wz_byte_t* name)
{
    size_t index;
    for (index = 0u; index < count; ++index) {
        if (same_name(files[index].name, name)) return &files[index];
    }
    return NULL;
}

bool wz_microdrive_manager_validate(const wz_mdr_image_t* image)
{
    return image != NULL && image->data != NULL &&
        image_shape(image->data, image->length) &&
        image->sector_count == image->length / WZ_MDR_SECTOR_SIZE &&
        validate_all(image->data, image->length);
}

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
     "drive-selector+file-selector+drive-selector", WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false},
    {WZ_MICRODRIVE_MANAGER_SECTOR_VIEW, "media.microdrive.sector.view", "View Sector",
     "Inspect one complete 543-byte sector", "drive,sector", "drive-selector+sector-selector",
     WZ_COMMAND_REMOTE_SAFE, false, false, true},
    {WZ_MICRODRIVE_MANAGER_SECTOR_VERIFY, "media.microdrive.sector.verify", "Verify Sector",
     "Validate sector structure and checksums", "drive,sector", "drive-selector+sector-selector",
     WZ_COMMAND_REMOTE_SAFE, false, false, true},
    {WZ_MICRODRIVE_MANAGER_SECTOR_REPAIR, "media.microdrive.sector.repair", "Repair Sector",
     "Adjust a sector with explicit confirmation", "drive,sector", "drive-selector+sector-selector+confirm",
     WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false},
    {WZ_MICRODRIVE_MANAGER_SECTOR_EDIT_DATA, "media.microdrive.sector.edit_data", "Edit Sector Data",
     "Edit ordinary logical sector data", "drive,sector,data", "drive-selector+sector-selector+data-editor",
     WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false},
    {WZ_MICRODRIVE_MANAGER_SECTOR_EDIT_RAW, "media.microdrive.sector.edit_raw", "Edit Whole Sector (Dangerous)",
     "Edit all 543 bytes including metadata and checksums", "drive,sector,bytes",
     "drive-selector+sector-selector+raw-editor+confirm", WZ_COMMAND_MEDIA_DESTRUCTIVE, true, false, false}
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
            operation->recordable,
            NULL
        };
        if (wz_command_registry_register(registry, metadata) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_microdrive_manager_catalog(
    const wz_mdr_image_t* image, wz_microdrive_manager_file_t* files,
    size_t file_capacity, size_t* file_count,
    wz_microdrive_manager_allocation_t* allocation)
{
    size_t sector_index, count = 0u;
    wz_microdrive_manager_allocation_t summary = {0u, 0u, 0u, 0u};
    if (image == NULL || file_count == NULL || allocation == NULL ||
        (file_capacity != 0u && files == NULL) || image->data == NULL ||
        !image_shape(image->data, image->length) ||
        image->sector_count != image->length / WZ_MDR_SECTOR_SIZE) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    summary.total_sectors = image->sector_count;
    for (sector_index = 0u; sector_index < image->sector_count;
         ++sector_index) {
        const wz_byte_t* sector = image->data +
            sector_index * WZ_MDR_SECTOR_SIZE;
        const wz_byte_t* descriptor = sector + WZ_MDR_IMAGE_DATA_OFFSET;
        const wz_byte_t* name = descriptor + 4u;
        size_t length = read_length(descriptor);
        wz_microdrive_manager_file_t* entry;
        if (!valid_sector(sector)) {
            ++summary.damaged_sectors;
            continue;
        }
        if (length == 0u && blank_name(name)) {
            ++summary.free_sectors;
            continue;
        }
        ++summary.allocated_sectors;
        if (blank_name(name)) continue;
        entry = find_file(files, count, name);
        if (entry == NULL) {
            size_t copy_length;
            if (count == file_capacity) return WZ_RESULT_INVALID_STATE;
            entry = &files[count++];
            memset(entry, 0, sizeof(*entry));
            copy_length = 10u;
            while (copy_length != 0u && name[copy_length - 1u] == ' ') {
                --copy_length;
            }
            memcpy(entry->name, name, copy_length);
            entry->name[copy_length] = '\0';
        }
        ++entry->sector_count;
        if (length > 512u || entry->byte_count > SIZE_MAX - length) {
            return WZ_RESULT_INVALID_STATE;
        }
        entry->byte_count += length;
    }
    *file_count = count;
    *allocation = summary;
    return WZ_RESULT_OK;
}

wz_result_t wz_microdrive_manager_format(wz_byte_t* data, size_t length,
                                         const char* name)
{
    size_t index, count;
    if (!image_shape(data, length) || !manager_name(name)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    count = length / WZ_MDR_SECTOR_SIZE;
    for (index = 0u; index < count; ++index) {
        wz_byte_t* sector = data + index * WZ_MDR_SECTOR_SIZE;
        wz_byte_t* header = sector + WZ_MDR_IMAGE_HEADER_OFFSET;
        wz_byte_t* descriptor = sector + WZ_MDR_IMAGE_DATA_OFFSET;
        memset(sector, 0, WZ_MDR_SECTOR_SIZE);
        header[0] = 1u;
        header[1] = (wz_byte_t)(count - index);
        store_name(header + 4u, name);
        descriptor[0] = 0u;
        descriptor[1] = 0u;
        write_length(descriptor, 0u);
        store_name(descriptor + 4u, "");
        update_checksums(sector);
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_microdrive_manager_rename(wz_byte_t* data, size_t length,
                                         const char* name)
{
    size_t index, count;
    if (!validate_all(data, length) || !manager_name(name)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    count = length / WZ_MDR_SECTOR_SIZE;
    for (index = 0u; index < count; ++index) {
        wz_byte_t* sector = data + index * WZ_MDR_SECTOR_SIZE;
        wz_byte_t* header = sector + WZ_MDR_IMAGE_HEADER_OFFSET;
        store_name(header + 4u, name);
        header[WZ_MDR_HEADER_SIZE - 1u] = checksum(header, 14u);
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_microdrive_manager_optimize(wz_byte_t* data, size_t length)
{
    size_t count, left;
    wz_byte_t temporary[WZ_MDR_SECTOR_SIZE];
    if (!validate_all(data, length)) return WZ_RESULT_INVALID_ARGUMENT;
    count = length / WZ_MDR_SECTOR_SIZE;
    for (left = 0u; left < count; ++left) {
        size_t best = left, right;
        for (right = left + 1u; right < count; ++right) {
            const size_t number_offset = WZ_MDR_IMAGE_HEADER_OFFSET + 1u;
            if (data[right * WZ_MDR_SECTOR_SIZE + number_offset] <
                data[best * WZ_MDR_SECTOR_SIZE + number_offset]) best = right;
        }
        if (best != left) {
            wz_byte_t* left_sector = data + left * WZ_MDR_SECTOR_SIZE;
            wz_byte_t* best_sector = data + best * WZ_MDR_SECTOR_SIZE;
            memcpy(temporary, left_sector, sizeof(temporary));
            memcpy(left_sector, best_sector, sizeof(temporary));
            memcpy(best_sector, temporary, sizeof(temporary));
        }
    }
    return WZ_RESULT_OK;
}
