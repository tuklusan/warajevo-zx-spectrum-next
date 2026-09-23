/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <inttypes.h>
#include <stdio.h>
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
    size_t recovered = 0u;
    wz_result_t result;
    if (argc != 2) {
        fputs("usage: wz_trace_dump TRACE_FILE\n", stderr);
        return 2;
    }
    result = wz_trace_file_recover(argv[1], print_event, stdout, &recovered);
    if (result != WZ_RESULT_OK) {
        fprintf(stderr, "trace recovery failed (%d)\n", (int)result);
        return 1;
    }
    fprintf(stderr, "recovered=%zu\n", recovered);
    return 0;
}
