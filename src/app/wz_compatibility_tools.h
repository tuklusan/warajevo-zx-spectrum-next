/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_COMPATIBILITY_TOOLS_H
#define WZ_APP_WZ_COMPATIBILITY_TOOLS_H

#include <stdbool.h>
#include <stddef.h>

#define WZ_COMPATIBILITY_TOOL_COUNT 6u

typedef enum {
    WZ_COMPATIBILITY_AVAILABLE = 0,
    WZ_COMPATIBILITY_LATER,
    WZ_COMPATIBILITY_UNAVAILABLE
} wz_compatibility_availability_t;

typedef enum {
    WZ_FILE_ROUTE_UNKNOWN = 0,
    WZ_FILE_ROUTE_NATIVE_LOAD,
    WZ_FILE_ROUTE_EXPLICIT_CONVERSION,
    WZ_FILE_ROUTE_UNSUPPORTED
} wz_file_route_t;

typedef struct {
    const char* id;
    const char* label;
    const char* source_format;
    const char* destination_format;
    const char* warning;
    const char* reason;
    wz_compatibility_availability_t availability;
} wz_compatibility_tool_t;

size_t wz_compatibility_tools_count(void);
const wz_compatibility_tool_t* wz_compatibility_tools_at(size_t index);
bool wz_compatibility_tools_is_available(size_t index, const char** reason);
const char* wz_compatibility_tools_command_id(void);
wz_file_route_t wz_compatibility_tools_route_for_format(
    const char* format,
    const char** reason);
const char* wz_compatibility_tools_route_name(wz_file_route_t route);

#endif
