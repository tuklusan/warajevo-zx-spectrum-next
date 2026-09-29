/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdio.h>
#include <string.h>

#include "app/wz_host_output_utf8.h"

#if defined(_WIN32)
#include <windows.h>
#include <process.h>
#include <wchar.h>
#else
#include <unistd.h>
#endif

static int verify_output(const char* path
#if defined(_WIN32)
                        , const wchar_t* wide_path
#endif
                        )
{
    static const unsigned char expected[] = {0x00u, 0x80u, 0xffu, 0x42u};
    static const unsigned char first[] = {0x31u};
    unsigned char actual[sizeof(expected)];
#if defined(_WIN32)
    unsigned char trailing_byte;
    DWORD read_count = 0u;
    DWORD trailing_count = 0u;
    BOOL close_result;
    HANDLE file;
#else
    size_t read_count;
    int trailing;
    int close_result;
    FILE* file;
#endif
    if (!wz_host_output_write_atomic_utf8(path, first, sizeof(first)) ||
        !wz_host_output_write_atomic_utf8(path, expected, sizeof(expected))) {
        return 0;
    }
#if defined(_WIN32)
    file = CreateFileW(wide_path, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    if (!ReadFile(file, actual, (DWORD)sizeof(actual), &read_count, NULL) ||
        !ReadFile(file, &trailing_byte, 1u, &trailing_count, NULL)) {
        CloseHandle(file);
        return 0;
    }
    close_result = CloseHandle(file);
    if (read_count != (DWORD)sizeof(actual) || trailing_count != 0u || !close_result ||
        memcmp(actual, expected, sizeof(expected)) != 0) return 0;
#else
    file = fopen(path, "rb");
    if (file == NULL) return 0;
    read_count = fread(actual, 1u, sizeof(actual), file);
    trailing = fgetc(file);
    close_result = fclose(file);
    if (read_count != sizeof(actual) || trailing != EOF || close_result != 0 ||
        memcmp(actual, expected, sizeof(expected)) != 0) return 0;
#endif
#if defined(_WIN32)
    if (!DeleteFileW(wide_path)) return 0;
#else
    if (remove(path) != 0) return 0;
#endif
    return 1;
}

int main(void)
{
#if defined(_WIN32)
    wchar_t wide_path[4096];
    char utf8_path[4096];
    const DWORD path_capacity = (DWORD)(sizeof(wide_path) / sizeof(wide_path[0]));
    DWORD directory_length = GetTempPathW(path_capacity - 96u, wide_path);
    int utf8_length;
    if (directory_length == 0u || directory_length >= path_capacity - 96u) return 1;
    if (swprintf(wide_path + directory_length,
            sizeof(wide_path) / sizeof(wide_path[0]) - directory_length,
            L"wzsn-atomic-\u03a9-%lu.tap", (unsigned long)_getpid()) < 0) return 1;
    utf8_length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide_path,
        -1, utf8_path, (int)sizeof(utf8_path), NULL, NULL);
    if (utf8_length <= 0 || !verify_output(utf8_path, wide_path)) return 1;
#else
    char path[1024];
    int length = snprintf(path, sizeof(path),
        "/tmp/wzsn-atomic-\xce\xa9-%ld.tap", (long)getpid());
    if (length < 0 || (size_t)length >= sizeof(path) ||
        !verify_output(path)) return 1;
#endif
    puts("PASS UTF-8 atomic output replacement");
    return 0;
}
