/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_ui_help.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    char text[256];
    const wz_ui_help_action_t* help;
    const wz_ui_help_action_t* about;

    if (wz_ui_help_action_count() != 2u) return 1;
    help = wz_ui_help_action_at(0u);
    about = wz_ui_help_action_at(1u);
    if (help == 0 || about == 0 || strcmp(help->id, "help.help") != 0 ||
        strcmp(about->id, "help.about") != 0) return 2;
    wz_ui_help_text(text, sizeof(text));
    if (strstr(text, "canonical menu") == 0) return 3;
    wz_ui_about_text(text, sizeof(text));
    if (strstr(text, "Warajevo ZX Spectrum Next") == 0 ||
        strstr(text, "GPL-2.0-or-later") == 0) return 4;
    puts("wz_ui_help contract passed");
    return 0;
}
