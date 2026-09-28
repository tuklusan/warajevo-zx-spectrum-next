/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. */

/*
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_file_open_run.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned calls[4];
    const char* expected_path;
    bool handler_result;
} test_context_t;

static bool record_route(unsigned route, const char* path, void* opaque)
{
    test_context_t* context = (test_context_t*)opaque;
    if (context == NULL || route >= 4u || path == NULL ||
        context->expected_path == NULL ||
        strcmp(path, context->expected_path) != 0) {
        return false;
    }
    ++context->calls[route];
    return context->handler_result;
}

static bool tape_handler(const char* path, void* context)
{
    return record_route(0u, path, context);
}

static bool snapshot_handler(const char* path, void* context)
{
    return record_route(1u, path, context);
}

static bool microdrive_handler(const char* path, void* context)
{
    return record_route(2u, path, context);
}

static bool conversion_handler(const char* path, void* context)
{
    return record_route(3u, path, context);
}

static bool verify_route(const char* path, wz_open_run_route_t expected)
{
    wz_open_run_route_t actual = WZ_OPEN_RUN_UNSUPPORTED;
    return wz_file_open_run_route(path, &actual) ==
               (expected == WZ_OPEN_RUN_UNSUPPORTED
                    ? WZ_OPEN_RUN_UNSUPPORTED_FORMAT
                    : WZ_OPEN_RUN_OK) &&
           actual == expected;
}

static bool verify_classification(void)
{
    static const struct {
        const char* path;
        wz_open_run_route_t route;
    } cases[] = {
        {"media/demo.tap", WZ_OPEN_RUN_TAPE},
        {"media/demo.TZX", WZ_OPEN_RUN_TAPE},
        {"media/demo.Wav", WZ_OPEN_RUN_TAPE},
        {"media/state.sna", WZ_OPEN_RUN_SNAPSHOT},
        {"media/state.Z80", WZ_OPEN_RUN_SNAPSHOT},
        {"media/disk.mdr", WZ_OPEN_RUN_MICRODRIVE},
        {"media/legacy.voc", WZ_OPEN_RUN_CONVERSION},
        {"media/legacy.SCR", WZ_OPEN_RUN_CONVERSION},
        {"media/unknown.bin", WZ_OPEN_RUN_UNSUPPORTED},
        {"media/no-extension", WZ_OPEN_RUN_UNSUPPORTED},
        {"media/name.", WZ_OPEN_RUN_UNSUPPORTED},
    };

    for (size_t index = 0u; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        if (!verify_route(cases[index].path, cases[index].route)) {
            fprintf(stderr, "route mismatch for %s\n", cases[index].path);
            return false;
        }
    }
    if (wz_file_open_run_route(NULL, &(wz_open_run_route_t){0}) !=
            WZ_OPEN_RUN_INVALID_ARGUMENT ||
        wz_file_open_run_route("media/demo.tap", NULL) !=
            WZ_OPEN_RUN_INVALID_ARGUMENT ||
        wz_file_open_run_route("", &(wz_open_run_route_t){0}) !=
            WZ_OPEN_RUN_INVALID_ARGUMENT) {
        fputs("invalid route arguments were not rejected\n", stderr);
        return false;
    }
    return true;
}

static bool verify_dispatch(void)
{
    test_context_t context = {{0u, 0u, 0u, 0u}, "media/demo.TAP", true};
    wz_open_run_handlers_t handlers = {
        tape_handler, snapshot_handler, microdrive_handler, conversion_handler,
        &context
    };

    if (wz_file_open_run_dispatch(context.expected_path, &handlers) !=
            WZ_OPEN_RUN_OK || context.calls[0] != 1u ||
        context.calls[1] != 0u || context.calls[2] != 0u ||
        context.calls[3] != 0u) {
        fputs("tape selection did not dispatch only to the tape handler\n", stderr);
        return false;
    }
    context.expected_path = "media/state.z80";
    if (wz_file_open_run_dispatch(context.expected_path, &handlers) !=
            WZ_OPEN_RUN_OK || context.calls[1] != 1u ||
        context.calls[0] != 1u || context.calls[2] != 0u ||
        context.calls[3] != 0u) {
        fputs("snapshot selection did not dispatch only to the snapshot handler\n",
              stderr);
        return false;
    }
    context.expected_path = "media/disk.mdr";
    if (wz_file_open_run_dispatch(context.expected_path, &handlers) !=
            WZ_OPEN_RUN_OK || context.calls[2] != 1u) {
        fputs("MDR selection did not dispatch to the Microdrive handler\n", stderr);
        return false;
    }
    context.expected_path = "media/legacy.voc";
    if (wz_file_open_run_dispatch(context.expected_path, &handlers) !=
            WZ_OPEN_RUN_OK || context.calls[3] != 1u) {
        fputs("conversion selection did not dispatch to Compatibility Tools\n",
              stderr);
        return false;
    }
    context.expected_path = "media/legacy.tap";
    handlers.tape = NULL;
    if (wz_file_open_run_dispatch(context.expected_path, &handlers) !=
            WZ_OPEN_RUN_HANDLER_UNAVAILABLE || context.calls[0] != 1u) {
        fputs("missing owner handler was not rejected without fallback\n", stderr);
        return false;
    }
    context.expected_path = "media/demo.tap";
    handlers.tape = tape_handler;
    context.handler_result = false;
    if (wz_file_open_run_dispatch(context.expected_path, &handlers) !=
            WZ_OPEN_RUN_HANDLER_FAILED || context.calls[0] != 2u) {
        fputs("owner handler failure was not propagated\n", stderr);
        return false;
    }
    if (wz_file_open_run_dispatch("media/unknown.bin", &handlers) !=
            WZ_OPEN_RUN_UNSUPPORTED_FORMAT ||
        wz_file_open_run_dispatch(NULL, &handlers) != WZ_OPEN_RUN_INVALID_ARGUMENT ||
        wz_file_open_run_dispatch("media/demo.tap", NULL) !=
            WZ_OPEN_RUN_INVALID_ARGUMENT) {
        fputs("dispatch did not preserve unsupported/invalid results\n", stderr);
        return false;
    }
    return true;
}

int main(void)
{
    if (!verify_classification() || !verify_dispatch()) {
        return 1;
    }
    puts("PASS file_open_run_route_and_dispatch");
    return 0;
}
