/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_debugger.h"

#include <stdint.h>

wz_result_t wz_debugger_snapshot(const wz_machine_t* machine,
                                 wz_debugger_snapshot_t* snapshot)
{
    if (machine == 0 || snapshot == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    snapshot->cpu = machine->cpu;
    snapshot->master_tick = machine->master_tick;
    snapshot->border_color = machine->border_color;
    snapshot->networking_mode = (wz_byte_t)machine->networking_mode;
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_read_memory(const wz_machine_t* machine,
                                    wz_word_t address,
                                    wz_byte_t* value)
{
    if (machine == 0 || value == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *value = wz_machine_memory_read(machine, address);
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_read_memory_block(const wz_machine_t* machine,
                                          wz_word_t address,
                                          wz_byte_t* values,
                                          size_t length)
{
    if (machine == 0 || (length > 0u && values == 0)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length > (size_t)UINT16_MAX + 1u - (size_t)address) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    for (size_t index = 0u; index < length; ++index) {
        values[index] = wz_machine_memory_read(machine,
                                               (wz_word_t)(address + index));
    }
    return WZ_RESULT_OK;
}
