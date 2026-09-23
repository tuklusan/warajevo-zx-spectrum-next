/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_HOST_CONFIG_H
#define WZ_APP_WZ_HOST_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

typedef struct wz_host_preferences {
    int window_width;
    int window_height;
    bool fullscreen;
    unsigned display_scale;
    bool crop_to_display;
    bool audio_enabled;
    bool status_panel_visible;
} wz_host_preferences_t;

void wz_host_preferences_init(wz_host_preferences_t* preferences);

/* Serialize only approved host preferences; machine/session state is excluded. */
bool wz_host_preferences_serialize(const wz_host_preferences_t* preferences,
                                   char* output,
                                   size_t capacity,
                                   size_t* written);

/* Publish one complete preference snapshot through the atomic/locking policy. */
bool wz_host_preferences_write(const char* path,
                               const wz_host_preferences_t* preferences);

/* Write one complete host configuration transaction without exposing partial data. */
bool wz_host_config_write_atomic(const char* path,
                                 const void* data,
                                 size_t size);

#endif
