/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

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
