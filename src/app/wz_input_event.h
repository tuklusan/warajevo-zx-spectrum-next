/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#ifndef WZ_APP_WZ_INPUT_EVENT_H
#define WZ_APP_WZ_INPUT_EVENT_H

typedef struct {
    unsigned char source;
    unsigned char key;
    unsigned char pressed;
} wz_input_event_t;

#endif
