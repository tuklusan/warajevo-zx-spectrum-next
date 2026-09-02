/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_host_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#include <process.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

static bool wz_make_suffixed_path(const char* path,
                                  const char* suffix,
                                  char output[1024])
{
    int written = snprintf(output, 1024u, "%s%s", path, suffix);
    return written >= 0 && (size_t)written < 1024u;
}

#if defined(_WIN32)

static bool wz_write_windows_file(HANDLE file, const void* data, size_t size)
{
    const unsigned char* bytes = (const unsigned char*)data;
    while (size > 0u) {
        DWORD chunk = size > (size_t)0xffffffffu ? (DWORD)0xffffffffu : (DWORD)size;
        DWORD written = 0u;
        if (!WriteFile(file, bytes, chunk, &written, NULL) || written != chunk) {
            return false;
        }
        bytes += written;
        size -= written;
    }
    return FlushFileBuffers(file) != 0;
}

bool wz_host_config_write_atomic(const char* path,
                                 const void* data,
                                 size_t size)
{
    char lock_path[1024];
    char temp_suffix[64];
    char temp_path[1024];
    HANDLE lock = INVALID_HANDLE_VALUE;
    HANDLE temp = INVALID_HANDLE_VALUE;
    bool success = false;

    if (path == NULL || (data == NULL && size != 0u) ||
        !wz_make_suffixed_path(path, ".lock", lock_path) ||
        snprintf(temp_suffix, sizeof(temp_suffix), ".tmp.%lu",
                 (unsigned long)_getpid()) < 0 ||
        !wz_make_suffixed_path(path, temp_suffix, temp_path)) {
        return false;
    }
    lock = CreateFileA(lock_path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       CREATE_NEW, FILE_ATTRIBUTE_HIDDEN, NULL);
    if (lock == INVALID_HANDLE_VALUE) {
        return false;
    }
    temp = CreateFileA(temp_path, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                       FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (temp != INVALID_HANDLE_VALUE && wz_write_windows_file(temp, data, size)) {
        CloseHandle(temp);
        temp = INVALID_HANDLE_VALUE;
        success = MoveFileExA(temp_path, path, MOVEFILE_REPLACE_EXISTING |
                              MOVEFILE_WRITE_THROUGH) != 0;
    }
    if (temp != INVALID_HANDLE_VALUE) {
        CloseHandle(temp);
    }
    if (!success) {
        DeleteFileA(temp_path);
    }
    CloseHandle(lock);
    DeleteFileA(lock_path);
    return success;
}

#else

static bool wz_write_posix_file(int file, const void* data, size_t size)
{
    const unsigned char* bytes = (const unsigned char*)data;
    while (size > 0u) {
        ssize_t written = write(file, bytes, size);
        if (written <= 0) {
            return false;
        }
        bytes += (size_t)written;
        size -= (size_t)written;
    }
    return fsync(file) == 0;
}

bool wz_host_config_write_atomic(const char* path,
                                 const void* data,
                                 size_t size)
{
    char lock_path[1024];
    char temp_suffix[64];
    char temp_path[1024];
    int lock = -1;
    int temp = -1;
    bool success = false;

    if (path == NULL || (data == NULL && size != 0u) ||
        !wz_make_suffixed_path(path, ".lock", lock_path) ||
        snprintf(temp_suffix, sizeof(temp_suffix), ".tmp.%ld", (long)getpid()) < 0 ||
        !wz_make_suffixed_path(path, temp_suffix, temp_path)) {
        return false;
    }
    lock = open(lock_path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (lock < 0) {
        return false;
    }
    temp = open(temp_path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (temp >= 0 && wz_write_posix_file(temp, data, size) && close(temp) == 0) {
        temp = -1;
        success = rename(temp_path, path) == 0;
    }
    if (temp >= 0) {
        close(temp);
    }
    if (!success) {
        unlink(temp_path);
    }
    close(lock);
    unlink(lock_path);
    return success;
}

#endif
