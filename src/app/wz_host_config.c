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
#include <errno.h>

#if defined(_WIN32)
#include <windows.h>
#include <process.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

void wz_host_preferences_init(wz_host_preferences_t* preferences)
{
    if (preferences == NULL) {
        return;
    }
    *preferences = (wz_host_preferences_t){
        .window_width = 0,
        .window_height = 0,
        .fullscreen = false,
        .display_scale = 0u,
        .crop_to_display = false,
        .audio_enabled = true,
        .status_panel_visible = true,
    };
}

static bool wz_make_user_directory(const char* path)
{
#if defined(_WIN32)
    DWORD attributes;
    if (CreateDirectoryA(path, NULL)) return true;
    if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
    attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0u;
#else
    struct stat info;
    if (mkdir(path, 0700) == 0) return true;
    return errno == EEXIST && stat(path, &info) == 0 && S_ISDIR(info.st_mode);
#endif
}

bool wz_host_preferences_path(char* output, size_t capacity)
{
    char directory[1024];
#if defined(__APPLE__)
    char parent[1024];
#endif
    const char* base = NULL;
    const char* separator = "/";
    int written;
    if (output == NULL || capacity == 0u) return false;
#if defined(_WIN32)
    base = getenv("LOCALAPPDATA");
    separator = "\\";
#elif defined(__APPLE__)
    base = getenv("HOME");
#else
    base = getenv("XDG_CONFIG_HOME");
    if (base == NULL || base[0] != '/') {
        const char* home = getenv("HOME");
        if (home == NULL || home[0] == '\0') return false;
        written = snprintf(directory, sizeof(directory), "%s/.config", home);
        if (written < 0 || (size_t)written >= sizeof(directory) ||
            !wz_make_user_directory(directory)) return false;
        base = directory;
    } else if (!wz_make_user_directory(base)) {
        return false;
    }
#endif
    if (base == NULL || base[0] == '\0') return false;
#if defined(__APPLE__)
    written = snprintf(directory, sizeof(directory),
        "%s/Library/Application Support", base);
    if (written < 0 || (size_t)written >= sizeof(directory) ||
        !wz_make_user_directory(directory)) return false;
    memcpy(parent, directory, (size_t)written + 1u);
    written = snprintf(directory, sizeof(directory),
        "%s/Warajevo-ZX-Spectrum-Next", parent);
#else
#if !defined(_WIN32)
    {
        const char* application_directory = "warajevo-zx-spectrum-next";
#else
    {
        const char* application_directory = "Warajevo-ZX-Spectrum-Next";
#endif
    written = snprintf(directory, sizeof(directory),
        "%s%s%s", base, separator, application_directory);
    }
#endif
    if (written < 0 || (size_t)written >= sizeof(directory) ||
        !wz_make_user_directory(directory)) return false;
    written = snprintf(output, capacity, "%s%shost-settings.v1",
                       directory, separator);
    return written >= 0 && (size_t)written < capacity;
}

static bool wz_parse_integer(const char* text, long minimum, long maximum,
                             long* value)
{
    char* end = NULL;
    long parsed;
    if (text == NULL || text[0] == '\0' || value == NULL) return false;
    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        parsed < minimum || parsed > maximum) return false;
    *value = parsed;
    return true;
}

