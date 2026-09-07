/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "app/wz_recent_files.h"

int main(void)
{
    wz_recent_files_t recent;
    const wz_recent_file_t* entry;
    size_t index;

    wz_recent_files_init(&recent);
    if (!wz_recent_files_add(&recent, "first.tap") ||
        !wz_recent_files_add(&recent, "second.sna") ||
        wz_recent_files_count(&recent) != 2u) {
        return 1;
    }
    entry = wz_recent_files_at(&recent, 0u);
    if (entry == NULL || strcmp(entry->path, "second.sna") != 0 ||
        strcmp(entry->command_id, "file.open_run") != 0 ||
        !wz_recent_files_add(&recent, "first.tap") ||
        strcmp(wz_recent_files_at(&recent, 0u)->path, "first.tap") != 0 ||
        wz_recent_files_count(&recent) != 2u ||
        !wz_recent_files_remove(&recent, 0u) ||
        strcmp(wz_recent_files_at(&recent, 0u)->path, "second.sna") != 0 ||
        wz_recent_files_remove(&recent, 4u) ||
        wz_recent_files_add(&recent, "") ||
        wz_recent_files_at(&recent, 4u) != NULL) {
        return 1;
    }
    for (index = 0u; index < WZ_RECENT_FILES_CAPACITY + 2u; ++index) {
        char path[32];
        (void)snprintf(path, sizeof(path), "file-%u.tap", (unsigned)index);
        if (!wz_recent_files_add(&recent, path)) {
            return 1;
        }
    }
    if (wz_recent_files_count(&recent) != WZ_RECENT_FILES_CAPACITY) {
        return 1;
    }
    puts("wz_recent_files contract passed");
    return 0;
}
