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
    machine->ula_output = 0u;
    machine->rom_identity = 0u;
    machine->master_tick = 0u;
    machine->ula_output_tick = 0u;
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
        machine->ula_output = 0u;
        machine->rom_identity = 0u;
        machine->master_tick = 0u;
        machine->ula_output_tick = 0u;
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

static wz_byte_t wz_contention_delay_at_tstate(wz_dword_t tstate)
{
    static const wz_byte_t delays[8u] = {6u, 5u, 4u, 3u, 2u, 1u, 0u, 0u};
    wz_dword_t frame_tstate = tstate % 69888u;
    wz_dword_t screen_offset;

    if (frame_tstate < 14335u) {
        return 0u;
    }
    screen_offset = (frame_tstate - 14335u) % 224u;
    if (screen_offset >= 128u) {
        return 0u;
    }
    return delays[(frame_tstate - 14335u) % 8u];
}

wz_byte_t wz_machine_contention_delay(const wz_machine_t* machine,
                                      wz_bus_cycle_t cycle,
                                      wz_word_t address,
                                      wz_master_tick_t master_tick,
                                      wz_byte_t t_states)
{
    wz_dword_t start_tstate;
    wz_byte_t delay = 0u;

    if (machine == 0 || machine->profile == 0 ||
        machine->profile->kind != WZ_MACHINE_48K_PAL) {
        return 0u;
    }
    start_tstate = (wz_dword_t)(master_tick / 2u);
    if (cycle == WZ_BUS_M1_OPCODE_FETCH || cycle == WZ_BUS_MEMORY_READ ||
        cycle == WZ_BUS_MEMORY_WRITE) {
        if (address >= 0x4000u && address <= 0x7fffu) {
            return wz_contention_delay_at_tstate(start_tstate);
        }
        return 0u;
    }
    if ((cycle != WZ_BUS_IO_READ && cycle != WZ_BUS_IO_WRITE) ||
        (address & 1u) != 0u || t_states == 0u) {
        return 0u;
    }
    if ((address & 0xff00u) >= 0x4000u && (address & 0xff00u) <= 0x7f00u) {
        for (wz_byte_t index = 0u; index < t_states; ++index) {
            delay = (wz_byte_t)(delay +
                wz_contention_delay_at_tstate(start_tstate + index));
        }
    } else {
        for (wz_byte_t index = 1u; index < t_states; ++index) {
            delay = (wz_byte_t)(delay +
                wz_contention_delay_at_tstate(start_tstate + index));
        }
    }
    return delay;
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
                                  wz_byte_t value, wz_master_tick_t master_tick)
{
    (void)address;
    if (machine == 0) {
        return;
    }
    machine->ula_output = (wz_byte_t)(value & 0x1fu);
    machine->ula_output_tick = master_tick;
    if (machine->timing_trace != 0) {
        wz_trace_event_t event = {0};
        event.kind = WZ_TRACE_DEVELOPER_MARKER;
        event.master_tick = master_tick;
        event.address = 0x00feu;
        event.value = machine->ula_output;
        event.auxiliary = 0x01u;
        wz_trace_emit_detail(machine->timing_trace, &event);
    }
}

const char* wz_machine_boot_message(void)
{
    return "Warajevo ZX Spectrum Next bootstrap";
}
