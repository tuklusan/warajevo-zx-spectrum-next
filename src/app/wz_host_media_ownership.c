/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#include "app/wz_host_media_ownership.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#include <ctype.h>
#else
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#endif

static bool wz_host_media_lock_path(const char* path, char* output,
                                    size_t capacity)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    int written;
    size_t index;
#if defined(_WIN32)
    char canonical[MAX_PATH];
    char temporary[MAX_PATH];
    DWORD length = GetFullPathNameA(path, MAX_PATH, canonical, NULL);
    DWORD temporary_length = GetTempPathA(MAX_PATH, temporary);
    if (length == 0u || length >= MAX_PATH || temporary_length == 0u ||
        temporary_length >= MAX_PATH) return false;
    for (index = 0u; canonical[index] != '\0'; ++index) {
        hash ^= (unsigned char)tolower((unsigned char)canonical[index]);
        hash *= UINT64_C(1099511628211);
    }
    written = snprintf(output, capacity, "%swzsn-media-claim-%016llx.lock",
        temporary, (unsigned long long)hash);
    return written > 0 && (size_t)written < capacity;
#else
    char canonical[4096];
    const char* resolved = realpath(path, canonical);
    if (resolved == NULL) return false;
    for (index = 0u; canonical[index] != '\0'; ++index) {
        hash ^= (unsigned char)canonical[index];
        hash *= UINT64_C(1099511628211);
    }
    written = snprintf(output, capacity, "/tmp/wzsn-media-claim-%016llx.lock",
        (unsigned long long)hash);
    return written > 0 && (size_t)written < capacity;
#endif
}

static bool wz_host_media_identity_lock_path(const char* path, char* output,
                                            size_t capacity)
{
    uint64_t identity = UINT64_C(14695981039346656037);
    int written;
#if defined(_WIN32)
    BY_HANDLE_FILE_INFORMATION info;
    HANDLE file = CreateFileA(path, FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE) return false;
    if (!GetFileInformationByHandle(file, &info)) {
        CloseHandle(file);
        return false;
    }
    CloseHandle(file);
    identity ^= info.dwVolumeSerialNumber;
    identity *= UINT64_C(1099511628211);
    identity ^= info.nFileIndexHigh;
    identity *= UINT64_C(1099511628211);
    identity ^= info.nFileIndexLow;
    {
        char temporary[MAX_PATH];
        DWORD length = GetTempPathA(MAX_PATH, temporary);
        if (length == 0u || length >= MAX_PATH) return false;
        written = snprintf(output, capacity,
            "%swzsn-media-identity-%016llx.lock", temporary,
            (unsigned long long)identity);
    }
#else
    struct stat info;
    if (stat(path, &info) != 0) return false;
    identity ^= (uint64_t)info.st_dev;
    identity *= UINT64_C(1099511628211);
    identity ^= (uint64_t)info.st_ino;
    written = snprintf(output, capacity,
        "/tmp/wzsn-media-identity-%016llx.lock",
        (unsigned long long)identity);
#endif
    return written > 0 && (size_t)written < capacity;
}

