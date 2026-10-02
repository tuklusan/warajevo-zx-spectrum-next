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
        (descriptor[0] & 0xf9u) == 0u &&
        ((size_t)descriptor[2] | ((size_t)descriptor[3] << 8u)) <= 512u &&
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

static size_t read_length(const wz_byte_t* descriptor);

static bool same_name(const char* left, const wz_byte_t* right);

static bool manager_name(const char* name);

static bool same_text_name(const char* left, const char* right)
{
    wz_byte_t padded[10];
    size_t length;
    if (!manager_name(left) || !manager_name(right)) return false;
    length = strlen(right);
    memset(padded, ' ', sizeof(padded));
    memcpy(padded, right, length);
    return same_name(left, padded);
}

static bool descriptor_hidden(const wz_byte_t* descriptor)
{
    size_t index;
    if (descriptor[4] != 0u) return false;
    for (index = 0u; index < 9u; ++index) {
        if (descriptor[5u + index] != ' ' && descriptor[5u + index] != 0u)
            return true;
    }
    return false;
}

static const wz_byte_t* logical_name(const wz_byte_t* descriptor)
{
    return descriptor + (descriptor_hidden(descriptor) ? 5u : 4u);
}

static bool blank_logical_name(const wz_byte_t* descriptor)
{
    size_t index;
    const size_t start = descriptor_hidden(descriptor) ? 5u : 4u;
    const size_t length = descriptor_hidden(descriptor) ? 9u : 10u;
    for (index = 0u; index < length; ++index) {
        if (descriptor[start + index] != ' ' &&
            descriptor[start + index] != 0u) return false;
    }
    return true;
}

static bool blank_record(const wz_byte_t* descriptor)
{
    return read_length(descriptor) == 0u && blank_name(descriptor + 4u);
}

