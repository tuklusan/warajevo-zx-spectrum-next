/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_recent_files.h"

#include <string.h>

static const char* const wz_open_run_command = "file.open_run";

void wz_recent_files_init(wz_recent_files_t* recent)
{
    if (recent != NULL) {
        memset(recent, 0, sizeof(*recent));
    }
}

bool wz_recent_files_add(wz_recent_files_t* recent, const char* path)
{
    size_t index;
    size_t length;
    bool existing;

    if (recent == NULL || path == NULL || path[0] == '\0') {
        return false;
    }
    length = strlen(path);
    if (length >= WZ_RECENT_FILE_PATH_CAPACITY) {
        return false;
    }
    for (index = 0u; index < recent->count; ++index) {
        if (strcmp(recent->entries[index].path, path) == 0) {
            break;
        }
    }
    existing = index < recent->count;
    if (existing) {
        memmove(&recent->entries[index], &recent->entries[index + 1u],
                (recent->count - index - 1u) * sizeof(recent->entries[0]));
    } else if (recent->count == WZ_RECENT_FILES_CAPACITY) {
        --recent->count;
    }
    if (recent->count > 0u) {
        memmove(&recent->entries[1], &recent->entries[0],
                recent->count * sizeof(recent->entries[0]));
    }
    (void)memcpy(recent->entries[0].path, path, length + 1u);
    recent->entries[0].command_id = wz_open_run_command;
    if (!existing) {
        ++recent->count;
    }
    return true;
}

bool wz_recent_files_remove(wz_recent_files_t* recent, size_t index)
{
    if (recent == NULL || index >= recent->count) {
        return false;
    }
    memmove(&recent->entries[index], &recent->entries[index + 1u],
            (recent->count - index - 1u) * sizeof(recent->entries[0]));
    --recent->count;
    return true;
}

size_t wz_recent_files_count(const wz_recent_files_t* recent)
{
    return recent == NULL ? 0u : recent->count;
}

const wz_recent_file_t* wz_recent_files_at(const wz_recent_files_t* recent,
                                           size_t index)
{
    return recent != NULL && index < recent->count ? &recent->entries[index] : NULL;
}