bool wz_host_media_claim_acquire(const char* path,
                                 bool writable,
                                 wz_host_media_claim_t* claim)
{
    if (path == 0 || claim == 0) {
        return false;
    }
    claim->held = false;
    claim->writable = writable;
    claim->native_handle = (intptr_t)-1;
    claim->identity_handle = (intptr_t)-1;
    claim->reason = 0;
    if (!writable) {
        return true;
    }

    {
        char lock_path[4096];
        char identity_path[4096];
        if (!wz_host_media_lock_path(path, lock_path, sizeof(lock_path))) {
            claim->reason = "media-path-unavailable";
            return false;
        }
        if (!wz_host_media_identity_lock_path(path, identity_path,
                                               sizeof(identity_path))) {
            claim->reason = "media-identity-unavailable";
            return false;
        }
#if defined(_WIN32)
    {
        HANDLE handle = CreateFileA(lock_path, GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    0, OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL, 0);
        OVERLAPPED offset = {0};
        if (handle == INVALID_HANDLE_VALUE ||
            !LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
                        0, 0xffffffffu, 0xffffffffu, &offset)) {
            if (handle != INVALID_HANDLE_VALUE) {
                CloseHandle(handle);
            }
            claim->reason = "media-writer-unavailable";
            return false;
        }
        claim->native_handle = (intptr_t)handle;
        handle = CreateFileA(identity_path,
            GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
            0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (handle == INVALID_HANDLE_VALUE ||
            !LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK |
                LOCKFILE_FAIL_IMMEDIATELY, 0, 0xffffffffu, 0xffffffffu,
                &offset)) {
            if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
            UnlockFileEx((HANDLE)claim->native_handle, 0, 0xffffffffu,
                         0xffffffffu, &offset);
            CloseHandle((HANDLE)claim->native_handle);
            claim->native_handle = (intptr_t)-1;
            claim->reason = "media-writer-unavailable";
            return false;
        }
        claim->identity_handle = (intptr_t)handle;
    }
#else
    {
        int file = open(lock_path, O_CREAT | O_RDWR, 0600);
        if (file < 0 || flock(file, LOCK_EX | LOCK_NB) != 0) {
            if (file >= 0) {
                close(file);
            }
            claim->reason = "media-writer-unavailable";
            return false;
        }
        claim->native_handle = (intptr_t)file;
        file = open(identity_path, O_CREAT | O_RDWR, 0600);
        if (file < 0 || flock(file, LOCK_EX | LOCK_NB) != 0) {
            if (file >= 0) close(file);
            flock((int)claim->native_handle, LOCK_UN);
            close((int)claim->native_handle);
            claim->native_handle = (intptr_t)-1;
            claim->reason = "media-writer-unavailable";
            return false;
        }
        claim->identity_handle = (intptr_t)file;
    }
#endif
    }
    claim->held = true;
    return true;
}

wz_host_media_claim_outcome_t wz_host_media_claim_acquire_with_fallback(
    const char* path, bool writable, bool allow_read_only,
    wz_host_media_claim_t* claim)
{
    if (path == 0 || claim == 0) {
        if (claim != 0) claim->reason = "invalid-argument";
        return WZ_HOST_MEDIA_CLAIM_INVALID_ARGUMENT;
    }
    if (wz_host_media_claim_acquire(path, writable, claim)) {
        claim->reason = writable ? 0 : "read-only-request";
        return writable ? WZ_HOST_MEDIA_CLAIM_WRITABLE : WZ_HOST_MEDIA_CLAIM_READ_ONLY;
    }
    if (allow_read_only) {
        claim->held = false;
        claim->writable = false;
        claim->native_handle = (intptr_t)-1;
        claim->identity_handle = (intptr_t)-1;
        claim->reason = "read-only-fallback";
        return WZ_HOST_MEDIA_CLAIM_READ_ONLY;
    }
    return WZ_HOST_MEDIA_CLAIM_UNAVAILABLE;
}

wz_host_media_claim_outcome_t wz_host_media_claim_outcome(
    const wz_host_media_claim_t* claim)
{
    if (claim == 0) return WZ_HOST_MEDIA_CLAIM_INVALID_ARGUMENT;
    if (claim->held && claim->writable) return WZ_HOST_MEDIA_CLAIM_WRITABLE;
    if (!claim->writable) return WZ_HOST_MEDIA_CLAIM_READ_ONLY;
    return WZ_HOST_MEDIA_CLAIM_UNAVAILABLE;
}

void wz_host_media_claim_release(wz_host_media_claim_t* claim)
{
    if (claim == 0 || !claim->held) {
        return;
    }
#if defined(_WIN32)
    {
        HANDLE handle = (HANDLE)claim->native_handle;
        OVERLAPPED offset = {0};
        HANDLE identity_handle = (HANDLE)claim->identity_handle;
        if (identity_handle != INVALID_HANDLE_VALUE &&
            identity_handle != NULL) {
            UnlockFileEx(identity_handle, 0, 0xffffffffu, 0xffffffffu,
                         &offset);
            CloseHandle(identity_handle);
        }
        UnlockFileEx(handle, 0, 0xffffffffu, 0xffffffffu, &offset);
        CloseHandle(handle);
    }
#else
    {
        int file = (int)claim->native_handle;
        int identity_file = (int)claim->identity_handle;
        if (identity_file >= 0) {
            flock(identity_file, LOCK_UN);
            close(identity_file);
        }
        flock(file, LOCK_UN);
        close(file);
    }
#endif
    claim->held = false;
    claim->native_handle = (intptr_t)-1;
    claim->identity_handle = (intptr_t)-1;
}
