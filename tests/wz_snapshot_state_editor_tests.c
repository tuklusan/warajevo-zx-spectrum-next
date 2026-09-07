/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <assert.h>

#include "app/wz_snapshot_state_editor.h"

int main(void)
{
    wz_machine_t machine;
    wz_snapshot_state_editor_t editor;
    wz_z80_state_t changed;
    wz_byte_t value = 0u;

    assert(wz_machine_init(&machine, wz_machine_profile_48k_pal()) == WZ_RESULT_OK);
    wz_snapshot_state_editor_init(&editor);
    assert(!wz_snapshot_state_editor_is_bound(&editor));
    assert(wz_snapshot_state_editor_set_cpu_state(&editor, &machine.cpu) ==
           WZ_RESULT_INVALID_ARGUMENT);
    assert(wz_snapshot_state_editor_bind(&editor, &machine) == WZ_RESULT_OK);
    assert(wz_snapshot_state_editor_is_bound(&editor));

    changed = machine.cpu;
    changed.program_counter = (wz_word_t)0x1234u;
    assert(wz_snapshot_state_editor_set_cpu_state(&editor, &changed) ==
           WZ_RESULT_INVALID_STATE);
    assert(machine.cpu.program_counter != (wz_word_t)0x1234u);
    assert(wz_snapshot_state_editor_set_access_mode(
               &editor, WZ_DEBUGGER_PAUSED_MUTATION) == WZ_RESULT_OK);
    assert(wz_snapshot_state_editor_set_cpu_state(&editor, &changed) ==
           WZ_RESULT_OK);
    assert(machine.cpu.program_counter == (wz_word_t)0x1234u);
    assert(wz_snapshot_state_editor_undo_available(&editor));
    assert(wz_snapshot_state_editor_undo_registers(&editor) == WZ_RESULT_OK);
    assert(machine.cpu.program_counter != (wz_word_t)0x1234u);

    assert(wz_snapshot_state_editor_set_access_mode(
               &editor, WZ_DEBUGGER_READ_ONLY) == WZ_RESULT_OK);
    assert(wz_snapshot_state_editor_write_memory(&editor, (wz_word_t)0x4000u,
                                                 (wz_byte_t)0xa5u) ==
           WZ_RESULT_INVALID_STATE);
    assert(wz_snapshot_state_editor_set_access_mode(
               &editor, WZ_DEBUGGER_PAUSED_MUTATION) == WZ_RESULT_OK);
    assert(wz_snapshot_state_editor_write_memory(&editor, (wz_word_t)0x4000u,
                                                 (wz_byte_t)0xa5u) == WZ_RESULT_OK);
    assert(wz_debugger_read_memory(&machine, (wz_word_t)0x4000u, &value) ==
           WZ_RESULT_OK);
    assert(value == (wz_byte_t)0xa5u);

    wz_snapshot_state_editor_unbind(&editor);
    assert(!wz_snapshot_state_editor_is_bound(&editor));
    assert(wz_snapshot_state_editor_write_memory(&editor, 0u, 0u) ==
           WZ_RESULT_INVALID_ARGUMENT);
    wz_machine_destroy(&machine);
    return 0;
}
