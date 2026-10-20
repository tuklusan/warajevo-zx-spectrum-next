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
#include "core/wz_bus.h"
#include "core/wz_machine_profile.h"
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
    case WZ_TRACE_ULA_FETCH: return "ula_fetch";
    case WZ_TRACE_BORDER_CHANGE: return "border_change";
    case WZ_TRACE_MIC_CHANGE: return "mic_change";
    case WZ_TRACE_EAR_CHANGE: return "ear_change";
    case WZ_TRACE_INTERFACE1_IO: return "interface1_io";
    default: return "unknown";
    }
}

typedef struct {
    wz_trace_cpu_state_sync_t cpu_state;
    const wz_machine_profile_t* profile;
    FILE* output;
} dump_context_t;

typedef struct {
    wz_qword_t frame;
    wz_dword_t line;
    wz_dword_t line_tstate;
    wz_dword_t master_phase;
    wz_dword_t ula_row;
    wz_dword_t ula_cell;
    wz_dword_t ula_offset;
    wz_byte_t ula_fetch_kind;
    wz_byte_t ula_active;
} trace_timing_position_t;

static const char* bus_cycle_name(wz_byte_t cycle)
{
    switch ((wz_bus_cycle_t)cycle) {
    case WZ_BUS_M1_OPCODE_FETCH: return "m1_opcode_fetch";
    case WZ_BUS_MEMORY_READ: return "memory_read";
    case WZ_BUS_MEMORY_WRITE: return "memory_write";
    case WZ_BUS_IO_READ: return "io_read";
    case WZ_BUS_IO_WRITE: return "io_write";
    case WZ_BUS_INTERRUPT_ACKNOWLEDGE: return "interrupt_acknowledge";
    case WZ_BUS_INTERNAL: return "internal";
    default: return "unknown";
    }
}

static const char* bus_source_name(wz_bus_source_t source)
{
    switch (source) {
    case WZ_BUS_SOURCE_NONE: return "none";
    case WZ_BUS_SOURCE_MEMORY: return "memory";
    case WZ_BUS_SOURCE_ULA: return "ula";
    case WZ_BUS_SOURCE_DATA_SOURCE: return "data_source";
    case WZ_BUS_SOURCE_INPUT: return "input";
    case WZ_BUS_SOURCE_FALLBACK: return "fallback";
    case WZ_BUS_SOURCE_AY: return "ay";
    case WZ_BUS_SOURCE_PRINTER: return "printer";
    default: return "unknown";
    }
}

static bool timing_position(const wz_machine_profile_t* profile,
                            wz_master_tick_t master_tick,
                            trace_timing_position_t* position)
{
    wz_qword_t frame_ticks;
    wz_qword_t frame_tick;
    wz_qword_t frame_tstate;

    if (profile == NULL || position == NULL ||
        profile->tstates_per_line == 0u ||
        profile->master_ticks_per_cpu_tstate == 0u ||
        profile->tstates_per_frame == 0u ||
        profile->ula_fetch_interval_tstates == 0u) {
        return false;
    }
    frame_ticks = (wz_qword_t)profile->tstates_per_frame *
        profile->master_ticks_per_cpu_tstate;
    if (frame_ticks == 0u) return false;

    position->frame = master_tick / frame_ticks;
    frame_tick = master_tick % frame_ticks;
    frame_tstate = frame_tick / profile->master_ticks_per_cpu_tstate;
    position->line = (wz_dword_t)(frame_tstate / profile->tstates_per_line);
    position->line_tstate =
        (wz_dword_t)(frame_tstate % profile->tstates_per_line);
    position->master_phase =
        (wz_dword_t)(frame_tick % profile->master_ticks_per_cpu_tstate);
    position->ula_fetch_kind = 0u;
    position->ula_active = 0u;

    if (profile->ula_fetch_interval_tstates != 0u &&
        frame_tstate >= profile->ula_fetch_start_tstate) {
        const wz_qword_t fetch_offset =
            frame_tstate - profile->ula_fetch_start_tstate;
        const wz_dword_t row = (wz_dword_t)(fetch_offset /
            profile->tstates_per_line);
        const wz_dword_t within_line = (wz_dword_t)(fetch_offset %
            profile->tstates_per_line);
        const wz_dword_t cell = within_line /
            profile->ula_fetch_interval_tstates;
        const wz_dword_t offset = within_line %
            profile->ula_fetch_interval_tstates;
        if (row < profile->ula_fetch_line_count &&
            cell < profile->ula_fetches_per_line) {
            position->ula_active = 1u;
            position->ula_row = row;
            position->ula_cell = cell;
            position->ula_offset = offset;
            if (offset == 0u) position->ula_fetch_kind = 1u;
            else if (offset == profile->ula_attribute_offset_tstates)
                position->ula_fetch_kind = 2u;
        }
    }
    return true;
}

