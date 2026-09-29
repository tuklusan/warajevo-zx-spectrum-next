/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_host_output_utf8.h"

#if defined(_WIN32)
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <wchar.h>
static volatile LONG wz_host_output_utf8_attempt;
#else
#include "app/wz_host_output.h"
#endif

bool wz_host_output_write_atomic_utf8(const char* utf8_path,
                                      const void* data,
                                      size_t size)
{
    if (utf8_path == NULL || utf8_path[0] == '\0' ||
        (data == NULL && size != 0u)) return false;
#if defined(_WIN32)
    {
        wchar_t path[4096];
        wchar_t temporary[4096];
        int wide_length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                               utf8_path, -1, path,
                                               (int)(sizeof(path) / sizeof(path[0])));
        int temporary_length;
        HANDLE file;
        bool success = false;
        bool created = false;
        if (wide_length <= 0) return false;
        temporary_length = swprintf(temporary,
            sizeof(temporary) / sizeof(temporary[0]),
            L"%ls.tmp.%lu.%ld", path, (unsigned long)_getpid(),
            (long)InterlockedIncrement(&wz_host_output_utf8_attempt));
        if (temporary_length < 0) return false;
        file = CreateFileW(temporary, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                           FILE_ATTRIBUTE_TEMPORARY, NULL);
        if (file == INVALID_HANDLE_VALUE) return false;
        created = true;
        {
            const unsigned char* bytes = (const unsigned char*)data;
            size_t remaining = size;
            bool written_all = true;
            while (remaining != 0u) {
                DWORD chunk = remaining > (size_t)0xffffffffu ?
                    0xffffffffu : (DWORD)remaining;
                DWORD written = 0u;
                if (!WriteFile(file, bytes, chunk, &written, NULL) ||
                    written == 0u) {
                    written_all = false;
                    break;
                }
                bytes += written;
                remaining -= written;
            }
            if (written_all && FlushFileBuffers(file) && CloseHandle(file)) {
                file = INVALID_HANDLE_VALUE;
                success = MoveFileExW(temporary, path,
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
            }
        }
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
        if (!success && created) DeleteFileW(temporary);
        return success;
    }
#else
    return wz_host_output_write_atomic(utf8_path, data, size);
#endif
}
