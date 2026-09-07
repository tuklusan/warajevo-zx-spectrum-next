/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_ui_help.h"

#include <stdio.h>

static const wz_ui_help_action_t actions[WZ_UI_HELP_ACTION_COUNT] = {
    {"help.help", "Help", "Show keyboard and application help."},
    {"help.about", "About Warajevo ZX Spectrum Next", "Show project identity and licensing information."}
};

size_t wz_ui_help_action_count(void)
{
    return WZ_UI_HELP_ACTION_COUNT;
}

const wz_ui_help_action_t* wz_ui_help_action_at(size_t index)
{
    return index < WZ_UI_HELP_ACTION_COUNT ? &actions[index] : 0;
}

void wz_ui_help_text(char* output, size_t capacity)
{
    if (output == 0 || capacity == 0u) return;
    (void)snprintf(output, capacity,
                   "Help: use the canonical menu and command registry actions; "
                   "machine state remains deterministic and host-independent.");
    output[capacity - 1u] = '\0';
}

void wz_ui_about_text(char* output, size_t capacity)
{
    if (output == 0 || capacity == 0u) return;
    (void)snprintf(output, capacity,
                   "Warajevo ZX Spectrum Next | Copyright (c) 2026 Supratim Sanyal, "
                   "SANYALnet Labs | GPL-2.0-or-later for new original project material.");
    output[capacity - 1u] = '\0';
}