static bool wz_host_preferences_parse(const char* serialized, size_t size,
                                      wz_host_preferences_t* preferences)
{
    enum { REQUIRED_KEYS = 0xffu };
    char input[2048];
    unsigned seen = 0u;
    char* line;
    if (serialized == NULL || preferences == NULL || size == 0u ||
        size >= sizeof(input) || memchr(serialized, '\0', size) != NULL) {
        return false;
    }
    memcpy(input, serialized, size);
    input[size] = '\0';
    wz_host_preferences_init(preferences);
    line = input;
    while (*line != '\0') {
        char* end = strchr(line, '\n');
        char* equals;
        long value;
        unsigned bit;
        if (end == NULL) return false;
        *end = '\0';
        equals = strchr(line, '=');
        if (equals == NULL) return false;
        *equals++ = '\0';
        if (strcmp(line, "version") == 0) {
            bit = 1u << 0;
            if (!wz_parse_integer(equals, 1, 1, &value)) return false;
        } else if (strcmp(line, "window_width") == 0) {
            bit = 1u << 1;
            if (!wz_parse_integer(equals, 0, 16384, &value) ||
                (value != 0 && value < 320)) return false;
            preferences->window_width = (int)value;
        } else if (strcmp(line, "window_height") == 0) {
            bit = 1u << 2;
            if (!wz_parse_integer(equals, 0, 16384, &value) ||
                (value != 0 && value < 240)) return false;
            preferences->window_height = (int)value;
        } else if (strcmp(line, "fullscreen") == 0) {
            bit = 1u << 3;
            if (!wz_parse_integer(equals, 0, 1, &value)) return false;
            preferences->fullscreen = value != 0;
        } else if (strcmp(line, "display_scale") == 0) {
            bit = 1u << 4;
            if (!wz_parse_integer(equals, 0, 4, &value)) return false;
            preferences->display_scale = (unsigned)value;
        } else if (strcmp(line, "crop_to_display") == 0) {
            bit = 1u << 5;
            if (!wz_parse_integer(equals, 0, 1, &value)) return false;
            preferences->crop_to_display = value != 0;
        } else if (strcmp(line, "audio_enabled") == 0) {
            bit = 1u << 6;
            if (!wz_parse_integer(equals, 0, 1, &value)) return false;
            preferences->audio_enabled = value != 0;
        } else if (strcmp(line, "status_panel_visible") == 0) {
            bit = 1u << 7;
            if (!wz_parse_integer(equals, 0, 1, &value)) return false;
            preferences->status_panel_visible = value != 0;
        } else {
            return false;
        }
        if ((seen & bit) != 0u) return false;
        seen |= bit;
        line = end + 1;
    }
    return seen == REQUIRED_KEYS;
}

bool wz_host_preferences_load(const char* path,
                              wz_host_preferences_t* preferences)
{
    FILE* file;
    char serialized[2048];
    size_t size;
    bool read_failed;
    bool close_failed;
    wz_host_preferences_t loaded;
    if (path == NULL || preferences == NULL) return false;
    file = fopen(path, "rb");
    if (file == NULL) {
        if (errno == ENOENT) {
            wz_host_preferences_init(preferences);
            return true;
        }
        return false;
    }
    size = fread(serialized, 1u, sizeof(serialized), file);
    read_failed = ferror(file) != 0 || size == sizeof(serialized);
    close_failed = fclose(file) != 0;
    if (read_failed || close_failed ||
        !wz_host_preferences_parse(serialized, size, &loaded)) return false;
    *preferences = loaded;
    return true;
}

bool wz_host_preferences_serialize(const wz_host_preferences_t* preferences,
                                   char* output,
                                   size_t capacity,
                                   size_t* written)
{
    int count;
    if (preferences == NULL || output == NULL || capacity == 0u) {
        return false;
    }
    if ((preferences->window_width != 0 &&
         (preferences->window_width < 320 || preferences->window_width > 16384)) ||
        (preferences->window_height != 0 &&
         (preferences->window_height < 240 || preferences->window_height > 16384)) ||
        preferences->display_scale > 4u) {
        return false;
    }
    count = snprintf(output, capacity,
                     "version=1\nwindow_width=%d\nwindow_height=%d\nfullscreen=%d\n"
                     "display_scale=%u\ncrop_to_display=%d\naudio_enabled=%d\n"
                     "status_panel_visible=%d\n",
                     preferences->window_width, preferences->window_height,
                     preferences->fullscreen ? 1 : 0, preferences->display_scale,
                     preferences->crop_to_display ? 1 : 0,
                     preferences->audio_enabled ? 1 : 0,
                     preferences->status_panel_visible ? 1 : 0);
    if (count < 0 || (size_t)count >= capacity) {
        return false;
    }
    if (written != NULL) {
        *written = (size_t)count;
    }
    return true;
}

bool wz_host_preferences_write(const char* path,
                               const wz_host_preferences_t* preferences)
{
    char serialized[512];
    size_t written = 0u;
    if (!wz_host_preferences_serialize(preferences, serialized,
                                       sizeof(serialized), &written)) {
        return false;
    }
    return wz_host_config_write_atomic(path, serialized, written);
}

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
