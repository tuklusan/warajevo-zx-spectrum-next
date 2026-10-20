/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <time.h>
#endif

#include "diagnostics/wz_trace_file.h"

static void pause_briefly(void)
{
#if defined(_WIN32)
    Sleep(10u);
#else
    const struct timespec delay = {0, 10000000L};
    (void)nanosleep(&delay, NULL);
#endif
}

static int wait_for_path(const char* path, unsigned int timeout_ms)
{
    for (unsigned int elapsed = 0u; elapsed < timeout_ms; elapsed += 10u) {
        FILE* file = fopen(path, "rb");
        if (file != NULL) {
            (void)fclose(file);
            return 1;
        }
        pause_briefly();
    }
    return 0;
}

static int write_result(const char* path, const char* result)
{
    char temporary_path[1024];
    int written;
    FILE* file;
    int ok;
    if (path == NULL || result == NULL) return 0;
    written = snprintf(temporary_path, sizeof(temporary_path), "%s.tmp", path);
    if (written <= 0 || (size_t)written >= sizeof(temporary_path)) return 0;
    file = fopen(temporary_path, "wb");
    if (file == NULL) return 0;
    ok = fprintf(file, "%s\n", result) > 0;
    if (fclose(file) != 0) ok = 0;
    if (ok && rename(temporary_path, path) == 0) return 1;
    (void)remove(temporary_path);
    return 0;
}

int main(int argc, char** argv)
{
    wz_trace_file_t trace = {0};
    wz_trace_event_t event = {0};
    char* end = NULL;
    unsigned long long session;
    int exit_code = 1;
    const char* outcome = "DENIED";

    if (argc != 6) return 2;
    errno = 0;
    session = strtoull(argv[5], &end, 10);
    if (errno != 0 || end == argv[5] || *end != '\0' || session == 0u)
        return 2;
    if (!wait_for_path(argv[2], 30000u)) return 3;

    if (wz_trace_file_create(&trace, argv[1], (wz_qword_t)session,
                             1u, 2u,
                             UINT32_C(1) << WZ_TRACE_DEVELOPER_MARKER) ==
        WZ_RESULT_OK) {
        outcome = "OWNER";
        event.kind = WZ_TRACE_DEVELOPER_MARKER;
        event.sequence = 0u;
        event.master_tick = (wz_master_tick_t)session;
        event.value = (wz_byte_t)session;
        wz_trace_file_emit(&event, &trace);
        if (trace.failed || trace.record_count != 1u ||
            wz_trace_file_freeze(&trace) != WZ_RESULT_OK) {
            exit_code = 4;
            (void)write_result(argv[4], "ERROR");
            goto cleanup;
        }
    }

    if (!write_result(argv[4], outcome)) goto cleanup;
    if (trace.file != NULL && !wait_for_path(argv[3], 30000u)) goto cleanup;
    exit_code = 0;

cleanup:
    if (trace.file != NULL) wz_trace_file_close(&trace);
    return exit_code;
}
