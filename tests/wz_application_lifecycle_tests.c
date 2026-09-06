/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>

#include "app/wz_application_lifecycle.h"

static void count_quit(void* context)
{
    unsigned* count = (unsigned*)context;
    ++(*count);
}

int main(void)
{
    wz_application_lifecycle_t lifecycle;
    wz_application_state_t state;
    unsigned callback_count = 0u;

    if (wz_application_lifecycle_init(&lifecycle, count_quit,
                                      &callback_count) != WZ_RESULT_OK ||
        wz_application_read_state(&lifecycle, &state) != WZ_RESULT_OK ||
        state != WZ_APPLICATION_RUNNING ||
        wz_application_mark_terminated(&lifecycle) != WZ_RESULT_INVALID_STATE ||
        wz_application_request_quit(&lifecycle) != WZ_RESULT_OK ||
        callback_count != 1u ||
        wz_application_read_state(&lifecycle, &state) != WZ_RESULT_OK ||
        state != WZ_APPLICATION_QUIT_REQUESTED ||
        !wz_application_quit_requested(&lifecycle) ||
        wz_application_request_quit(&lifecycle) != WZ_RESULT_OK ||
        callback_count != 1u ||
        wz_application_mark_terminated(&lifecycle) != WZ_RESULT_OK ||
        wz_application_read_state(&lifecycle, &state) != WZ_RESULT_OK ||
        state != WZ_APPLICATION_TERMINATED ||
        wz_application_request_quit(&lifecycle) != WZ_RESULT_INVALID_STATE ||
        wz_application_mark_terminated(&lifecycle) != WZ_RESULT_OK) {
        fputs("application lifecycle contract failed\n", stderr);
        return 1;
    }

    puts("wz_application_lifecycle contract passed");
    return 0;
}
