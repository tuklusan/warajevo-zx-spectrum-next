/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_snapshot_inspector.h"

#include <stdio.h>
#include <string.h>

static const char* wz_snapshot_inspector_format_name = "live-machine";
static const char* wz_snapshot_inspector_format_version = "runtime";

void wz_snapshot_inspector_init(wz_snapshot_inspector_t* inspector)
{
    if (inspector == 0) {
        return;
    }
    memset(inspector, 0, sizeof(*inspector));
}

wz_snapshot_inspector_result_t wz_snapshot_inspector_open(
    wz_snapshot_inspector_t* inspector,
    const wz_machine_t* machine)
{
    wz_result_t result;

    if (inspector == 0 || machine == 0) {
        return WZ_SNAPSHOT_INSPECTOR_INVALID_ARGUMENT;
    }
    result = wz_debugger_snapshot(machine, &inspector->machine);
    if (result != WZ_RESULT_OK ||
        wz_debugger_read_page_info(machine, &inspector->paging) != WZ_RESULT_OK) {
        wz_snapshot_inspector_close(inspector);
        return WZ_SNAPSHOT_INSPECTOR_DEBUGGER_UNAVAILABLE;
    }
    inspector->format_name = wz_snapshot_inspector_format_name;
    inspector->format_version = wz_snapshot_inspector_format_version;
    inspector->model_kind = machine->profile->kind;
    inspector->model_name = machine->profile->name;
    inspector->ay_selected_register = wz_machine_ay_selected_register(machine);
    for (size_t index = 0u; index < WZ_AY_REGISTER_COUNT; ++index) {
        inspector->ay_registers[index] = wz_machine_ay_register_value(machine,
                                                                        (wz_byte_t)index);
    }
    inspector->memory_page_count = machine->ram_128k != 0
        ? WZ_128K_RAM_BANK_COUNT : 3u;
    for (size_t index = 0u; index < inspector->memory_page_count; ++index) {
        inspector->memory_pages[index] = 1u;
    }
    inspector->open = true;
    return WZ_SNAPSHOT_INSPECTOR_OK;
}

void wz_snapshot_inspector_close(wz_snapshot_inspector_t* inspector)
{
    if (inspector == 0) {
        return;
    }
    memset(inspector, 0, sizeof(*inspector));
}

bool wz_snapshot_inspector_is_open(const wz_snapshot_inspector_t* inspector)
{
    return inspector != 0 && inspector->open;
}

const wz_debugger_snapshot_t* wz_snapshot_inspector_machine(
    const wz_snapshot_inspector_t* inspector)
{
    return wz_snapshot_inspector_is_open(inspector) ? &inspector->machine : 0;
}

const wz_debugger_page_info_t* wz_snapshot_inspector_paging(
    const wz_snapshot_inspector_t* inspector)
{
    return wz_snapshot_inspector_is_open(inspector) ? &inspector->paging : 0;
}

const wz_snapshot_inspector_t* wz_snapshot_inspector_metadata(
    const wz_snapshot_inspector_t* inspector)
{
    return wz_snapshot_inspector_is_open(inspector) ? inspector : 0;
}

