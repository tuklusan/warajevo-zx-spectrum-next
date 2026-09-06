/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_DEBUGGER_H
#define WZ_CORE_WZ_DEBUGGER_H

#include <stddef.h>
#include <stdbool.h>

#include "core/wz_machine.h"
#include "core/wz_trace.h"
#include "core/wz_z80.h"

typedef struct {
    wz_z80_state_t cpu;
    wz_master_tick_t master_tick;
    wz_byte_t border_color;
    wz_byte_t networking_mode;
} wz_debugger_snapshot_t;

typedef enum {
    WZ_DEBUGGER_READ_ONLY = 0,
    WZ_DEBUGGER_PAUSED_MUTATION = 1
} wz_debugger_access_mode_t;

enum {
    WZ_DEBUGGER_TRACE_ACCESS_MODE = 1,
    WZ_DEBUGGER_TRACE_CPU_MUTATION = 2,
    WZ_DEBUGGER_TRACE_MEMORY_MUTATION = 3,
    WZ_DEBUGGER_TRACE_STEP = 4,
    WZ_DEBUGGER_TRACE_CONTINUE = 5,
    WZ_DEBUGGER_TRACE_JUMP = 6,
    WZ_DEBUGGER_TRACE_UNDO = 7
};

/* Inspection always reads the one live machine; it does not clone execution state. */
wz_result_t wz_debugger_snapshot(const wz_machine_t* machine,
                                 wz_debugger_snapshot_t* snapshot);
wz_result_t wz_debugger_read_memory(const wz_machine_t* machine,
                                    wz_word_t address,
                                    wz_byte_t* value);
wz_result_t wz_debugger_read_memory_block(const wz_machine_t* machine,
                                          wz_word_t address,
                                          wz_byte_t* values,
                                          size_t length);
wz_result_t wz_debugger_find_memory(const wz_machine_t* machine,
                                    wz_word_t address,
                                    size_t length,
                                    wz_byte_t value,
                                    wz_word_t* found_address);
wz_result_t wz_debugger_format_hex8(wz_byte_t value, char* output, size_t capacity);
wz_result_t wz_debugger_format_hex16(wz_word_t value, char* output, size_t capacity);
wz_result_t wz_debugger_disassemble(const wz_machine_t* machine,
                                    wz_word_t address,
                                    char* output,
                                    size_t capacity,
                                    size_t* consumed);

/* Mutation and execution controls are defined here and implemented by later CRs. */
wz_result_t wz_debugger_set_access_mode(wz_machine_t* machine,
                                         wz_debugger_access_mode_t mode);
wz_result_t wz_debugger_write_memory(wz_machine_t* machine,
                                     wz_word_t address,
                                     wz_byte_t value);
wz_result_t wz_debugger_write_memory_word(wz_machine_t* machine,
                                          wz_word_t address,
                                          wz_word_t value);
wz_result_t wz_debugger_fill_memory(wz_machine_t* machine,
                                    wz_word_t address,
                                    size_t length,
                                    wz_byte_t value);
wz_result_t wz_debugger_copy_memory(wz_machine_t* machine,
                                    wz_word_t source_address,
                                    wz_word_t destination_address,
                                    size_t length);
wz_result_t wz_debugger_set_cpu_state(wz_machine_t* machine,
                                      const wz_z80_state_t* state);
wz_result_t wz_debugger_jump(wz_machine_t* machine, wz_word_t address);
wz_result_t wz_debugger_undo_registers(wz_machine_t* machine);
bool wz_debugger_undo_available(const wz_machine_t* machine);
wz_result_t wz_debugger_set_breakpoint(wz_machine_t* machine,
                                       wz_word_t address);
wz_result_t wz_debugger_clear_breakpoint(wz_machine_t* machine);
bool wz_debugger_breakpoint_active(const wz_machine_t* machine);
bool wz_debugger_breakpoint_hit(const wz_machine_t* machine);
void wz_debugger_clear_breakpoint_hit(wz_machine_t* machine);
wz_result_t wz_debugger_step(wz_machine_t* machine, size_t* executed);
wz_result_t wz_debugger_continue(wz_machine_t* machine,
                                 size_t max_instructions,
                                 size_t* executed);

#endif
