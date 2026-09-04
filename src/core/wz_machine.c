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
    wz_bus_data_source_init(&machine->bus_data_source, 0, 0);
    machine->timing_trace = 0;
    machine->has_48k_rom = 0u;
    machine->paging_7ffd = 0u;
    machine->paging_7ffd_locked = 0u;
    machine->hardware_io_decode_enabled = 1u;
    for (size_t index = 0u; index < 8u; ++index) {
        machine->keyboard_rows[index] = 0x1fu;
    }
    wz_kempston_init(&machine->kempston);
    machine->ula_output = 0u;
    wz_beeper_init(&machine->beeper);
    machine->tape.segments = 0;
    machine->tape.segment_count = 0u;
    machine->tape_mounted = 0u;
    machine->tape_loading_mode = WZ_TAPE_LOADING_NORMAL;
    machine->networking_mode = WZ_NETWORKING_NONE;
    machine->maskable_interrupt_line_low = 0u;
    machine->rom_identity = 0u;
    machine->master_tick = 0u;
    machine->ula_output_tick = 0u;
    machine->border_color = 0u;
    machine->border_event_count = 0u;
    machine->im0_injected_opcode = 0u;
    machine->im0_injected_opcode_pending = 0u;
    for (size_t index = 0u; index < sizeof(machine->memory); ++index) {
        machine->memory[index] = 0u;
    }
    for (size_t index = 0u; index < sizeof(machine->ram_128k); ++index) {
        ((wz_byte_t*)machine->ram_128k)[index] = 0u;
    }
    return WZ_RESULT_OK;
}

void wz_machine_destroy(wz_machine_t* machine)
{
    if (machine != 0) {
        machine->profile = 0;
        wz_bus_observer_init(&machine->bus_observer, 0, 0);
        wz_bus_input_init(&machine->bus_input, 0, 0);
        wz_bus_data_source_init(&machine->bus_data_source, 0, 0);
        machine->timing_trace = 0;
        machine->has_48k_rom = 0u;
        machine->paging_7ffd = 0u;
        machine->paging_7ffd_locked = 0u;
        machine->hardware_io_decode_enabled = 1u;
        for (size_t index = 0u; index < 8u; ++index) {
            machine->keyboard_rows[index] = 0x1fu;
        }
        wz_kempston_init(&machine->kempston);
        wz_beeper_init(&machine->beeper);
        machine->tape.segments = 0;
        machine->tape.segment_count = 0u;
        machine->tape_mounted = 0u;
        machine->tape_loading_mode = WZ_TAPE_LOADING_NORMAL;
        machine->networking_mode = WZ_NETWORKING_NONE;
        machine->ula_output = 0u;
        machine->maskable_interrupt_line_low = 0u;
        machine->rom_identity = 0u;
        machine->master_tick = 0u;
        machine->ula_output_tick = 0u;
        machine->border_color = 0u;
        machine->border_event_count = 0u;
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

wz_result_t wz_machine_mount_tape(wz_machine_t* machine,
                                  const wz_tape_segment_t* segments,
                                  size_t segment_count)
{
    if (machine == 0 ||
        wz_tape_mount(&machine->tape, segments, segment_count) != WZ_RESULT_OK ||
        wz_tape_state_init(&machine->tape_state, &machine->tape) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    machine->tape_mounted = 1u;
    return WZ_RESULT_OK;
}

wz_result_t wz_machine_unmount_tape(wz_machine_t* machine)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    machine->tape.segments = 0;
    machine->tape.segment_count = 0u;
    machine->tape_mounted = 0u;
    return WZ_RESULT_OK;
}

wz_result_t wz_machine_set_tape_motor(wz_machine_t* machine, bool motor_on)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (!machine->tape_mounted) {
        return WZ_RESULT_INVALID_STATE;
    }
    return wz_tape_state_set_motor(&machine->tape_state, motor_on);
}

wz_result_t wz_machine_rewind_tape(wz_machine_t* machine)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (!machine->tape_mounted) {
        return WZ_RESULT_INVALID_STATE;
    }
    return wz_tape_state_rewind(&machine->tape_state);
}

