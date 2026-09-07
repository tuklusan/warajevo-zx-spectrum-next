/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_host_output.h"

#include <stdio.h>

#if defined(_WIN32)
#include <windows.h>
#include <process.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#define WZ_HOST_OUTPUT_PATH_CAPACITY 1024u

static unsigned wz_host_output_attempt;

static bool make_temp_path(const char* path, char output[WZ_HOST_OUTPUT_PATH_CAPACITY])
{
    unsigned attempt = ++wz_host_output_attempt;
#if defined(_WIN32)
    unsigned long process_id = (unsigned long)_getpid();
#else
    long process_id = (long)getpid();
#endif
    int written = snprintf(output, WZ_HOST_OUTPUT_PATH_CAPACITY,
                           "%s.tmp.%lu.%u", path, process_id, attempt);
    return written >= 0 && (size_t)written < WZ_HOST_OUTPUT_PATH_CAPACITY;
}

#if defined(_WIN32)

static bool write_complete(HANDLE file, const void* data, size_t size)
{
    const unsigned char* bytes = (const unsigned char*)data;
    while (size != 0u) {
        DWORD chunk = size > (size_t)0xffffffffu ? 0xffffffffu : (DWORD)size;
        DWORD written = 0u;
        if (!WriteFile(file, bytes, chunk, &written, NULL) || written != chunk) {
            return false;
        }
        bytes += written;
        size -= written;
    }
    return FlushFileBuffers(file) != 0;
}

bool wz_host_output_write_atomic(const char* path,
                                 const void* data,
                                 size_t size)
{
    char temporary[WZ_HOST_OUTPUT_PATH_CAPACITY];
    HANDLE file;
    bool success = false;

    if (path == NULL || (data == NULL && size != 0u) ||
        !make_temp_path(path, temporary)) {
        return false;
    }
    file = CreateFileA(temporary, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                       FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    if (write_complete(file, data, size) && CloseHandle(file)) {
        file = INVALID_HANDLE_VALUE;
        success = MoveFileExA(temporary, path, MOVEFILE_REPLACE_EXISTING |
                              MOVEFILE_WRITE_THROUGH) != 0;
    }
    if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
    }
    if (!success) {
        DeleteFileA(temporary);
    }
    return success;
}

#else

static bool write_complete(int file, const void* data, size_t size)
{
    const unsigned char* bytes = (const unsigned char*)data;
    while (size != 0u) {
        ssize_t written = write(file, bytes, size);
        if (written <= 0) {
            return false;
        }
        bytes += (size_t)written;
        size -= (size_t)written;
    }
    return fsync(file) == 0;
}

bool wz_host_output_write_atomic(const char* path,
                                 const void* data,
                                 size_t size)
{
    char temporary[WZ_HOST_OUTPUT_PATH_CAPACITY];
    int file;
    bool success = false;

    if (path == NULL || (data == NULL && size != 0u) ||
        !make_temp_path(path, temporary)) {
        return false;
    }
    file = open(temporary, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (file < 0) {
        return false;
    }
    if (write_complete(file, data, size) && close(file) == 0) {
        file = -1;
        success = rename(temporary, path) == 0;
    }
    if (file >= 0) {
        close(file);
    }
    if (!success) {
        unlink(temporary);
    }
    return success;
}

#endif
