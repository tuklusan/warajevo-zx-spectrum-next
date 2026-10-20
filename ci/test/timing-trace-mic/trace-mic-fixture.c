/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdint.h>

#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"
#include "core/wz_trace.h"
#include "diagnostics/wz_trace_file.h"

int main(int argc, char** argv)
{
    wz_machine_t machine = {0};
    wz_trace_file_t trace_file = {0};
    wz_trace_sink_t trace_sink = {0};
    const wz_dword_t event_mask =
        (UINT32_C(1) << WZ_TRACE_DEVELOPER_MARKER) |
        (UINT32_C(1) << WZ_TRACE_MIC_CHANGE);
    int result = 1;

    if (argc != 2 || wz_machine_init(&machine, wz_machine_profile_48k_pal()) !=
                         WZ_RESULT_OK) {
        goto cleanup;
    }
    if (wz_trace_file_create(&trace_file, argv[1], 1u,
            (wz_dword_t)WZ_MACHINE_48K_PAL, 0u, event_mask) != WZ_RESULT_OK) {
        goto cleanup;
    }
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    wz_machine_set_timing_trace(&machine, &trace_sink);

    /* A stable MIC level between writes must not produce a duplicate edge. */
    wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x10u, 10u);
    wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x10u, 20u);
    wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x00u, 25u);
    wz_machine_set_timing_trace(&machine, NULL);
    if (wz_trace_file_freeze(&trace_file) != WZ_RESULT_OK) {
        goto cleanup;
    }
    result = 0;

cleanup:
    wz_machine_set_timing_trace(&machine, NULL);
    wz_trace_file_close(&trace_file);
    wz_machine_destroy(&machine);
    return result;
}