static bool file_name_equal(const wz_byte_t* descriptor, const char* name)
{
    wz_byte_t normalized[10];
    const size_t length = descriptor_hidden(descriptor) ? 9u : 10u;
    if (blank_logical_name(descriptor)) return false;
    memset(normalized, ' ', sizeof(normalized));
    memcpy(normalized, logical_name(descriptor), length);
    return same_name(name, normalized);
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
    if (length > 10u) length = 10u;
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
        const wz_byte_t* name = logical_name(descriptor);
        size_t length = read_length(descriptor);
        wz_microdrive_manager_file_t* entry;
        if (!valid_sector(sector)) {
            ++summary.damaged_sectors;
            continue;
        }
        if (blank_record(descriptor)) {
            ++summary.free_sectors;
            continue;
        }
        ++summary.allocated_sectors;
        if (blank_logical_name(descriptor)) continue;
        entry = find_file(files, count, name);
        if (entry == NULL) {
            size_t copy_length;
            if (count == file_capacity) return WZ_RESULT_INVALID_STATE;
            entry = &files[count++];
            memset(entry, 0, sizeof(*entry));
            copy_length = descriptor_hidden(descriptor) ? 9u : 10u;
            while (copy_length != 0u && name[copy_length - 1u] == ' ') {
                --copy_length;
            }
            memcpy(entry->name, name, copy_length);
            entry->name[copy_length] = '\0';
            entry->hidden = descriptor_hidden(descriptor);
        } else if (entry->hidden != descriptor_hidden(descriptor)) {
            return WZ_RESULT_INVALID_STATE;
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

static size_t matching_sector_count(const wz_byte_t* data, size_t count,
                                    const char* name)
{
    size_t index, matches = 0u;
    for (index = 0u; index < count; ++index) {
        const wz_byte_t* descriptor = data +
            index * WZ_MDR_SECTOR_SIZE + WZ_MDR_IMAGE_DATA_OFFSET;
        if (file_name_equal(descriptor, name)) ++matches;
    }
    return matches;
}

static bool has_named_file(const wz_byte_t* data, size_t count,
                           const char* name)
{
    return matching_sector_count(data, count, name) != 0u;
}

wz_result_t wz_microdrive_manager_file_delete(
    wz_byte_t* data, size_t length, const char* name)
{
    size_t index, count;
    bool found = false;
    if (!validate_all(data, length) || !manager_name(name))
        return WZ_RESULT_INVALID_ARGUMENT;
    count = length / WZ_MDR_SECTOR_SIZE;
    if (!has_named_file(data, count, name)) return WZ_RESULT_INVALID_STATE;
    for (index = 0u; index < count; ++index) {
        wz_byte_t* sector = data + index * WZ_MDR_SECTOR_SIZE;
        wz_byte_t* descriptor = sector + WZ_MDR_IMAGE_DATA_OFFSET;
        if (!file_name_equal(descriptor, name)) continue;
        found = true;
        descriptor[0] = 2u;
        descriptor[1] = 0u;
        write_length(descriptor, 0u);
        store_name(descriptor + 4u, "");
        memset(descriptor + 15u, 0, 512u);
        update_checksums(sector);
    }
    return found ? WZ_RESULT_OK : WZ_RESULT_INVALID_STATE;
}

wz_result_t wz_microdrive_manager_file_rename(
    wz_byte_t* data, size_t length, const char* old_name,
    const char* new_name)
{
    size_t index, count, matches;
    if (!validate_all(data, length) || !manager_name(old_name) ||
        !manager_name(new_name)) return WZ_RESULT_INVALID_ARGUMENT;
    count = length / WZ_MDR_SECTOR_SIZE;
    matches = matching_sector_count(data, count, old_name);
    if (matches == 0u) return WZ_RESULT_INVALID_STATE;
    if (!same_text_name(old_name, new_name) &&
        has_named_file(data, count, new_name)) return WZ_RESULT_INVALID_STATE;
    if (strlen(new_name) > 9u) {
        for (index = 0u; index < count; ++index) {
            const wz_byte_t* descriptor = data +
                index * WZ_MDR_SECTOR_SIZE + WZ_MDR_IMAGE_DATA_OFFSET;
            if (descriptor_hidden(descriptor) &&
                file_name_equal(descriptor, old_name))
                return WZ_RESULT_INVALID_ARGUMENT;
        }
    }
    for (index = 0u; index < count; ++index) {
        wz_byte_t* sector = data + index * WZ_MDR_SECTOR_SIZE;
        wz_byte_t* descriptor = sector + WZ_MDR_IMAGE_DATA_OFFSET;
        const bool hidden = descriptor_hidden(descriptor);
        if (!file_name_equal(descriptor, old_name)) continue;
        if (hidden) {
            descriptor[4] = 0u;
            memset(descriptor + 5u, ' ', 9u);
            memcpy(descriptor + 5u, new_name, strlen(new_name));
        } else {
            store_name(descriptor + 4u, new_name);
        }
        update_checksums(sector);
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_microdrive_manager_file_set_hidden(
    wz_byte_t* data, size_t length, const char* name, bool hidden)
{
    size_t index, count;
    bool found = false;
    if (!validate_all(data, length) || !manager_name(name))
        return WZ_RESULT_INVALID_ARGUMENT;
    if (hidden && strlen(name) > 9u) return WZ_RESULT_INVALID_ARGUMENT;
    count = length / WZ_MDR_SECTOR_SIZE;
    if (!has_named_file(data, count, name)) return WZ_RESULT_INVALID_STATE;
    for (index = 0u; index < count; ++index) {
        wz_byte_t* sector = data + index * WZ_MDR_SECTOR_SIZE;
        wz_byte_t* descriptor = sector + WZ_MDR_IMAGE_DATA_OFFSET;
        const bool was_hidden = descriptor_hidden(descriptor);
        if (!file_name_equal(descriptor, name)) continue;
        found = true;
        if (was_hidden == hidden) continue;
        if (hidden) {
            memmove(descriptor + 5u, descriptor + 4u, 9u);
            descriptor[4] = 0u;
        } else {
            memmove(descriptor + 4u, descriptor + 5u, 9u);
            descriptor[13] = ' ';
        }
        update_checksums(sector);
    }
    return found ? WZ_RESULT_OK : WZ_RESULT_INVALID_STATE;
}

wz_result_t wz_microdrive_manager_file_copy(
    const wz_byte_t* source, size_t source_length, const char* name,
    wz_byte_t* destination, size_t destination_length)
{
    size_t source_count, destination_count, index, matches = 0u, target_count = 0u;
    size_t ordered[WZ_MDR_MAX_SECTORS];
    size_t targets[WZ_MDR_MAX_SECTORS];
    wz_byte_t* records;
    if (source == destination) return WZ_RESULT_INVALID_ARGUMENT;
    if (!validate_all(source, source_length) ||
        !validate_all(destination, destination_length) || !manager_name(name))
        return WZ_RESULT_INVALID_ARGUMENT;
    source_count = source_length / WZ_MDR_SECTOR_SIZE;
    destination_count = destination_length / WZ_MDR_SECTOR_SIZE;
    memset(ordered, 0, sizeof(ordered));
    if (!has_named_file(source, source_count, name) ||
        has_named_file(destination, destination_count, name))
        return WZ_RESULT_INVALID_STATE;
    for (index = 0u; index < source_count; ++index) {
        const wz_byte_t* descriptor = source +
            index * WZ_MDR_SECTOR_SIZE + WZ_MDR_IMAGE_DATA_OFFSET;
        if (file_name_equal(descriptor, name)) {
            const size_t sequence = descriptor[1];
            if (sequence >= WZ_MDR_MAX_SECTORS || ordered[sequence] != 0u)
                return WZ_RESULT_INVALID_STATE;
            ordered[sequence] = index + 1u;
            ++matches;
        }
    }
    if (matches == 0u) return WZ_RESULT_INVALID_STATE;
    for (index = 0u; index < matches; ++index) {
        const size_t source_index = ordered[index] == 0u
            ? SIZE_MAX : ordered[index] - 1u;
        const wz_byte_t* descriptor;
        if (source_index == SIZE_MAX) return WZ_RESULT_INVALID_STATE;
        descriptor = source + source_index * WZ_MDR_SECTOR_SIZE +
            WZ_MDR_IMAGE_DATA_OFFSET;
        if (((descriptor[0] & 2u) != 0u) != (index + 1u == matches))
            return WZ_RESULT_INVALID_STATE;
    }
    for (index = 0u; index < destination_count && target_count < matches; ++index) {
        const wz_byte_t* descriptor = destination +
            index * WZ_MDR_SECTOR_SIZE + WZ_MDR_IMAGE_DATA_OFFSET;
        if (blank_record(descriptor)) targets[target_count++] = index;
    }
    if (target_count != matches) return WZ_RESULT_INVALID_STATE;
    records = (wz_byte_t*)malloc(matches * WZ_MDR_SECTOR_SIZE);
    if (records == NULL) return WZ_RESULT_INVALID_STATE;
    for (index = 0u; index < matches; ++index) {
        const size_t source_index = ordered[index] - 1u;
        memcpy(records + index * WZ_MDR_SECTOR_SIZE,
            source + source_index * WZ_MDR_SECTOR_SIZE,
            WZ_MDR_SECTOR_SIZE);
    }
    for (index = 0u; index < matches; ++index) {
        wz_byte_t* target = destination + targets[index] * WZ_MDR_SECTOR_SIZE;
        const wz_byte_t* record = records + index * WZ_MDR_SECTOR_SIZE;
        memcpy(target + WZ_MDR_IMAGE_DATA_OFFSET,
            record + WZ_MDR_IMAGE_DATA_OFFSET,
            WZ_MDR_SECTOR_SIZE - WZ_MDR_IMAGE_DATA_OFFSET);
        target[WZ_MDR_IMAGE_DATA_OFFSET + 1u] = (wz_byte_t)index;
        target[WZ_MDR_IMAGE_DATA_OFFSET] &= (wz_byte_t)~2u;
        if (index + 1u == matches)
            target[WZ_MDR_IMAGE_DATA_OFFSET] |= 2u;
        update_checksums(target);
    }
    free(records);
    return WZ_RESULT_OK;
}
