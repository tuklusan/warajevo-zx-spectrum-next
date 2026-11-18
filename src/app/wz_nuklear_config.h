/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#ifndef WZ_APP_WZ_NUKLEAR_CONFIG_H
#define WZ_APP_WZ_NUKLEAR_CONFIG_H

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
/* Complete pointer releases before opening synchronous native dialogs. */
#define NK_BUTTON_TRIGGER_ON_RELEASE

#include "nuklear.h"

#endif
