/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_compatibility_tools.h"

static const wz_compatibility_tool_t tools[WZ_COMPATIBILITY_TOOL_COUNT] = {
    {"tools.compatibility", "Compatibility Tools", "", "", "", 0,
     WZ_COMPATIBILITY_AVAILABLE},
    {"tools.compatibility.tape", "Tape Converter", "tape", "tape", "", 
     "conversion-not-yet-implemented", WZ_COMPATIBILITY_LATER},
    {"tools.compatibility.snapshot", "Snapshot Converter", "snapshot", "snapshot", "",
     "conversion-not-yet-implemented", WZ_COMPATIBILITY_LATER},
    {"tools.compatibility.spectrum_data", "Spectrum Data Converter", "spectrum-data", "text-or-spectrum-data", "",
     "conversion-not-yet-implemented", WZ_COMPATIBILITY_LATER},
    {"tools.compatibility.microdrive", "Microdrive Tools", "MDR", "MDR", "",
     "conversion-not-yet-implemented", WZ_COMPATIBILITY_LATER},
    {"tools.compatibility.database", "Legacy Database Converter", "legacy-database", "portable-data", "",
     "conversion-not-yet-implemented", WZ_COMPATIBILITY_LATER}
};

size_t wz_compatibility_tools_count(void)
{
    return WZ_COMPATIBILITY_TOOL_COUNT;
}

const wz_compatibility_tool_t* wz_compatibility_tools_at(size_t index)
{
    return index < WZ_COMPATIBILITY_TOOL_COUNT ? &tools[index] : 0;
}

bool wz_compatibility_tools_is_available(size_t index, const char** reason)
{
    const wz_compatibility_tool_t* tool = wz_compatibility_tools_at(index);

    if (reason != 0) {
        *reason = tool == 0 ? "unknown-compatibility-tool" : tool->reason;
    }
    return tool != 0 && tool->availability == WZ_COMPATIBILITY_AVAILABLE;
}

const char* wz_compatibility_tools_command_id(void)
{
    return "tools.compatibility";
}
