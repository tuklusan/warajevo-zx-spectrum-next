/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>

#include "app/wz_file_open_run.h"

static int expect_route(const char* path, wz_open_run_route_t expected)
{
    wz_open_run_route_t actual = WZ_OPEN_RUN_UNSUPPORTED;
    if (wz_file_open_run_route(path, &actual) != WZ_OPEN_RUN_OK ||
        actual != expected) {
        return 1;
    }
    return 0;
}

typedef struct {
    wz_open_run_route_t route;
    const char* path;
    unsigned calls;
    bool succeed;
} dispatch_probe_t;

static bool record_dispatch(const char* path, void* context)
{
    dispatch_probe_t* probe = (dispatch_probe_t*)context;
    ++probe->calls;
    probe->path = path;
    return probe->succeed;
}

int main(void)
{
    wz_open_run_route_t route = WZ_OPEN_RUN_TAPE;
    dispatch_probe_t probe = {WZ_OPEN_RUN_TAPE, NULL, 0u, true};
    wz_open_run_handlers_t handlers = {record_dispatch, record_dispatch,
                                       record_dispatch, record_dispatch, &probe};
    if (expect_route("game.TAP", WZ_OPEN_RUN_TAPE) != 0 ||
        expect_route("game.tZx", WZ_OPEN_RUN_TAPE) != 0 ||
        expect_route("audio.WAV", WZ_OPEN_RUN_TAPE) != 0 ||
        expect_route("state.SNA", WZ_OPEN_RUN_SNAPSHOT) != 0 ||
        expect_route("state.z80", WZ_OPEN_RUN_SNAPSHOT) != 0 ||
        expect_route("drive.MDR", WZ_OPEN_RUN_MICRODRIVE) != 0 ||
        expect_route("legacy.VOC", WZ_OPEN_RUN_CONVERSION) != 0 ||
        expect_route("legacy.SNP", WZ_OPEN_RUN_CONVERSION) != 0 ||
        wz_file_open_run_route("unknown.bin", &route) !=
            WZ_OPEN_RUN_UNSUPPORTED_FORMAT ||
        route != WZ_OPEN_RUN_UNSUPPORTED ||
        wz_file_open_run_route("no-extension", &route) !=
            WZ_OPEN_RUN_UNSUPPORTED_FORMAT ||
        wz_file_open_run_route("", &route) != WZ_OPEN_RUN_INVALID_ARGUMENT ||
        wz_file_open_run_route(NULL, &route) != WZ_OPEN_RUN_INVALID_ARGUMENT ||
        wz_file_open_run_route("file.tap", NULL) != WZ_OPEN_RUN_INVALID_ARGUMENT) {
        fputs("wz_file_open_run contract failed\n", stderr);
        return 1;
    }
    if (wz_file_open_run_dispatch("game.TZX", &handlers) != WZ_OPEN_RUN_OK ||
        probe.calls != 1u || probe.path == NULL ||
        wz_file_open_run_dispatch("state.SNA", &handlers) != WZ_OPEN_RUN_OK ||
        probe.calls != 2u ||
        wz_file_open_run_dispatch("drive.MDR", &handlers) != WZ_OPEN_RUN_OK ||
        probe.calls != 3u ||
        wz_file_open_run_dispatch("legacy.VOC", &handlers) != WZ_OPEN_RUN_OK ||
        probe.calls != 4u ||
        wz_file_open_run_dispatch("other.bin", &handlers) !=
            WZ_OPEN_RUN_UNSUPPORTED_FORMAT || probe.calls != 4u ||
        wz_file_open_run_dispatch("game.tap", NULL) != WZ_OPEN_RUN_INVALID_ARGUMENT) {
        fputs("wz_file_open_run dispatch contract failed\n", stderr);
        return 1;
    }
    probe.succeed = false;
    if (wz_file_open_run_dispatch("game.tap", &handlers) !=
            WZ_OPEN_RUN_HANDLER_FAILED ||
        wz_file_open_run_dispatch("legacy.voc", &(wz_open_run_handlers_t){0}) !=
            WZ_OPEN_RUN_HANDLER_UNAVAILABLE) {
        fputs("wz_file_open_run handler contract failed\n", stderr);
        return 1;
    }
    puts("wz_file_open_run contract passed");
    return 0;
}
