/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_DIAGNOSTICS_WZ_TRACE_FILE_H
#define WZ_DIAGNOSTICS_WZ_TRACE_FILE_H

#include <stdio.h>
#include "core/wz_trace.h"
#include "core/wz_z80.h"

#define WZ_TRACE_FILE_SIZE 16777216u
#define WZ_TRACE_HEADER_SIZE 256u
#define WZ_TRACE_RECORD_SIZE 24u
#define WZ_TRACE_COMMIT_OFFSET 20u

typedef struct {
    FILE* file;
    wz_qword_t session_id;
    wz_qword_t rom_identity;
    wz_qword_t next_slot;
    wz_qword_t generation;
    wz_qword_t first_sequence;
    wz_qword_t last_sequence;
    wz_master_tick_t last_master_tick;
    wz_dword_t event_mask;
    wz_dword_t profile_kind;
    wz_trace_emit_fn append_emit;
    void* append_context;
    bool frozen;
    bool failed;
} wz_trace_file_t;

typedef bool (*wz_trace_recover_fn)(const wz_trace_event_t* event, void* context);

/* Assembles the five adjacent CPU synchronization chunks into one Z80 state. */
typedef struct {
    wz_z80_state_t state;
    wz_master_tick_t master_tick;
    wz_qword_t last_sequence;
    wz_byte_t next_chunk;
    bool has_absolute_state;
} wz_trace_cpu_state_sync_t;

wz_result_t wz_trace_file_create(wz_trace_file_t* trace, const char* path,
                                 wz_qword_t session_id, wz_dword_t profile_kind,
                                 wz_qword_t rom_identity, wz_dword_t event_mask);
void wz_trace_file_set_forwarder(wz_trace_file_t* trace,
                                  wz_trace_emit_fn append_emit,
                                  void* append_context);
void wz_trace_file_emit(const wz_trace_event_t* event, void* context);
wz_result_t wz_trace_file_freeze(wz_trace_file_t* trace);
void wz_trace_file_close(wz_trace_file_t* trace);
wz_result_t wz_trace_file_recover(const char* path, wz_trace_recover_fn recover,
                                  void* context, size_t* recovered_count);
void wz_trace_cpu_state_sync_init(wz_trace_cpu_state_sync_t* sync);
bool wz_trace_cpu_state_sync_apply(wz_trace_cpu_state_sync_t* sync,
                                   const wz_trace_event_t* event);

#endif