wz_result_t wz_snapshot_inspector_format(const wz_snapshot_inspector_t* inspector,
                                        char* output, size_t capacity)
{
    int written;

    if (!wz_snapshot_inspector_is_open(inspector) || output == 0 || capacity == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    written = snprintf(output, capacity,
                       "Format: %s/%s\nModel: %s\n"
                       "Registers: A=%02X F=%02X B=%02X C=%02X D=%02X E=%02X H=%02X L=%02X "
                       "A'=%02X F'=%02X B'=%02X C'=%02X D'=%02X E'=%02X H'=%02X L'=%02X\n"
                       "PC=%04X SP=%04X IX=%04X IY=%04X MEMPTR=%04X I=%02X R=%02X "
                       "IFF1=%u IFF2=%u IM=%u HALT=%u\n"
                       "Paging: %02X screen=%u rom=%u locked=%u\n"
                       "AY: selected=%u registers="
                       "%02X %02X %02X %02X %02X %02X %02X %02X "
                       "%02X %02X %02X %02X %02X %02X %02X %02X\n"
                       "Memory pages: count=%u present="
                       "%u %u %u %u %u %u %u %u\nWarnings: %u\n",
                       inspector->format_name, inspector->format_version,
                       inspector->model_name,
                       (unsigned)inspector->machine.cpu.main.a,
                       (unsigned)inspector->machine.cpu.main.f,
                       (unsigned)inspector->machine.cpu.main.b,
                       (unsigned)inspector->machine.cpu.main.c,
                       (unsigned)inspector->machine.cpu.main.d,
                       (unsigned)inspector->machine.cpu.main.e,
                       (unsigned)inspector->machine.cpu.main.h,
                       (unsigned)inspector->machine.cpu.main.l,
                       (unsigned)inspector->machine.cpu.alternate.a,
                       (unsigned)inspector->machine.cpu.alternate.f,
                       (unsigned)inspector->machine.cpu.alternate.b,
                       (unsigned)inspector->machine.cpu.alternate.c,
                       (unsigned)inspector->machine.cpu.alternate.d,
                       (unsigned)inspector->machine.cpu.alternate.e,
                       (unsigned)inspector->machine.cpu.alternate.h,
                       (unsigned)inspector->machine.cpu.alternate.l,
                       (unsigned)inspector->machine.cpu.program_counter,
                       (unsigned)inspector->machine.cpu.stack_pointer,
                       (unsigned)inspector->machine.cpu.ix,
                       (unsigned)inspector->machine.cpu.iy,
                       (unsigned)inspector->machine.cpu.memptr,
                       (unsigned)inspector->machine.cpu.i,
                       (unsigned)inspector->machine.cpu.r,
                       (unsigned)inspector->machine.cpu.iff1,
                       (unsigned)inspector->machine.cpu.iff2,
                       (unsigned)inspector->machine.cpu.interrupt_mode,
                       (unsigned)inspector->machine.cpu.halted,
                       (unsigned)inspector->paging.paging_value,
                       (unsigned)inspector->paging.screen_bank,
                       (unsigned)inspector->paging.rom_bank,
                       (unsigned)inspector->paging.paging_locked,
                       (unsigned)inspector->ay_selected_register,
                       (unsigned)inspector->ay_registers[0u],
                       (unsigned)inspector->ay_registers[1u],
                       (unsigned)inspector->ay_registers[2u],
                       (unsigned)inspector->ay_registers[3u],
                       (unsigned)inspector->ay_registers[4u],
                       (unsigned)inspector->ay_registers[5u],
                       (unsigned)inspector->ay_registers[6u],
                       (unsigned)inspector->ay_registers[7u],
                       (unsigned)inspector->ay_registers[8u],
                       (unsigned)inspector->ay_registers[9u],
                       (unsigned)inspector->ay_registers[10u],
                       (unsigned)inspector->ay_registers[11u],
                       (unsigned)inspector->ay_registers[12u],
                       (unsigned)inspector->ay_registers[13u],
                       (unsigned)inspector->ay_registers[14u],
                       (unsigned)inspector->ay_registers[15u],
                       (unsigned)inspector->memory_page_count,
                       (unsigned)inspector->memory_pages[0u],
                       (unsigned)inspector->memory_pages[1u],
                       (unsigned)inspector->memory_pages[2u],
                       (unsigned)inspector->memory_pages[3u],
                       (unsigned)inspector->memory_pages[4u],
                       (unsigned)inspector->memory_pages[5u],
                       (unsigned)inspector->memory_pages[6u],
                       (unsigned)inspector->memory_pages[7u],
                       (unsigned)inspector->warning_count);
    if (written < 0 || (size_t)written >= capacity) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    return WZ_RESULT_OK;
}
