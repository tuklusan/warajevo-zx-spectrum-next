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
    machine->hardware_io_decode_enabled = 1u;
    for (size_t index = 0u; index < 8u; ++index) {
        machine->keyboard_rows[index] = 0x1fu;
    }
    machine->rom_identity = 0u;
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
        machine->hardware_io_decode_enabled = 1u;
        for (size_t index = 0u; index < 8u; ++index) {
            machine->keyboard_rows[index] = 0x1fu;
        }
        machine->rom_identity = 0u;
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

wz_result_t wz_machine_set_hardware_io_decode(wz_machine_t* machine, bool enabled)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    machine->hardware_io_decode_enabled = enabled ? 1u : 0u;
    return WZ_RESULT_OK;
}

wz_result_t wz_machine_set_keyboard_key(wz_machine_t* machine,
                                        wz_byte_t row,
                                        wz_byte_t key,
                                        bool pressed)
{
    wz_byte_t mask;

    if (machine == 0 || row >= 8u || key >= 5u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    mask = (wz_byte_t)(1u << key);
    if (pressed) {
        machine->keyboard_rows[row] &= (wz_byte_t)~mask;
    } else {
        machine->keyboard_rows[row] |= mask;
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_machine_load_48k_rom(wz_machine_t* machine,
                                    const wz_byte_t* bytes,
                                    size_t length)
{
    wz_qword_t identity;

    if (machine == 0 || bytes == 0 || length != WZ_48K_ROM_SIZE) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (machine->profile == 0 || machine->profile->expected_rom_identity == 0u) {
        return WZ_RESULT_INVALID_PROFILE;
    }
    identity = wz_machine_rom_identity(bytes, length);
    if (identity != machine->profile->expected_rom_identity) {
        return WZ_RESULT_ROM_IDENTITY_MISMATCH;
    }
    for (size_t index = 0u; index < WZ_48K_ROM_SIZE; ++index) {
        machine->memory[index] = bytes[index];
    }
    machine->has_48k_rom = 1u;
    machine->rom_identity = identity;
    return WZ_RESULT_OK;
}

wz_qword_t wz_machine_rom_identity(const wz_byte_t* bytes, size_t length)
{
    wz_qword_t identity = UINT64_C(14695981039346656037);

    if (bytes == 0 || length != WZ_48K_ROM_SIZE) {
        return 0u;
    }
    for (size_t index = 0u; index < length; ++index) {
        identity ^= (wz_qword_t)bytes[index];
        identity *= UINT64_C(1099511628211);
    }
    return identity;
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

bool wz_machine_ula_port_fe_selected(wz_word_t address)
{
    return (address & 1u) == 0u;
}

wz_byte_t wz_machine_ula_port_fe_read(const wz_machine_t* machine,
                                       wz_word_t address)
{
    wz_byte_t value = 0xbfu;

    if (machine == 0) {
        return 0xffu;
    }
    for (size_t row = 0u; row < 8u; ++row) {
        if ((address & (wz_word_t)(1u << (8u + row))) == 0u) {
            value = (wz_byte_t)((value & 0xe0u) |
                                ((value & 0x1fu) & machine->keyboard_rows[row]));
        }
    }
    return value;
}

void wz_machine_ula_port_fe_write(wz_machine_t* machine, wz_word_t address,
                                  wz_byte_t value)
{
    (void)machine;
    (void)address;
    (void)value;
}

const char* wz_machine_boot_message(void)
{
    return "Warajevo ZX Spectrum Next bootstrap";
}
