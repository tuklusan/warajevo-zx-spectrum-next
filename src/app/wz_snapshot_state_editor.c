/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_snapshot_state_editor.h"

void wz_snapshot_state_editor_init(wz_snapshot_state_editor_t* editor)
{
    if (editor != 0) editor->machine = 0;
}

wz_result_t wz_snapshot_state_editor_bind(wz_snapshot_state_editor_t* editor,
                                           wz_machine_t* machine)
{
    if (editor == 0 || machine == 0) return WZ_RESULT_INVALID_ARGUMENT;
    editor->machine = machine;
    return WZ_RESULT_OK;
}

void wz_snapshot_state_editor_unbind(wz_snapshot_state_editor_t* editor)
{
    if (editor != 0) editor->machine = 0;
}

bool wz_snapshot_state_editor_is_bound(
    const wz_snapshot_state_editor_t* editor)
{
    return editor != 0 && editor->machine != 0;
}

wz_result_t wz_snapshot_state_editor_set_access_mode(
    wz_snapshot_state_editor_t* editor, wz_debugger_access_mode_t mode)
{
    if (!wz_snapshot_state_editor_is_bound(editor)) return WZ_RESULT_INVALID_ARGUMENT;
    return wz_debugger_set_access_mode(editor->machine, mode);
}

wz_result_t wz_snapshot_state_editor_set_cpu_state(
    wz_snapshot_state_editor_t* editor, const wz_z80_state_t* state)
{
    if (!wz_snapshot_state_editor_is_bound(editor) || state == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    return wz_debugger_set_cpu_state(editor->machine, state);
}

wz_result_t wz_snapshot_state_editor_write_memory(
    wz_snapshot_state_editor_t* editor, wz_word_t address, wz_byte_t value)
{
    if (!wz_snapshot_state_editor_is_bound(editor)) return WZ_RESULT_INVALID_ARGUMENT;
    return wz_debugger_write_memory(editor->machine, address, value);
}

wz_result_t wz_snapshot_state_editor_undo_registers(
    wz_snapshot_state_editor_t* editor)
{
    if (!wz_snapshot_state_editor_is_bound(editor)) return WZ_RESULT_INVALID_ARGUMENT;
    return wz_debugger_undo_registers(editor->machine);
}

bool wz_snapshot_state_editor_undo_available(
    const wz_snapshot_state_editor_t* editor)
{
    return wz_snapshot_state_editor_is_bound(editor) &&
           wz_debugger_undo_available(editor->machine);
}