wz_result_t wz_machine_advance_tape(wz_machine_t* machine,
                                    wz_master_tick_t ticks)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (!machine->tape_mounted) {
        return WZ_RESULT_OK;
    }
    return wz_tape_state_advance(&machine->tape_state, ticks);
}

wz_result_t wz_machine_set_tape_loading_mode(wz_machine_t* machine,
                                              wz_tape_loading_mode_t mode)
{
    if (machine == 0 || mode > WZ_TAPE_LOADING_INSTANT_TRAP) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    machine->tape_loading_mode = mode;
    return WZ_RESULT_OK;
}

wz_tape_loading_mode_t wz_machine_tape_loading_mode(const wz_machine_t* machine)
{
    return machine == 0 ? WZ_TAPE_LOADING_NORMAL : machine->tape_loading_mode;
}

wz_result_t wz_machine_set_networking_mode(wz_machine_t* machine,
                                            wz_networking_mode_t mode)
{
    if (machine == 0 || mode > WZ_NETWORKING_EAR_MIC) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    machine->networking_mode = mode;
    return WZ_RESULT_OK;
}

wz_networking_mode_t wz_machine_networking_mode(const wz_machine_t* machine)
{
    return machine == 0 ? WZ_NETWORKING_NONE : machine->networking_mode;
}

wz_tape_trap_reason_t wz_machine_tape_trap_reason(const wz_machine_t* machine)
{
    if (machine == 0 || machine->tape_loading_mode == WZ_TAPE_LOADING_NORMAL) {
        return WZ_TAPE_TRAP_REASON_NORMAL_MODE;
    }
    if (machine->tape_mounted == 0u) {
        return WZ_TAPE_TRAP_REASON_NO_TAPE;
    }
    if (machine->has_48k_rom == 0u) {
        return WZ_TAPE_TRAP_REASON_NO_ROM;
    }
    return WZ_TAPE_TRAP_REASON_NO_RECOGNIZED_LOADER;
}

