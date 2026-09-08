/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "app/wz_compatibility_tools.h"

int main(void)
{
    const wz_compatibility_tool_t* tool;
    const char* reason;
    size_t index;

    if (wz_compatibility_tools_count() != 6u ||
        strcmp(wz_compatibility_tools_command_id(), "tools.compatibility") != 0 ||
        wz_compatibility_tools_at(6u) != 0) {
        return 1;
    }
    tool = wz_compatibility_tools_at(0u);
    if (tool == 0 || !wz_compatibility_tools_is_available(0u, &reason) ||
        reason != 0 || strcmp(tool->label, "Compatibility Tools") != 0) {
        return 1;
    }
    for (index = 1u; index < wz_compatibility_tools_count(); ++index) {
        tool = wz_compatibility_tools_at(index);
        if (tool == 0 || tool->availability != WZ_COMPATIBILITY_LATER ||
            wz_compatibility_tools_is_available(index, &reason) ||
            reason == 0 || strcmp(reason, "conversion-not-yet-implemented") != 0 ||
            tool->source_format[0] == '\0' || tool->destination_format[0] == '\0') {
            return 1;
        }
    }
    if (wz_compatibility_tools_is_available(6u, &reason) || reason == 0 ||
        strcmp(reason, "unknown-compatibility-tool") != 0) {
        return 1;
    }
    puts("wz_compatibility_tools contract passed");
    return 0;
}
