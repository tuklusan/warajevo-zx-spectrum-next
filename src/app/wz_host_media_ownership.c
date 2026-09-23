/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_host_media_ownership.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

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
    claim->reason = 0;
    if (!writable) {
        return true;
    }

#if defined(_WIN32)
    {
        HANDLE handle = CreateFileA(path, GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ, 0, OPEN_EXISTING,
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
    }
#else
    {
        int file = open(path, O_RDWR);
        if (file < 0 || flock(file, LOCK_EX | LOCK_NB) != 0) {
            if (file >= 0) {
                close(file);
            }
            claim->reason = "media-writer-unavailable";
            return false;
        }
        claim->native_handle = (intptr_t)file;
    }
#endif
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
        UnlockFileEx(handle, 0, 0xffffffffu, 0xffffffffu, &offset);
        CloseHandle(handle);
    }
#else
    {
        int file = (int)claim->native_handle;
        flock(file, LOCK_UN);
        close(file);
    }
#endif
    claim->held = false;
    claim->native_handle = (intptr_t)-1;
}
