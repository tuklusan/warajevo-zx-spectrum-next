/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_file_open_run.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

static bool wz_extension_equals(const char* extension, const char* expected)
{
    while (*extension != '\0' && *expected != '\0') {
        if (tolower((unsigned char)*extension) !=
            tolower((unsigned char)*expected)) {
            return false;
        }
        ++extension;
        ++expected;
    }
    return *extension == '\0' && *expected == '\0';
}

wz_open_run_result_t wz_file_open_run_route(const char* path,
                                             wz_open_run_route_t* route)
{
    const char* separator;
    const char* extension;

    if (path == NULL || route == NULL || path[0] == '\0') {
        return WZ_OPEN_RUN_INVALID_ARGUMENT;
    }
    separator = strrchr(path, '.');
    if (separator == NULL || separator[1] == '\0') {
        *route = WZ_OPEN_RUN_UNSUPPORTED;
        return WZ_OPEN_RUN_UNSUPPORTED_FORMAT;
    }
    extension = separator;
    if (wz_extension_equals(extension, ".tap") ||
        wz_extension_equals(extension, ".tzx") ||
        wz_extension_equals(extension, ".wav")) {
        *route = WZ_OPEN_RUN_TAPE;
        return WZ_OPEN_RUN_OK;
    }
    if (wz_extension_equals(extension, ".sna") ||
        wz_extension_equals(extension, ".z80")) {
        *route = WZ_OPEN_RUN_SNAPSHOT;
        return WZ_OPEN_RUN_OK;
    }
    if (wz_extension_equals(extension, ".mdr")) {
        *route = WZ_OPEN_RUN_MICRODRIVE;
        return WZ_OPEN_RUN_OK;
    }
    *route = WZ_OPEN_RUN_UNSUPPORTED;
    return WZ_OPEN_RUN_UNSUPPORTED_FORMAT;
}
