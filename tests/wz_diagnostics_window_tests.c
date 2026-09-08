/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include "app/wz_diagnostics_window.h"

static wz_result_t command_handler(const void* context,
                                   wz_command_arguments_t arguments,
                                   wz_command_result_t* result)
{
    (void)context; (void)arguments;
    if (result != 0) result->status = WZ_COMMAND_RESULT_SUCCESS;
    return WZ_RESULT_OK;
}

int main(void)
{
    wz_machine_t machine;
    wz_diagnostics_window_t window;
    wz_command_metadata_t command;
    wz_command_registry_t registry;
    const wz_diagnostics_router_t router = {0};
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) return 1;
    wz_diagnostics_window_init(&window);
    if (wz_diagnostics_window_open(&window, &machine, 0, &router) != WZ_DIAGNOSTICS_WINDOW_OK ||
        !wz_diagnostics_window_is_open(&window) || !window.diagnostic_block_available ||
        window.trace_available || wz_diagnostics_window_refresh(&window) != WZ_DIAGNOSTICS_WINDOW_OK ||
        wz_diagnostics_window_trace_reason(&window)[0] == '\0' ||
        wz_diagnostics_window_forwarding_reason(&window)[0] == '\0') {
        fputs("diagnostics window presentation failed\n", stderr);
        wz_machine_destroy(&machine); return 1;
    }
    if (wz_command_registry_init(&registry, &command, 1u) != WZ_RESULT_OK ||
        wz_diagnostics_window_register_commands(&registry, 0, command_handler, 0) != WZ_RESULT_OK ||
        wz_command_registry_finalize(&registry) != WZ_RESULT_OK ||
        wz_command_registry_find(&registry, WZ_DIAGNOSTICS_WINDOW_COMMAND_ID) == 0) {
        fputs("diagnostics command surface failed\n", stderr);
        wz_machine_destroy(&machine); return 1;
    }
    wz_diagnostics_window_close(&window);
    wz_machine_destroy(&machine);
    puts("wz_diagnostics_window contract passed");
    return 0;
}
