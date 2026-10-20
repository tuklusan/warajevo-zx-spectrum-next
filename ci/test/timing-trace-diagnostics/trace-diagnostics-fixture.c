/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdio.h>
#include <string.h>

#include "core/wz_bus.h"
#include "core/wz_machine_profile.h"
#include "diagnostics/wz_trace_file.h"

int main(int argc, char** argv)
{
    wz_trace_file_t trace = {0};
    wz_trace_event_t event = {0};
    int result = 1;

    if (argc != 2) return 2;
    if (wz_trace_file_create(&trace, argv[1], 1u,
            (wz_dword_t)WZ_MACHINE_48K_PAL, 0u,
            UINT32_C(1) << WZ_TRACE_CPU_BUS) != WZ_RESULT_OK) {
        return 3;
    }

    event.kind = WZ_TRACE_CPU_BUS;
    event.sequence = 0u;
    event.master_tick = UINT64_C(14340) * 2u;
    event.address = 0x4000u;
    event.value = 0xa5u;
    event.cycle = (wz_byte_t)WZ_BUS_MEMORY_READ;
    event.t_states = 3u;
    event.auxiliary = 3u;
    wz_trace_file_emit(&event, &trace);
    if (trace.failed || trace.record_count != 1u ||
        wz_trace_file_freeze(&trace) != WZ_RESULT_OK) {
        goto cleanup;
    }
    result = 0;

cleanup:
    if (trace.file != NULL) wz_trace_file_close(&trace);
    return result;
}
