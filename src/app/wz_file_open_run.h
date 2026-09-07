/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_FILE_OPEN_RUN_H
#define WZ_APP_WZ_FILE_OPEN_RUN_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    WZ_OPEN_RUN_TAPE = 0,
    WZ_OPEN_RUN_SNAPSHOT,
    WZ_OPEN_RUN_MICRODRIVE,
    WZ_OPEN_RUN_UNSUPPORTED
} wz_open_run_route_t;

typedef enum {
    WZ_OPEN_RUN_OK = 0,
    WZ_OPEN_RUN_INVALID_ARGUMENT,
    WZ_OPEN_RUN_UNSUPPORTED_FORMAT,
    WZ_OPEN_RUN_HANDLER_UNAVAILABLE,
    WZ_OPEN_RUN_HANDLER_FAILED
} wz_open_run_result_t;

typedef bool (*wz_open_run_handler_fn)(const char* path, void* context);

typedef struct {
    wz_open_run_handler_fn tape;
    wz_open_run_handler_fn snapshot;
    wz_open_run_handler_fn microdrive;
    void* context;
} wz_open_run_handlers_t;

/* Classify a selected path without opening it or changing machine state. */
wz_open_run_result_t wz_file_open_run_route(const char* path,
                                             wz_open_run_route_t* route);

/* Dispatch to the owning subsystem without selecting a fallback route. */
wz_open_run_result_t wz_file_open_run_dispatch(
    const char* path, const wz_open_run_handlers_t* handlers);

#endif
