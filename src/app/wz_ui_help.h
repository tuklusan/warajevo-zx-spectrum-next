/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_UI_HELP_H
#define WZ_APP_WZ_UI_HELP_H

#include <stddef.h>

#define WZ_UI_HELP_ACTION_COUNT 2u

typedef struct {
    const char* id;
    const char* label;
    const char* description;
} wz_ui_help_action_t;

size_t wz_ui_help_action_count(void);
const wz_ui_help_action_t* wz_ui_help_action_at(size_t index);
void wz_ui_help_text(char* output, size_t capacity);
void wz_ui_about_text(char* output, size_t capacity);

#endif
