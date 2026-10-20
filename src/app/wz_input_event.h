/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
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
