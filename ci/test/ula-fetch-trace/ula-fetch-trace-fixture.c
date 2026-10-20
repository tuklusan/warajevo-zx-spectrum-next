/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdio.h>
#include <string.h>

#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"
#include "diagnostics/wz_trace_file.h"

int main(int argc, char** argv)
{
    wz_machine_t machine = {0};
    wz_trace_file_t trace = {0};
    wz_trace_sink_t sink;
    int result = 1;

    if (argc != 2) {
        result = 2;
        goto cleanup;
    }
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) !=
        WZ_RESULT_OK) {
        result = 3;
        goto cleanup;
    }
    wz_machine_memory_write(&machine, 0x4000u, 0xa5u);
    wz_machine_memory_write(&machine, 0x5800u, 0x41u);
    wz_machine_memory_write(&machine, 0x4001u, 0x5au);
    wz_machine_memory_write(&machine, 0x5801u, 0x63u);

    if (wz_trace_file_create(&trace, argv[1], 1u,
            (wz_dword_t)WZ_MACHINE_48K_PAL, 0u,
            UINT32_C(1) << WZ_TRACE_ULA_FETCH) != WZ_RESULT_OK) {
        goto cleanup;
    }
    wz_trace_sink_init(&sink, wz_trace_file_emit, &trace);
    wz_machine_set_timing_trace(&machine, &sink);
    if (wz_machine_memory_write_at_tick(&machine, 0x5801u, 0x73u,
            UINT64_C(14340) * 2u) != WZ_RESULT_OK ||
        trace.failed || trace.record_count != 4u) {
        goto cleanup;
    }
    wz_machine_set_timing_trace(&machine, NULL);
    if (wz_trace_file_freeze(&trace) != WZ_RESULT_OK) goto cleanup;
    result = 0;

cleanup:
    wz_machine_set_timing_trace(&machine, NULL);
    if (trace.file != NULL) wz_trace_file_close(&trace);
    wz_machine_destroy(&machine);
    return result;
}
