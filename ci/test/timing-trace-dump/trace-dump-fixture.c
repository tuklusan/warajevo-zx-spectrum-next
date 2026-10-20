/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "diagnostics/wz_trace_file.h"

#include <stdio.h>
#include <string.h>

static wz_qword_t pack_register_bank(wz_byte_t a, wz_byte_t f, wz_byte_t b,
                                    wz_byte_t c, wz_byte_t d, wz_byte_t e,
                                    wz_byte_t h, wz_byte_t l)
{
    return (wz_qword_t)a | ((wz_qword_t)f << 8u) |
        ((wz_qword_t)b << 16u) | ((wz_qword_t)c << 24u) |
        ((wz_qword_t)d << 32u) | ((wz_qword_t)e << 40u) |
        ((wz_qword_t)h << 48u) | ((wz_qword_t)l << 56u);
}

static void write_sync_chunk(wz_trace_file_t* trace, wz_qword_t sequence,
                             wz_byte_t cycle, wz_qword_t payload)
{
    wz_trace_event_t event;
    memset(&event, 0, sizeof(event));
    event.kind = WZ_TRACE_CPU_STATE_SYNC;
    event.sequence = sequence;
    event.cycle = cycle;
    event.register_snapshot = payload;
    wz_trace_file_emit(&event, trace);
}

int main(int argc, char** argv)
{
    wz_trace_file_t trace;
    wz_trace_event_t event;
    const wz_dword_t mask = (UINT32_C(1) << WZ_TRACE_CPU_STATE_SYNC) |
        (UINT32_C(1) << WZ_TRACE_CPU_BUS) |
        (UINT32_C(1) << WZ_TRACE_INTERRUPT);
    if (argc != 2 || wz_trace_file_create(&trace, argv[1], 1u, 0u, 0u, mask) !=
                          WZ_RESULT_OK) {
        fputs("could not create trace fixture\n", stderr);
        return 1;
    }

    write_sync_chunk(&trace, 1u, 0u,
        pack_register_bank(0x12u, 0x34u, 0x56u, 0x78u,
                           0x9au, 0xbcu, 0xdeu, 0xf0u));
    write_sync_chunk(&trace, 2u, 1u,
        pack_register_bank(0x13u, 0x35u, 0x57u, 0x79u,
                           0x9bu, 0xbdu, 0xdfu, 0xf1u));
    write_sync_chunk(&trace, 3u, 2u,
        UINT64_C(0x7788556633441122));
    write_sync_chunk(&trace, 4u, 3u,
        UINT64_C(0x02010001ccbb99aa));
    write_sync_chunk(&trace, 5u, 4u,
        UINT64_C(0x0000000000010000));

    memset(&event, 0, sizeof(event));
    event.kind = WZ_TRACE_CPU_BUS;
    event.sequence = 6u;
    event.master_tick = 42u;
    event.address = 0x4000u;
    event.program_counter = 0x7788u;
    event.value = 0xa5u;
    event.auxiliary = 0x06u;
    event.cycle = 0x02u;
    event.t_states = 3u;
    event.register_snapshot = 0xbeefu;
    wz_trace_file_emit(&event, &trace);

    memset(&event, 0, sizeof(event));
    event.kind = WZ_TRACE_INTERRUPT;
    event.sequence = 7u;
    event.master_tick = 42u;
    event.program_counter = 0x7788u;
    event.auxiliary = WZ_TRACE_INTERRUPT_NMI_ACCEPT;
    wz_trace_file_emit(&event, &trace);

    if (trace.failed || wz_trace_file_freeze(&trace) != WZ_RESULT_OK) {
        wz_trace_file_close(&trace);
        fputs("could not finish trace fixture\n", stderr);
        return 1;
    }
    wz_trace_file_close(&trace);
    return 0;
}
