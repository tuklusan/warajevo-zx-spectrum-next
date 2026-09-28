/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. */

#ifndef WZ_TEST_SOKOL_AUDIO_STUB_H
#define WZ_TEST_SOKOL_AUDIO_STUB_H

#include <stdbool.h>

typedef struct {
    int sample_rate;
    int num_channels;
    int buffer_frames;
    int packet_frames;
} saudio_desc;

void saudio_setup(const saudio_desc* description);
void saudio_shutdown(void);
bool saudio_isvalid(void);
int saudio_push(const float* frames, int num_frames);

#endif
