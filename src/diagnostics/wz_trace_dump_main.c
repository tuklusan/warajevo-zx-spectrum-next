/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
Proprietary rights reserved except as expressly licensed herein.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
patent, trademark, and governing-law provisions.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#if !defined(_WIN32) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 700
#endif

#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(_WIN32)
#include <windows.h>
#define WZ_TRACE_PATH_MAX MAX_PATH
#else
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#define WZ_TRACE_PATH_MAX PATH_MAX
#endif
#include "diagnostics/wz_trace_file.h"

static bool print_event(const wz_trace_event_t* event, void* context)
{
    FILE* output = (FILE*)context;
    fprintf(output, "sequence=%" PRIu64 " tick=%" PRIu64 " kind=%u\n",
            event->sequence, event->master_tick, (unsigned)event->kind);
    return true;
}

int main(int argc, char** argv)
{
    char canonical_path[WZ_TRACE_PATH_MAX];
    size_t recovered = 0u;
    wz_result_t result;
    if (argc != 2) {
        fputs("usage: wz_trace_dump TRACE_FILE\n", stderr);
        return 2;
    }
#if defined(_WIN32)
    if (_fullpath(canonical_path, argv[1], sizeof(canonical_path)) == 0) {
#else
    if (realpath(argv[1], canonical_path) == 0) {
#endif
        fputs("trace file path could not be resolved\n", stderr);
        return 1;
    }
    result = wz_trace_file_recover(canonical_path, print_event, stdout, &recovered);
    if (result != WZ_RESULT_OK) {
        fprintf(stderr, "trace recovery failed (%d)\n", (int)result);
        return 1;
    }
    fprintf(stderr, "recovered=%zu\n", recovered);
    return 0;
}
