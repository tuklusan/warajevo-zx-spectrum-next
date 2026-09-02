/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#define SOKOL_NO_ENTRY
#define SOKOL_IMPL
#if defined(_WIN32)
#define SOKOL_D3D11
#elif defined(__APPLE__)
#define SOKOL_METAL
#else
#define SOKOL_GLCORE
#define SOKOL_X11
#endif
#include "sokol_app.h"
#include "sokol_audio.h"

#include "core/wz_machine.h"
#include "app/wz_sokol_audio.h"

typedef struct {
    wz_machine_t machine;
    wz_sokol_audio_t audio;
    bool initialized;
} wz_host_session_t;

static wz_host_session_t wz_host_session;

static void wz_host_session_init(void)
{
    wz_host_session.initialized =
        wz_machine_init(&wz_host_session.machine, wz_machine_profile_48k_pal()) == WZ_RESULT_OK;
    if (wz_host_session.initialized) {
        (void)wz_sokol_audio_init(&wz_host_session.audio);
    }
}

static void wz_host_session_shutdown(void)
{
    if (wz_host_session.initialized) {
        wz_machine_destroy(&wz_host_session.machine);
        wz_sokol_audio_shutdown(&wz_host_session.audio);
        wz_host_session.initialized = false;
    }
}

static void wz_host_init(void)
{
    wz_host_session_init();
}

static void wz_host_cleanup(void)
{
    wz_host_session_shutdown();
}

static void wz_host_frame(void)
{
    /* Presentation and deterministic stepping are separate Phase-5 tasks. */
}

static void wz_host_event(const sapp_event* event)
{
    if (event->type == SAPP_EVENTTYPE_KEY_DOWN && event->key_code == SAPP_KEYCODE_ESCAPE) {
        sapp_request_quit();
    }
}

int main(void)
{
    sapp_run(&(sapp_desc){
        .init_cb = wz_host_init,
        .frame_cb = wz_host_frame,
        .cleanup_cb = wz_host_cleanup,
        .event_cb = wz_host_event,
        .width = 448,
        .height = 312,
        .window_title = "Warajevo ZX Spectrum Next",
    });
    return 0;
}
