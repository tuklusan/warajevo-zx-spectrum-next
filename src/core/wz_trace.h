/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_TRACE_H
#define WZ_CORE_WZ_TRACE_H

#include "core/wz_types.h"
#include "core/wz_bus.h"

#define WZ_TRACE_CPU_SYNC_INTERVAL 4096u
#define WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE 1u
#define WZ_TRACE_INTERRUPT_MASKABLE_ACCEPT 2u
#define WZ_TRACE_INTERRUPT_NMI_ACCEPT 3u
#define WZ_TRACE_INTERRUPT_LINE_ASSERT 4u
#define WZ_TRACE_INTERRUPT_LINE_DEASSERT 5u
#define WZ_TRACE_DEBUGGER_BREAKPOINT_HIT 6u

typedef enum {
    WZ_TRACE_MASTER_TICK_ADVANCED = 0,
    WZ_TRACE_TIMING_FULL = 1,
    WZ_TRACE_DEVELOPER_MARKER = 2,
    WZ_TRACE_CPU_INSTRUCTION = 3,
    WZ_TRACE_CPU_BUS = 4,
    WZ_TRACE_INTERRUPT = 5,
    WZ_TRACE_CPU_STATE_SYNC = 6,
    WZ_TRACE_CPU_STATE_DELTA = 7,
    /* Byte returned by an M1 cycle; pairs with the preceding CPU bus record. */
    WZ_TRACE_CPU_OPCODE_BYTE = 8,
    WZ_TRACE_ULA_FETCH = 9,
    WZ_TRACE_BORDER_CHANGE = 10,
    WZ_TRACE_MIC_CHANGE = 11,
    WZ_TRACE_EAR_CHANGE = 12,
    WZ_TRACE_INTERFACE1_IO = 13,
    WZ_TRACE_ZXNET_EVENT = 14
} wz_trace_event_kind_t;

#define WZ_TRACE_ZXNET_BLOCK_QUEUED 1u
#define WZ_TRACE_ZXNET_READ_STARTED 2u
#define WZ_TRACE_ZXNET_CLAIM_STARTED 3u
#define WZ_TRACE_ZXNET_CLAIM_RESOLVED 4u
#define WZ_TRACE_ZXNET_READ_PROGRESS 5u
#define WZ_TRACE_ZXNET_WRITE_PROGRESS 6u
#define WZ_TRACE_ZXNET_WRITE_COMMITTED 7u

#define WZ_TRACE_ULA_BITMAP_FETCH 0u
#define WZ_TRACE_ULA_ATTRIBUTE_FETCH 1u

typedef struct {
    wz_trace_event_kind_t kind;
    wz_master_tick_t master_tick;
    wz_qword_t sequence;
    wz_word_t address;
    wz_word_t program_counter;
    wz_word_t stack_pointer;
    wz_qword_t register_snapshot;
    wz_byte_t value;
    wz_byte_t auxiliary;
    wz_byte_t cycle;
    wz_byte_t t_states;
    wz_bus_source_t bus_source;
} wz_trace_event_t;

typedef void (*wz_trace_emit_fn)(const wz_trace_event_t* event, void* context);

typedef struct {
    wz_trace_emit_fn emit;
    void* context;
    wz_qword_t next_sequence;
    wz_qword_t cpu_fetch_count;
    wz_qword_t cpu_state_chunks[5];
    bool cpu_state_valid;
} wz_trace_sink_t;

void wz_trace_sink_init(wz_trace_sink_t* sink,
                        wz_trace_emit_fn emit,
                        void* context);
void wz_trace_emit(wz_trace_sink_t* sink,
                   wz_trace_event_kind_t kind,
                   wz_master_tick_t master_tick);
void wz_trace_emit_detail(wz_trace_sink_t* sink,
                          const wz_trace_event_t* event);

#endif
