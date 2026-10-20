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
#include <string.h>
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

static const char* event_kind_name(wz_trace_event_kind_t kind)
{
    switch (kind) {
    case WZ_TRACE_MASTER_TICK_ADVANCED: return "master_tick_advanced";
    case WZ_TRACE_TIMING_FULL: return "timing_full";
    case WZ_TRACE_DEVELOPER_MARKER: return "developer_marker";
    case WZ_TRACE_CPU_INSTRUCTION: return "cpu_instruction";
    case WZ_TRACE_CPU_BUS: return "cpu_bus";
    case WZ_TRACE_INTERRUPT: return "interrupt";
    case WZ_TRACE_CPU_STATE_SYNC: return "cpu_state_sync";
    case WZ_TRACE_CPU_STATE_DELTA: return "cpu_state_delta";
    case WZ_TRACE_CPU_OPCODE_BYTE: return "cpu_opcode_byte";
    default: return "unknown";
    }
}

typedef struct {
    wz_trace_cpu_state_sync_t cpu_state;
    FILE* output;
} dump_context_t;

static void print_register_bank(FILE* output, const char* prefix,
                                const wz_z80_register_bank_t* bank)
{
    fprintf(output,
            " %sA=%02x %sF=%02x %sB=%02x %sC=%02x"
            " %sD=%02x %sE=%02x %sH=%02x %sL=%02x",
            prefix, (unsigned)bank->a, prefix, (unsigned)bank->f,
            prefix, (unsigned)bank->b, prefix, (unsigned)bank->c,
            prefix, (unsigned)bank->d, prefix, (unsigned)bank->e,
            prefix, (unsigned)bank->h, prefix, (unsigned)bank->l);
}

static void print_cpu_state(FILE* output,
                            const wz_trace_cpu_state_sync_t* sync)
{
    const wz_z80_state_t* state = &sync->state;
    fprintf(output, "cpu_state sequence=%" PRIu64 " tick=%" PRIu64,
            sync->last_sequence, sync->master_tick);
    print_register_bank(output, "", &state->main);
    print_register_bank(output, "_alt", &state->alternate);
    fprintf(output,
            " IX=%04x IY=%04x SP=%04x PC=%04x MEMPTR=%04x"
            " I=%02x R=%02x IFF1=%u IFF2=%u EI_DELAY=%u IM=%u"
            " HALTED=%u Q=%02x\n",
            (unsigned)state->ix, (unsigned)state->iy,
            (unsigned)state->stack_pointer, (unsigned)state->program_counter,
            (unsigned)state->memptr, (unsigned)state->i, (unsigned)state->r,
            (unsigned)state->iff1, (unsigned)state->iff2,
            (unsigned)state->interrupt_enable_delay,
            (unsigned)state->interrupt_mode, (unsigned)state->halted,
            (unsigned)state->q);
}

static bool print_event(const wz_trace_event_t* event, void* context)
{
    dump_context_t* dump = (dump_context_t*)context;
    FILE* output = dump->output;
    if (fprintf(output,
                   "sequence=%" PRIu64 " tick=%" PRIu64
                   " kind=%s(%u) address=%04x pc=%04x sp=%04x"
                   " payload=%016" PRIx64 " value=%02x auxiliary=%02x"
                   " cycle=%02x t_states=%u\n",
                   event->sequence, event->master_tick,
                   event_kind_name(event->kind), (unsigned)event->kind,
                   (unsigned)event->address,
                   (unsigned)event->program_counter,
                   (unsigned)event->stack_pointer,
                   event->register_snapshot, (unsigned)event->value,
                   (unsigned)event->auxiliary, (unsigned)event->cycle,
                   (unsigned)event->t_states) < 0) {
        return false;
    }
    if (wz_trace_cpu_state_sync_apply(&dump->cpu_state, event)) {
        print_cpu_state(output, &dump->cpu_state);
        return !ferror(output);
    }
    return true;
}

int main(int argc, char** argv)
{
    char canonical_path[WZ_TRACE_PATH_MAX];
    size_t recovered = 0u;
    dump_context_t dump;
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
    memset(&dump, 0, sizeof(dump));
    wz_trace_cpu_state_sync_init(&dump.cpu_state);
    dump.output = stdout;
    result = wz_trace_file_recover(canonical_path, print_event, &dump, &recovered);
    if (result != WZ_RESULT_OK) {
        fprintf(stderr, "trace recovery failed (%d)\n", (int)result);
        return 1;
    }
    fprintf(stderr, "recovered=%zu\n", recovered);
    return 0;
}