wz_byte_t wz_machine_tape_ear_level(const wz_machine_t* machine)
{
    if (machine == 0 || !machine->tape_mounted) {
        return 0u;
    }
    return wz_tape_state_ear_level(&machine->tape_state);
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

wz_result_t wz_machine_set_kempston_control(wz_machine_t* machine,
                                             wz_kempston_control_t control,
                                             bool pressed)
{
    if (machine == 0 ||
        !wz_kempston_set(&machine->kempston, control, pressed)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    return WZ_RESULT_OK;
}

wz_byte_t wz_machine_kempston_read(const wz_machine_t* machine,
                                   wz_word_t address)
{
    if (machine == 0) {
        return 0u;
    }
    return wz_kempston_read(&machine->kempston, address);
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
    if (machine == 0) {
        return 0xffu;
    }
    if (machine->profile != 0 && machine->profile->kind == WZ_MACHINE_128K_PAL) {
        if (address >= 0xc000u) {
            return machine->ram_128k[machine->paging_7ffd & 0x07u][address - 0xc000u];
        }
        if (address >= 0x8000u) {
            return machine->ram_128k[2u][address - 0x8000u];
        }
        if (address >= 0x4000u) {
            return machine->ram_128k[5u][address - 0x4000u];
        }
    }
    return machine->memory[address];
}

void wz_machine_memory_write(wz_machine_t* machine, wz_word_t address,
                             wz_byte_t value)
{
    if (machine != 0 && machine->profile != 0 &&
        machine->profile->kind == WZ_MACHINE_128K_PAL) {
        if (address >= 0xc000u) {
            machine->ram_128k[machine->paging_7ffd & 0x07u][address - 0xc000u] = value;
        } else if (address >= 0x8000u) {
            machine->ram_128k[2u][address - 0x8000u] = value;
        } else if (address >= 0x4000u) {
            machine->ram_128k[5u][address - 0x4000u] = value;
        }
    } else if (machine != 0 &&
               (!machine->has_48k_rom || address >= WZ_48K_ROM_SIZE)) {
        machine->memory[address] = value;
    }
}

wz_result_t wz_machine_128k_paging_write(wz_machine_t* machine,
                                         wz_word_t address,
                                         wz_byte_t value)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (machine->profile == 0 || machine->profile->kind != WZ_MACHINE_128K_PAL) {
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
    if ((address & 0x8002u) != 0u) {
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
    if (machine->paging_7ffd_locked != 0u) {
        return WZ_RESULT_OK;
    }
    machine->paging_7ffd = (wz_byte_t)(value & 0x3fu);
    machine->paging_7ffd_locked = (wz_byte_t)((value >> 5u) & 1u);
    return WZ_RESULT_OK;
}

wz_byte_t wz_machine_128k_paging_value(const wz_machine_t* machine)
{
    return machine == 0 ? 0u : machine->paging_7ffd;
}

wz_byte_t wz_machine_128k_screen_bank(const wz_machine_t* machine)
{
    return machine == 0 ? 5u : (wz_byte_t)(((machine->paging_7ffd >> 3u) & 1u) != 0u ? 7u : 5u);
}

wz_byte_t wz_machine_128k_rom_bank(const wz_machine_t* machine)
{
    return machine == 0 ? 0u : (wz_byte_t)((machine->paging_7ffd >> 4u) & 1u);
}

void wz_machine_memory_write_at_tick(wz_machine_t* machine, wz_word_t address,
                                     wz_byte_t value, wz_master_tick_t master_tick)
{
    (void)master_tick;
    /* CPU writes are applied before a same-tick ULA fetch by the scheduler contract. */
    wz_machine_memory_write(machine, address, value);
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
    value = (wz_byte_t)((value & 0xbfu) |
                        (wz_byte_t)(wz_machine_tape_ear_level(machine) << 6u));
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
    wz_beeper_port_fe_write(&machine->beeper, value, master_tick);
    machine->border_color = (wz_byte_t)(value & 0x07u);
    if (machine->border_event_count < WZ_BORDER_EVENT_CAPACITY) {
        machine->border_events[machine->border_event_count].master_tick = master_tick;
        machine->border_events[machine->border_event_count].color = machine->border_color;
        machine->border_event_count += 1u;
    }
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

wz_byte_t wz_machine_beeper_level(const wz_machine_t* machine)
{
    return machine == 0 ? 0u : wz_beeper_level(&machine->beeper);
}

wz_byte_t wz_machine_mic_level(const wz_machine_t* machine)
{
    return machine == 0 ? 0u : wz_beeper_mic_level(&machine->beeper);
}

size_t wz_machine_beeper_events(const wz_machine_t* machine,
                                wz_beeper_event_t* events,
                                size_t capacity)
{
    return machine == 0 ? 0u : wz_beeper_events(&machine->beeper, events, capacity);
}

wz_result_t wz_machine_mic_capture_begin(wz_machine_t* machine)
{
    return machine == 0 ? WZ_RESULT_INVALID_ARGUMENT :
        wz_beeper_mic_capture_begin(&machine->beeper);
}

wz_result_t wz_machine_mic_capture_end(wz_machine_t* machine)
{
    return machine == 0 ? WZ_RESULT_INVALID_ARGUMENT :
        wz_beeper_mic_capture_end(&machine->beeper);
}

size_t wz_machine_mic_events(const wz_machine_t* machine,
                             wz_mic_event_t* events,
                             size_t capacity)
{
    return machine == 0 ? 0u : wz_beeper_mic_events(&machine->beeper, events, capacity);
}

bool wz_machine_mic_capture_overflowed(const wz_machine_t* machine)
{
    return machine != 0 && wz_beeper_mic_capture_overflowed(&machine->beeper);
}

wz_byte_t wz_machine_border_color(const wz_machine_t* machine)
{
    return machine == 0 ? 0u : machine->border_color;
}

size_t wz_machine_border_events(const wz_machine_t* machine,
                                wz_border_event_t* events, size_t capacity)
{
    size_t count;

    if (machine == 0 || (events == 0 && capacity != 0u)) {
        return 0u;
    }
    count = machine->border_event_count < capacity ?
        machine->border_event_count : capacity;
    if (events != 0) {
        for (size_t index = 0u; index < count; ++index) {
            events[index] = machine->border_events[index];
        }
    }
    return count;
}

void wz_machine_update_interrupt_line(wz_machine_t* machine)
{
    wz_dword_t frame_tstate;
    wz_dword_t assert_tstate;
    wz_dword_t deassert_tstate;
    bool should_be_low;

    if (machine == 0 || machine->profile == 0 ||
        machine->profile->master_ticks_per_cpu_tstate == 0u ||
        machine->profile->tstates_per_frame == 0u) {
        return;
    }
    frame_tstate = (wz_dword_t)((machine->master_tick /
        machine->profile->master_ticks_per_cpu_tstate) %
        machine->profile->tstates_per_frame);
    assert_tstate = machine->profile->interrupt_assert_tstate %
        machine->profile->tstates_per_frame;
    deassert_tstate = machine->profile->interrupt_deassert_tstate %
        machine->profile->tstates_per_frame;
    if (assert_tstate < deassert_tstate) {
        should_be_low = frame_tstate >= assert_tstate &&
            frame_tstate < deassert_tstate;
    } else {
        should_be_low = frame_tstate >= assert_tstate ||
            frame_tstate < deassert_tstate;
    }
    if ((machine->maskable_interrupt_line_low != 0u) == should_be_low) {
        return;
    }
    machine->maskable_interrupt_line_low = should_be_low ? 1u : 0u;
    if (machine->timing_trace != 0) {
        wz_trace_event_t event = {0};
        event.kind = WZ_TRACE_INTERRUPT;
        event.master_tick = machine->master_tick;
        event.value = should_be_low ? WZ_TRACE_INTERRUPT_LINE_ASSERT :
            WZ_TRACE_INTERRUPT_LINE_DEASSERT;
        wz_trace_emit_detail(machine->timing_trace, &event);
    }
}

bool wz_machine_maskable_interrupt_line_low(const wz_machine_t* machine)
{
    return machine != 0 && machine->maskable_interrupt_line_low != 0u;
}

wz_result_t wz_machine_raster_position(const wz_machine_t* machine,
                                       wz_raster_position_t* position)
{
    wz_qword_t raster_clocks_per_frame;
    wz_qword_t frame_raster_clock;

    if (machine == 0 || position == 0 || machine->profile == 0 ||
        machine->profile->raster_clocks_per_line == 0u ||
        machine->profile->lines_per_frame == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    raster_clocks_per_frame =
        (wz_qword_t)machine->profile->raster_clocks_per_line *
        (wz_qword_t)machine->profile->lines_per_frame;
    position->frame_number = machine->master_tick / raster_clocks_per_frame;
    frame_raster_clock = machine->master_tick % raster_clocks_per_frame;
    position->frame_raster_clock = (wz_dword_t)frame_raster_clock;
    position->line = (wz_dword_t)(frame_raster_clock /
                                  machine->profile->raster_clocks_per_line);
    position->raster_clock = (wz_dword_t)(frame_raster_clock %
                                          machine->profile->raster_clocks_per_line);
    return WZ_RESULT_OK;
}

static wz_word_t wz_ula_bitmap_address(wz_dword_t row, wz_dword_t cell)
{
    wz_dword_t address = 0x4000u;
    address |= (row & 0xc0u) << 5u;
    address |= (row & 0x07u) << 8u;
    address |= (row & 0x38u) << 2u;
    address |= cell;
    return (wz_word_t)address;
}

wz_result_t wz_machine_ula_fetches_at_tick(const wz_machine_t* machine,
                                           wz_master_tick_t master_tick,
                                           wz_ula_fetch_event_t* events,
                                           size_t capacity,
                                           size_t* count)
{
    const wz_machine_profile_t* profile;
    wz_dword_t tstate;
    wz_dword_t elapsed;
    wz_dword_t line;
    wz_dword_t cell;
    wz_dword_t row;
    wz_master_tick_t bitmap_tick;
    wz_master_tick_t attribute_tick;

    if (count == 0 || machine == 0 || machine->profile == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *count = 0u;
    profile = machine->profile;
    if (profile->kind != WZ_MACHINE_48K_PAL ||
        profile->master_ticks_per_cpu_tstate == 0u ||
        profile->ula_fetch_line_count == 0u ||
        profile->ula_fetches_per_line == 0u ||
        profile->ula_fetch_interval_tstates == 0u) {
        return WZ_RESULT_OK;
    }
    tstate = (wz_dword_t)(master_tick / profile->master_ticks_per_cpu_tstate);
    if (tstate < profile->ula_fetch_start_tstate) {
        return WZ_RESULT_OK;
    }
    elapsed = tstate - profile->ula_fetch_start_tstate;
    line = elapsed / profile->tstates_per_line;
    if (line >= profile->ula_fetch_line_count) {
        return WZ_RESULT_OK;
    }
    cell = (elapsed % profile->tstates_per_line) /
        profile->ula_fetch_interval_tstates;
    if (cell >= profile->ula_fetches_per_line ||
        (elapsed % profile->ula_fetch_interval_tstates) != 0u) {
        return WZ_RESULT_OK;
    }
    bitmap_tick = (wz_master_tick_t)tstate *
        profile->master_ticks_per_cpu_tstate;
    attribute_tick = bitmap_tick + profile->ula_attribute_offset_tstates *
        profile->master_ticks_per_cpu_tstate;
    row = line;
    if (capacity < 2u || events == 0) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    events[0].kind = WZ_ULA_FETCH_BITMAP;
    events[0].master_tick = bitmap_tick;
    events[0].address = wz_ula_bitmap_address(row, cell);
    events[0].value = machine->memory[events[0].address];
    events[1].kind = WZ_ULA_FETCH_ATTRIBUTE;
    events[1].master_tick = attribute_tick;
    events[1].address = (wz_word_t)(0x5800u + (row / 8u) * 32u + cell);
    events[1].value = machine->memory[events[1].address];
    *count = 2u;
    return WZ_RESULT_OK;
}

wz_byte_t wz_machine_floating_bus_value(const wz_machine_t* machine,
                                        wz_master_tick_t master_tick)
{
    wz_ula_fetch_event_t events[2u];
    size_t count = 0u;

    if (wz_machine_ula_fetches_at_tick(machine, master_tick, events,
                                       sizeof(events) / sizeof(events[0u]),
                                       &count) == WZ_RESULT_OK && count != 0u) {
        return events[0u].value;
    }
    if (master_tick < 2u ||
        wz_machine_ula_fetches_at_tick(machine, master_tick - 2u, events,
                                       sizeof(events) / sizeof(events[0u]),
                                       &count) != WZ_RESULT_OK || count == 0u ||
        events[1u].master_tick != master_tick) {
        return 0xffu;
    }
    return events[1u].value;
}

bool wz_machine_flash_phase(const wz_machine_t* machine,
                            wz_master_tick_t master_tick)
{
    wz_qword_t frame_ticks;
    wz_qword_t flash_period_ticks;

    if (machine == 0 || machine->profile == 0 ||
        machine->profile->master_ticks_per_cpu_tstate == 0u ||
        machine->profile->tstates_per_frame == 0u) {
        return false;
    }
    frame_ticks = (wz_qword_t)machine->profile->tstates_per_frame *
        machine->profile->master_ticks_per_cpu_tstate;
    flash_period_ticks = frame_ticks * 16u;
    if (flash_period_ticks == 0u) {
        return false;
    }
    return ((master_tick / flash_period_ticks) & 1u) != 0u;
}

const char* wz_machine_boot_message(void)
{
    return "Warajevo ZX Spectrum Next bootstrap";
}