static bool print_timing_position(FILE* output,
                                  const wz_machine_profile_t* profile,
                                  wz_master_tick_t tick)
{
    trace_timing_position_t position;
    const char* fetch = "active";
    if (!timing_position(profile, tick, &position)) return false;
    if (position.ula_fetch_kind == 1u) fetch = "bitmap";
    else if (position.ula_fetch_kind == 2u) fetch = "attribute";
    if (fprintf(output,
                "frame=%" PRIu64 " line=%u line_tstate=%u phase=%u",
                position.frame, (unsigned)position.line,
                (unsigned)position.line_tstate,
                (unsigned)position.master_phase) < 0) return false;
    if (position.ula_active != 0u) {
        if (fprintf(output,
                    " ula=%s,row=%u,cell=%u,offset=%u", fetch,
                    (unsigned)position.ula_row,
                    (unsigned)position.ula_cell,
                    (unsigned)position.ula_offset) < 0) return false;
    } else if (fputs(" ula=off", output) == EOF) {
        return false;
    }
    return true;
}

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
    if (event->kind == WZ_TRACE_CPU_BUS) {
        const wz_qword_t contention_ticks =
            (wz_qword_t)event->auxiliary *
            dump->profile->master_ticks_per_cpu_tstate;
        const wz_master_tick_t request_tick = event->master_tick >= contention_ticks ?
            event->master_tick - contention_ticks : event->master_tick;
        if (fprintf(output,
                    "bus_timing source=%s cycle=%s contention_delay_tstates=%u"
                    " requested_tick=%" PRIu64 " requested_",
                    bus_source_name(event->bus_source),
                    bus_cycle_name(event->cycle), (unsigned)event->auxiliary,
                    request_tick) < 0 ||
            !print_timing_position(output, dump->profile, request_tick) ||
            fprintf(output, " bus_tick=%" PRIu64 " bus_", event->master_tick) < 0 ||
            !print_timing_position(output, dump->profile, event->master_tick) ||
            fputc('\n', output) == EOF) {
            return false;
        }
    }
    if (event->kind == WZ_TRACE_ULA_FETCH) {
        const char* fetch_kind = event->auxiliary ==
            WZ_TRACE_ULA_ATTRIBUTE_FETCH ? "attribute" : "bitmap";
        const wz_dword_t row =
            (wz_dword_t)((event->register_snapshot >> 8u) & 0xffu);
        const wz_dword_t cell =
            (wz_dword_t)(event->register_snapshot & 0xffu);
        if (fprintf(output,
                    "ula_fetch kind=%s row=%u cell=%u address=%04x"
                    " value=%02x tick=%" PRIu64 " ",
                    fetch_kind, (unsigned)row, (unsigned)cell,
                    (unsigned)event->address, (unsigned)event->value,
                    event->master_tick) < 0 ||
            !print_timing_position(output, dump->profile,
                                   event->master_tick) ||
            fputc('\n', output) == EOF) {
            return false;
        }
    }
    if (event->kind == WZ_TRACE_BORDER_CHANGE) {
        if (fprintf(output,
                    "border_transition port=%04x from=%u to=%u tick=%" PRIu64 " ",
                    (unsigned)event->address, (unsigned)event->auxiliary,
                    (unsigned)event->value, event->master_tick) < 0 ||
            !print_timing_position(output, dump->profile, event->master_tick) ||
            fputc('\n', output) == EOF) {
            return false;
        }
    }
    if (event->kind == WZ_TRACE_MIC_CHANGE) {
        if (fprintf(output,
                    "mic_transition port=%04x from=%u to=%u tick=%" PRIu64 " ",
                    (unsigned)event->address, (unsigned)event->auxiliary,
                    (unsigned)event->value, event->master_tick) < 0 ||
            !print_timing_position(output, dump->profile, event->master_tick) ||
            fputc('\n', output) == EOF) {
            return false;
        }
    }
    if (event->kind == WZ_TRACE_EAR_CHANGE) {
        if (fprintf(output, "ear_transition from=%u to=%u tick=%" PRIu64 " ",
                    (unsigned)event->auxiliary, (unsigned)event->value,
                    event->master_tick) < 0 ||
            !print_timing_position(output, dump->profile, event->master_tick) ||
            fputc('\n', output) == EOF) {
            return false;
        }
    }
    if (event->kind == WZ_TRACE_INTERFACE1_IO) {
        const wz_byte_t motor_shift = (wz_byte_t)event->register_snapshot;
        const wz_byte_t active_motor = (wz_byte_t)(event->register_snapshot >> 8u);
        const char* direction = event->cycle == WZ_BUS_IO_READ ? "read" : "write";
        if (fprintf(output,
                    "interface1_io direction=%s port=%04x value=%02x"
                    " control=%02x motor_shift=%02x active_motor=%02x"
                    " tick=%" PRIu64 " ",
                    direction, (unsigned)event->address,
                    (unsigned)event->value, (unsigned)event->auxiliary,
                    (unsigned)motor_shift, (unsigned)active_motor,
                    event->master_tick) < 0 ||
            !print_timing_position(output, dump->profile, event->master_tick) ||
            fputc('\n', output) == EOF) {
            return false;
        }
    }
    return true;
}

int main(int argc, char** argv)
{
    char canonical_path[WZ_TRACE_PATH_MAX];
    size_t recovered = 0u;
    dump_context_t dump;
    wz_trace_file_metadata_t metadata;
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
    memset(&metadata, 0, sizeof(metadata));
    result = wz_trace_file_read_metadata(canonical_path, &metadata);
    if (result != WZ_RESULT_OK) {
        fprintf(stderr, "trace metadata could not be read (%d)\n", (int)result);
        return 1;
    }
    if (metadata.profile_kind == (wz_dword_t)WZ_MACHINE_48K_PAL) {
        dump.profile = wz_machine_profile_48k_pal();
    } else if (metadata.profile_kind == (wz_dword_t)WZ_MACHINE_128K_PAL) {
        dump.profile = wz_machine_profile_128k_pal();
    } else {
        fputs("trace contains an unsupported machine profile\n", stderr);
        return 1;
    }
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
