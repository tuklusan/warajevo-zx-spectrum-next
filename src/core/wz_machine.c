/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_machine.h"

wz_result_t wz_machine_init(wz_machine_t* machine,
                            const wz_machine_profile_t* profile)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (profile == 0) {
        return WZ_RESULT_INVALID_PROFILE;
    }

    machine->profile = profile;
    wz_z80_state_init(&machine->cpu);
    wz_bus_observer_init(&machine->bus_observer, 0, 0);
    wz_bus_input_init(&machine->bus_input, 0, 0);
    machine->timing_trace = 0;
    machine->has_48k_rom = 0u;
    machine->master_tick = 0u;
    machine->im0_injected_opcode = 0u;
    machine->im0_injected_opcode_pending = 0u;
    for (size_t index = 0u; index < sizeof(machine->memory); ++index) {
        machine->memory[index] = 0u;
    }
    return WZ_RESULT_OK;
}

void wz_machine_destroy(wz_machine_t* machine)
{
    if (machine != 0) {
        machine->profile = 0;
        wz_bus_observer_init(&machine->bus_observer, 0, 0);
        wz_bus_input_init(&machine->bus_input, 0, 0);
        machine->timing_trace = 0;
        machine->has_48k_rom = 0u;
        machine->master_tick = 0u;
        machine->im0_injected_opcode = 0u;
        machine->im0_injected_opcode_pending = 0u;
    }
}

void wz_machine_set_timing_trace(wz_machine_t* machine, wz_trace_sink_t* trace)
{
    if (machine != 0) {
        machine->timing_trace = trace;
    }
}

wz_result_t wz_machine_load_48k_rom(wz_machine_t* machine,
                                    const wz_byte_t* bytes,
                                    size_t length)
{
    if (machine == 0 || bytes == 0 || length != WZ_48K_ROM_SIZE) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (size_t index = 0u; index < WZ_48K_ROM_SIZE; ++index) {
        machine->memory[index] = bytes[index];
    }
    machine->has_48k_rom = 1u;
    return WZ_RESULT_OK;
}

wz_byte_t wz_machine_memory_read(const wz_machine_t* machine, wz_word_t address)
{
    return machine == 0 ? 0xffu : machine->memory[address];
}

void wz_machine_memory_write(wz_machine_t* machine, wz_word_t address,
                             wz_byte_t value)
{
    if (machine != 0 &&
        (!machine->has_48k_rom || address >= WZ_48K_ROM_SIZE)) {
        machine->memory[address] = value;
    }
}

const char* wz_machine_boot_message(void)
{
    return "Warajevo ZX Spectrum Next bootstrap";
}
