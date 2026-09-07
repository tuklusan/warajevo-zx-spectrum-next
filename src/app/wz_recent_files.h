/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_RECENT_FILES_H
#define WZ_APP_WZ_RECENT_FILES_H

#include <stdbool.h>
#include <stddef.h>

#define WZ_RECENT_FILES_CAPACITY 10u
#define WZ_RECENT_FILE_PATH_CAPACITY 1024u

typedef struct {
    char path[WZ_RECENT_FILE_PATH_CAPACITY];
    const char* command_id;
} wz_recent_file_t;

typedef struct {
    wz_recent_file_t entries[WZ_RECENT_FILES_CAPACITY];
    size_t count;
} wz_recent_files_t;

void wz_recent_files_init(wz_recent_files_t* recent);
bool wz_recent_files_add(wz_recent_files_t* recent, const char* path);
bool wz_recent_files_remove(wz_recent_files_t* recent, size_t index);
size_t wz_recent_files_count(const wz_recent_files_t* recent);
const wz_recent_file_t* wz_recent_files_at(const wz_recent_files_t* recent,
                                           size_t index);

#endif
