/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_SNAPSHOT_STATE_EDITOR_H
#define WZ_APP_WZ_SNAPSHOT_STATE_EDITOR_H

#include <stdbool.h>

#include "core/wz_debugger.h"

/* The UI owns no editing state; this is an adapter to the live debugger. */
typedef struct {
    wz_machine_t* machine;
} wz_snapshot_state_editor_t;

void wz_snapshot_state_editor_init(wz_snapshot_state_editor_t* editor);
wz_result_t wz_snapshot_state_editor_bind(wz_snapshot_state_editor_t* editor,
                                           wz_machine_t* machine);
void wz_snapshot_state_editor_unbind(wz_snapshot_state_editor_t* editor);
bool wz_snapshot_state_editor_is_bound(
    const wz_snapshot_state_editor_t* editor);
wz_result_t wz_snapshot_state_editor_set_access_mode(
    wz_snapshot_state_editor_t* editor, wz_debugger_access_mode_t mode);
wz_result_t wz_snapshot_state_editor_set_cpu_state(
    wz_snapshot_state_editor_t* editor, const wz_z80_state_t* state);
wz_result_t wz_snapshot_state_editor_write_memory(
    wz_snapshot_state_editor_t* editor, wz_word_t address, wz_byte_t value);
wz_result_t wz_snapshot_state_editor_undo_registers(
    wz_snapshot_state_editor_t* editor);
bool wz_snapshot_state_editor_undo_available(
    const wz_snapshot_state_editor_t* editor);

#endif
