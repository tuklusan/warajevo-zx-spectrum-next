/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_state.h"

#include "core/wz_machine.h"

#define WZ_STATE_VERSION 12u
#define WZ_STATE_HEADER_LENGTH 113u
#define WZ_STATE_MACHINE_LENGTH (65536u + WZ_STATE_HEADER_LENGTH)

static wz_result_t wz_state_write(wz_state_writer_t* writer,
                                  const wz_byte_t* data,
                                  size_t length)
{
    if (writer == 0 || (length > 0u && data == 0)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (writer->length > writer->capacity ||
        length > writer->capacity - writer->length) {
        return WZ_RESULT_SERIALIZATION_FAILURE;
    }

    for (size_t index = 0u; index < length; ++index) {
        writer->data[writer->length + index] = data[index];
    }
    writer->length += length;
    return WZ_RESULT_OK;
}

static wz_result_t wz_state_write_u8(wz_state_writer_t* writer, wz_byte_t value)
{
    return wz_state_write(writer, &value, 1u);
}

static wz_result_t wz_state_write_u16(wz_state_writer_t* writer, wz_word_t value)
{
    wz_byte_t bytes[2];
    wz_write_le16(bytes, value);
    return wz_state_write(writer, bytes, sizeof(bytes));
}

static wz_result_t wz_state_write_z80_bank(wz_state_writer_t* writer,
                                           const wz_z80_register_bank_t* bank)
{
    if (bank == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (wz_state_write_u8(writer, bank->a) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, bank->f) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, bank->b) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, bank->c) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, bank->d) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, bank->e) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, bank->h) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, bank->l) != WZ_RESULT_OK) {
        return WZ_RESULT_SERIALIZATION_FAILURE;
    }
    return WZ_RESULT_OK;
}

static wz_result_t wz_state_write_u64(wz_state_writer_t* writer, wz_qword_t value)
{
    wz_byte_t bytes[8];
    for (size_t index = 0u; index < sizeof(bytes); ++index) {
        bytes[index] = (wz_byte_t)(value >> (index * 8u));
    }
    return wz_state_write(writer, bytes, sizeof(bytes));
}

static wz_result_t wz_state_write_u32(wz_state_writer_t* writer, wz_dword_t value)
{
    wz_byte_t bytes[4];
    for (size_t index = 0u; index < sizeof(bytes); ++index) {
        bytes[index] = (wz_byte_t)(value >> (index * 8u));
    }
    return wz_state_write(writer, bytes, sizeof(bytes));
}

static wz_result_t wz_state_write_ay(wz_state_writer_t* writer,
                                     const wz_ay_t* ay)
{
    if (writer == 0 || ay == 0 || ay->selected_register >= WZ_AY_REGISTER_COUNT ||
        ay->noise_level > 1u || ay->noise_lfsr > 0x1ffffu ||
        ay->envelope_level >= WZ_AY_ENVELOPE_LEVEL_COUNT ||
        ay->envelope_attack > 1u || ay->envelope_holding > 1u ||
        ay->tone_master_tick_phase >= WZ_AY_MASTER_TICKS_PER_CLOCK) {
        return WZ_RESULT_INVALID_STATE;
    }
    for (wz_byte_t channel = 0u; channel < WZ_AY_CHANNEL_COUNT; ++channel) {
        if (ay->tone_levels[channel] > 1u) {
            return WZ_RESULT_INVALID_STATE;
        }
    }
    if (wz_state_write_u8(writer, ay->selected_register) != WZ_RESULT_OK ||
        wz_state_write(writer, ay->registers, sizeof(ay->registers)) != WZ_RESULT_OK) {
        return WZ_RESULT_SERIALIZATION_FAILURE;
    }
    for (wz_byte_t channel = 0u; channel < WZ_AY_CHANNEL_COUNT; ++channel) {
        if (wz_state_write_u16(writer, ay->tone_counters[channel]) != WZ_RESULT_OK) {
            return WZ_RESULT_SERIALIZATION_FAILURE;
        }
    }
    if (wz_state_write(writer, ay->tone_levels, sizeof(ay->tone_levels)) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, ay->noise_counter) != WZ_RESULT_OK ||
        wz_state_write_u32(writer, ay->noise_lfsr) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, ay->noise_level) != WZ_RESULT_OK ||
        wz_state_write_u16(writer, ay->envelope_counter) != WZ_RESULT_OK ||
        wz_state_write_u16(writer, ay->envelope_period) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, ay->envelope_level) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, ay->envelope_attack) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, ay->envelope_holding) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, ay->tone_master_tick_phase) != WZ_RESULT_OK) {
        return WZ_RESULT_SERIALIZATION_FAILURE;
    }
    return WZ_RESULT_OK;
}

static bool wz_state_ay_data_valid(const wz_byte_t* data)
{
    size_t offset = 73u;
    wz_dword_t noise_lfsr = 0u;

    if (data[offset++] >= WZ_AY_REGISTER_COUNT) {
        return false;
    }
    offset += sizeof(((wz_ay_t*)0)->registers);
    offset += WZ_AY_CHANNEL_COUNT * sizeof(wz_word_t);
    for (size_t index = 0u; index < WZ_AY_CHANNEL_COUNT; ++index) {
        if (data[offset++] > 1u) {
            return false;
        }
    }
    ++offset;
    for (size_t index = 0u; index < sizeof(noise_lfsr); ++index) {
        noise_lfsr |= (wz_dword_t)data[offset++] << (index * 8u);
    }
    if (noise_lfsr > 0x1ffffu || data[offset++] > 1u) {
        return false;
    }
    offset += 2u;
    offset += 2u;
    if (data[offset++] >= WZ_AY_ENVELOPE_LEVEL_COUNT ||
        data[offset++] > 1u || data[offset++] > 1u ||
        data[offset] >= WZ_AY_MASTER_TICKS_PER_CLOCK) {
        return false;
    }
    return true;
}

void wz_state_writer_init(wz_state_writer_t* writer,
                          wz_byte_t* data,
                          size_t capacity)
{
    if (writer != 0) {
        writer->data = data;
        writer->capacity = capacity;
        writer->length = 0u;
    }
}

wz_result_t wz_state_serialize_machine(const wz_machine_t* machine,
                                       wz_state_writer_t* writer)
{
    if (machine == 0 || writer == 0 || writer->data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (machine->profile == 0) {
        return WZ_RESULT_INVALID_PROFILE;
    }
    if (wz_z80_state_validate(&machine->cpu) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (machine->networking_mode > WZ_NETWORKING_EAR_MIC) {
        return WZ_RESULT_INVALID_STATE;
    }

    if (wz_state_write_u8(writer, WZ_STATE_VERSION) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, (wz_byte_t)machine->profile->kind) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, machine->has_48k_rom) != WZ_RESULT_OK ||
        wz_state_write_u64(writer, machine->rom_identity) != WZ_RESULT_OK ||
        wz_state_write_z80_bank(writer, &machine->cpu.main) != WZ_RESULT_OK ||
        wz_state_write_z80_bank(writer, &machine->cpu.alternate) != WZ_RESULT_OK ||
        wz_state_write_u16(writer, machine->cpu.ix) != WZ_RESULT_OK ||
        wz_state_write_u16(writer, machine->cpu.iy) != WZ_RESULT_OK ||
        wz_state_write_u16(writer, machine->cpu.stack_pointer) != WZ_RESULT_OK ||
        wz_state_write_u16(writer, machine->cpu.program_counter) != WZ_RESULT_OK ||
        wz_state_write_u16(writer, machine->cpu.memptr) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, machine->cpu.i) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, machine->cpu.r) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, machine->cpu.iff1) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, machine->cpu.iff2) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, machine->cpu.interrupt_enable_delay) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, machine->cpu.interrupt_mode) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, machine->cpu.halted) != WZ_RESULT_OK ||
        wz_state_write_u64(writer, machine->master_tick) != WZ_RESULT_OK ||
        wz_state_write(writer, machine->keyboard_rows,
                       sizeof(machine->keyboard_rows)) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, wz_kempston_read(&machine->kempston,
                                                   WZ_KEMPSTON_PORT)) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, machine->ula_output) != WZ_RESULT_OK ||
        wz_state_write_u64(writer, machine->ula_output_tick) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, machine->maskable_interrupt_line_low) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, (wz_byte_t)machine->tape_loading_mode) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, (wz_byte_t)machine->networking_mode) != WZ_RESULT_OK ||
        wz_state_write_ay(writer, &machine->ay) != WZ_RESULT_OK ||
        wz_state_write(writer, machine->memory, sizeof(machine->memory)) != WZ_RESULT_OK) {
        return WZ_RESULT_SERIALIZATION_FAILURE;
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_state_hash_machine(const wz_machine_t* machine,
                                  wz_qword_t* hash)
{
    wz_byte_t bytes[WZ_STATE_MACHINE_LENGTH];
    wz_state_writer_t writer;

    if (hash == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    wz_state_writer_init(&writer, bytes, sizeof(bytes));
    if (wz_state_serialize_machine(machine, &writer) != WZ_RESULT_OK) {
        return WZ_RESULT_SERIALIZATION_FAILURE;
    }

    *hash = UINT64_C(14695981039346656037);
    for (size_t index = 0u; index < writer.length; ++index) {
        *hash ^= (wz_qword_t)bytes[index];
        *hash *= UINT64_C(1099511628211);
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_state_validate_historical_representability(
    const wz_machine_t* machine,
    wz_historical_state_format_t format)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (format > WZ_HISTORICAL_FORMAT_Z80) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (machine->tape_loading_mode != WZ_TAPE_LOADING_NORMAL ||
        machine->networking_mode != WZ_NETWORKING_NONE) {
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_state_deserialize_machine(wz_machine_t* machine,
                                         const wz_byte_t* data,
                                         size_t length)
{
    const wz_machine_profile_t* profile;
    wz_z80_state_t cpu;
    wz_qword_t tick = 0u;
    wz_qword_t ula_output_tick = 0u;
    wz_networking_mode_t networking_mode;

    if (machine == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length != WZ_STATE_MACHINE_LENGTH || data[0] != WZ_STATE_VERSION) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (data[71u] > (wz_byte_t)WZ_TAPE_LOADING_INSTANT_TRAP) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (data[72u] > (wz_byte_t)WZ_NETWORKING_EAR_MIC) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (!wz_state_ay_data_valid(data)) {
        return WZ_RESULT_INVALID_STATE;
    }
    networking_mode = (wz_networking_mode_t)data[72u];
    if (data[1] == (wz_byte_t)WZ_MACHINE_48K_PAL) {
        profile = wz_machine_profile_48k_pal();
    } else if (data[1] == (wz_byte_t)WZ_MACHINE_128K_PAL) {
        profile = wz_machine_profile_128k_pal();
    } else {
        profile = 0;
    }
    if (profile == 0) {
        return WZ_RESULT_INVALID_PROFILE;
    }
    for (size_t index = 0u; index < 8u; ++index) {
        tick |= (wz_qword_t)data[44u + index] << (index * 8u);
    }
    cpu.main.a = data[11u];
    cpu.main.f = data[12u];
    cpu.main.b = data[13u];
    cpu.main.c = data[14u];
    cpu.main.d = data[15u];
    cpu.main.e = data[16u];
    cpu.main.h = data[17u];
    cpu.main.l = data[18u];
    cpu.alternate.a = data[19u];
    cpu.alternate.f = data[20u];
    cpu.alternate.b = data[21u];
    cpu.alternate.c = data[22u];
    cpu.alternate.d = data[23u];
    cpu.alternate.e = data[24u];
    cpu.alternate.h = data[25u];
    cpu.alternate.l = data[26u];
    cpu.ix = wz_read_le16(data + 27u);
    cpu.iy = wz_read_le16(data + 29u);
    cpu.stack_pointer = wz_read_le16(data + 31u);
    cpu.program_counter = wz_read_le16(data + 33u);
    cpu.memptr = wz_read_le16(data + 35u);
    cpu.i = data[37u];
    cpu.r = data[38u];
    cpu.iff1 = data[39u];
    cpu.iff2 = data[40u];
    cpu.interrupt_enable_delay = data[41u];
    cpu.interrupt_mode = data[42u];
    cpu.halted = data[43u];
    if (wz_z80_state_validate(&cpu) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    machine->profile = profile;
    machine->cpu = cpu;
    machine->has_48k_rom = data[2u];
    machine->hardware_io_decode_enabled = 1u;
    machine->rom_identity = 0u;
    for (size_t index = 0u; index < sizeof(machine->rom_identity); ++index) {
        machine->rom_identity |= (wz_qword_t)data[3u + index] << (index * 8u);
    }
    if (machine->has_48k_rom > 1u ||
        (machine->has_48k_rom == 0u && machine->rom_identity != 0u) ||
        (machine->has_48k_rom != 0u && machine->rom_identity == 0u)) {
        return WZ_RESULT_INVALID_STATE;
    }
    machine->master_tick = tick;
    for (size_t index = 0u; index < sizeof(machine->keyboard_rows); ++index) {
        if ((data[52u + index] & 0xe0u) != 0u) {
            return WZ_RESULT_INVALID_STATE;
        }
        machine->keyboard_rows[index] = data[52u + index];
    }
    if ((data[60u] & (wz_byte_t)~0x1fu) != 0u) {
        return WZ_RESULT_INVALID_STATE;
    }
    wz_kempston_init(&machine->kempston);
    for (size_t index = 0u; index < WZ_KEMPSTON_CONTROL_COUNT; ++index) {
        if (!wz_kempston_set(&machine->kempston,
                             (wz_kempston_control_t)index,
                             (data[60u] & (wz_byte_t[]){0x10u, 0x08u, 0x02u, 0x01u, 0x04u}[index]) != 0u)) {
            return WZ_RESULT_INVALID_STATE;
        }
    }
    machine->ula_output = data[61u];
    if ((machine->ula_output & 0xe0u) != 0u) {
        return WZ_RESULT_INVALID_STATE;
    }
    for (size_t index = 0u; index < sizeof(ula_output_tick); ++index) {
        ula_output_tick |= (wz_qword_t)data[62u + index] << (index * 8u);
    }
    machine->ula_output_tick = ula_output_tick;
    if (data[70u] > 1u) {
        return WZ_RESULT_INVALID_STATE;
    }
    machine->maskable_interrupt_line_low = data[70u];
    machine->tape_loading_mode = (wz_tape_loading_mode_t)data[71u];
    machine->networking_mode = networking_mode;
    {
        size_t offset = 73u;
        wz_dword_t noise_lfsr = 0u;

        machine->ay.selected_register = data[offset++];
        if (machine->ay.selected_register >= WZ_AY_REGISTER_COUNT) {
            return WZ_RESULT_INVALID_STATE;
        }
        for (size_t index = 0u; index < sizeof(machine->ay.registers); ++index) {
            machine->ay.registers[index] = data[offset++];
        }
        for (wz_byte_t channel = 0u; channel < WZ_AY_CHANNEL_COUNT; ++channel) {
            machine->ay.tone_counters[channel] = wz_read_le16(data + offset);
            offset += 2u;
        }
        for (size_t index = 0u; index < sizeof(machine->ay.tone_levels); ++index) {
            machine->ay.tone_levels[index] = data[offset++];
            if (machine->ay.tone_levels[index] > 1u) {
                return WZ_RESULT_INVALID_STATE;
            }
        }
        machine->ay.noise_counter = data[offset++];
        for (size_t index = 0u; index < sizeof(noise_lfsr); ++index) {
            noise_lfsr |= (wz_dword_t)data[offset++] << (index * 8u);
        }
        machine->ay.noise_lfsr = noise_lfsr;
        machine->ay.noise_level = data[offset++];
        machine->ay.envelope_counter = wz_read_le16(data + offset);
        offset += 2u;
        machine->ay.envelope_period = wz_read_le16(data + offset);
        offset += 2u;
        machine->ay.envelope_level = data[offset++];
        machine->ay.envelope_attack = data[offset++];
        machine->ay.envelope_holding = data[offset++];
        machine->ay.tone_master_tick_phase = data[offset++];
        machine->ay.event_count = 0u;
        if (machine->ay.noise_lfsr > 0x1ffffu || machine->ay.noise_level > 1u ||
            machine->ay.envelope_level >= WZ_AY_ENVELOPE_LEVEL_COUNT ||
            machine->ay.envelope_attack > 1u || machine->ay.envelope_holding > 1u ||
            machine->ay.tone_master_tick_phase >= WZ_AY_MASTER_TICKS_PER_CLOCK) {
            return WZ_RESULT_INVALID_STATE;
        }
    }
    for (size_t index = 0u; index < sizeof(machine->memory); ++index) {
        machine->memory[index] = data[113u + index];
    }
    return WZ_RESULT_OK;
}

void wz_snapshot_state_init(wz_snapshot_state_t* snapshot)
{
    if (snapshot != 0) {
        snapshot->length = 0u;
    }
}

wz_result_t wz_snapshot_state_capture(wz_snapshot_state_t* snapshot,
                                      const wz_machine_t* machine)
{
    wz_state_writer_t writer;

    if (snapshot == 0 || machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    wz_state_writer_init(&writer, snapshot->data, sizeof(snapshot->data));
    if (wz_state_serialize_machine(machine, &writer) != WZ_RESULT_OK) {
        return WZ_RESULT_SERIALIZATION_FAILURE;
    }
    snapshot->length = writer.length;
    return WZ_RESULT_OK;
}

wz_result_t wz_snapshot_state_load(wz_snapshot_state_t* snapshot,
                                   const wz_byte_t* data,
                                   size_t length)
{
    wz_machine_t* candidate;

    if (snapshot == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length > WZ_STATE_SNAPSHOT_CAPACITY) {
        return WZ_RESULT_INVALID_STATE;
    }
    candidate = (wz_machine_t*)calloc(1u, sizeof(*candidate));
    if (candidate == 0) {
        return WZ_RESULT_OUT_OF_MEMORY;
    }
    if (wz_state_deserialize_machine(candidate, data, length) != WZ_RESULT_OK) {
        wz_machine_destroy(candidate);
        free(candidate);
        return WZ_RESULT_INVALID_STATE;
    }
    wz_machine_destroy(candidate);
    free(candidate);
    for (size_t index = 0u; index < length; ++index) {
        snapshot->data[index] = data[index];
    }
    snapshot->length = length;
    return WZ_RESULT_OK;
}

const wz_byte_t* wz_snapshot_state_data(const wz_snapshot_state_t* snapshot)
{
    if (snapshot == 0 || snapshot->length == 0u) {
        return 0;
    }
    return snapshot->data;
}

size_t wz_snapshot_state_length(const wz_snapshot_state_t* snapshot)
{
    return snapshot == 0 ? 0u : snapshot->length;
}

wz_result_t wz_snapshot_state_load_sna_48k(wz_snapshot_state_t* snapshot,
                                           const wz_byte_t* data,
                                           size_t length)
{
    static wz_machine_t candidate;
    wz_word_t stack_pointer;

    if (snapshot == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length != WZ_SNA_48K_LENGTH || data[19u] > 4u ||
        (data[19u] != 0u && data[19u] != 4u) || data[25u] > 2u ||
        data[26u] > 7u) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (wz_machine_init(&candidate, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_PROFILE;
    }

    candidate.cpu.alternate.h = data[1u];
    candidate.cpu.alternate.l = data[2u];
    candidate.cpu.alternate.d = data[3u];
    candidate.cpu.alternate.e = data[4u];
    candidate.cpu.alternate.b = data[5u];
    candidate.cpu.alternate.c = data[6u];
    candidate.cpu.alternate.a = data[7u];
    candidate.cpu.alternate.f = data[8u];
    candidate.cpu.main.h = data[9u];
    candidate.cpu.main.l = data[10u];
    candidate.cpu.main.d = data[11u];
    candidate.cpu.main.e = data[12u];
    candidate.cpu.main.b = data[13u];
    candidate.cpu.main.c = data[14u];
    candidate.cpu.iy = wz_read_le16(data + 15u);
    candidate.cpu.ix = wz_read_le16(data + 17u);
    candidate.cpu.i = data[0u];
    candidate.cpu.iff1 = data[19u] == 4u ? 1u : 0u;
    candidate.cpu.iff2 = candidate.cpu.iff1;
    candidate.cpu.r = data[20u];
    candidate.cpu.main.f = data[21u];
    candidate.cpu.main.a = data[22u];
    stack_pointer = wz_read_le16(data + 23u);
    candidate.cpu.stack_pointer = (wz_word_t)(stack_pointer + 2u);
    candidate.cpu.interrupt_mode = data[25u];
    candidate.border_color = data[26u];
    candidate.ula_output = data[26u];
    for (size_t index = 0u; index < 49152u; ++index) {
        candidate.memory[0x4000u + index] = data[27u + index];
    }
    candidate.cpu.program_counter =
        (wz_word_t)(candidate.memory[stack_pointer] |
                    ((wz_word_t)candidate.memory[(wz_word_t)(stack_pointer + 1u)] << 8u));
    if (wz_z80_state_validate(&candidate.cpu) != WZ_RESULT_OK) {
        wz_machine_destroy(&candidate);
        return WZ_RESULT_INVALID_STATE;
    }
    if (wz_snapshot_state_capture(snapshot, &candidate) != WZ_RESULT_OK) {
        wz_machine_destroy(&candidate);
        return WZ_RESULT_SERIALIZATION_FAILURE;
    }
    wz_machine_destroy(&candidate);
    return WZ_RESULT_OK;
}

static void wz_sna_write_u16(wz_byte_t* data, size_t offset, wz_word_t value)
{
    wz_write_le16(data + offset, value);
}

wz_result_t wz_state_save_sna_48k(const wz_machine_t* machine,
                                  wz_byte_t* data,
                                  size_t capacity)
{
    wz_word_t sna_stack_pointer;

    if (machine == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (capacity < WZ_SNA_48K_LENGTH) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    if (machine->profile == 0 || machine->profile->kind != WZ_MACHINE_48K_PAL ||
        wz_state_validate_historical_representability(
            machine, WZ_HISTORICAL_FORMAT_SNA) != WZ_RESULT_OK ||
        wz_z80_state_validate(&machine->cpu) != WZ_RESULT_OK) {
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
    sna_stack_pointer = (wz_word_t)(machine->cpu.stack_pointer - 2u);
    if (sna_stack_pointer < 0x4000u || sna_stack_pointer == 0xffffu) {
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }

    data[0u] = machine->cpu.i;
    wz_sna_write_u16(data, 1u, (wz_word_t)((wz_word_t)machine->cpu.alternate.h << 8u |
                                          machine->cpu.alternate.l));
    wz_sna_write_u16(data, 3u, (wz_word_t)((wz_word_t)machine->cpu.alternate.d << 8u |
                                          machine->cpu.alternate.e));
    wz_sna_write_u16(data, 5u, (wz_word_t)((wz_word_t)machine->cpu.alternate.b << 8u |
                                          machine->cpu.alternate.c));
    wz_sna_write_u16(data, 7u, (wz_word_t)((wz_word_t)machine->cpu.alternate.a << 8u |
                                          machine->cpu.alternate.f));
    wz_sna_write_u16(data, 9u, (wz_word_t)((wz_word_t)machine->cpu.main.h << 8u |
                                          machine->cpu.main.l));
    wz_sna_write_u16(data, 11u, (wz_word_t)((wz_word_t)machine->cpu.main.d << 8u |
                                           machine->cpu.main.e));
    wz_sna_write_u16(data, 13u, (wz_word_t)((wz_word_t)machine->cpu.main.b << 8u |
                                           machine->cpu.main.c));
    wz_sna_write_u16(data, 15u, machine->cpu.iy);
    wz_sna_write_u16(data, 17u, machine->cpu.ix);
    data[19u] = machine->cpu.iff2 == 0u ? 0u : 4u;
    data[20u] = machine->cpu.r;
    data[21u] = machine->cpu.main.f;
    data[22u] = machine->cpu.main.a;
    wz_sna_write_u16(data, 23u, sna_stack_pointer);
    data[25u] = machine->cpu.interrupt_mode;
    data[26u] = machine->border_color;
    for (size_t index = 0u; index < 49152u; ++index) {
        data[27u + index] = machine->memory[0x4000u + index];
    }
    data[27u + (size_t)(sna_stack_pointer - 0x4000u)] =
        (wz_byte_t)(machine->cpu.program_counter & 0xffu);
    data[27u + (size_t)(sna_stack_pointer - 0x4000u) + 1u] =
        (wz_byte_t)(machine->cpu.program_counter >> 8u);
    return WZ_RESULT_OK;
}

wz_result_t wz_sna_128k_image_init(wz_sna_128k_image_t* image)
{
    if (image == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (size_t index = 0u; index < sizeof(*image); ++index) {
        ((wz_byte_t*)image)[index] = 0u;
    }
    return WZ_RESULT_OK;
}

static bool wz_sna_128k_page_is_present(const wz_sna_128k_image_t* image,
                                         wz_byte_t page)
{
    return page < WZ_SNA_128K_PAGE_COUNT && image->page_present[page] != 0u;
}

wz_result_t wz_sna_128k_image_load(wz_sna_128k_image_t* image,
                                    const wz_byte_t* data,
                                    size_t length)
{
    wz_byte_t current_page;
    size_t offset;

    if (image == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length != WZ_SNA_128K_LENGTH) {
        return WZ_RESULT_PARSE_ERROR;
    }

    current_page = (wz_byte_t)(data[49181u] & 0x07u);
    if (current_page == 2u || current_page == 5u || data[49181u] > 0x3fu ||
        data[49182u] > 1u) {
        return WZ_RESULT_PARSE_ERROR;
    }

    wz_sna_128k_image_t parsed;
    wz_sna_128k_image_init(&parsed);
    for (size_t index = 0u; index < sizeof(parsed.header); ++index) {
        parsed.header[index] = data[index];
    }
    parsed.program_counter = wz_read_le16(data + 49179u);
    parsed.paging_7ffd = data[49181u];
    parsed.trdos_active = data[49182u];

    offset = 27u;
    for (size_t index = 0u; index < WZ_SNA_128K_PAGE_SIZE; ++index) {
        parsed.pages[5u][index] = data[offset + index];
        parsed.pages[2u][index] = data[offset + WZ_SNA_128K_PAGE_SIZE + index];
        parsed.pages[current_page][index] =
            data[offset + (2u * WZ_SNA_128K_PAGE_SIZE) + index];
    }
    parsed.page_present[5u] = 1u;
    parsed.page_present[2u] = 1u;
    parsed.page_present[current_page] = 1u;
    offset += 3u * WZ_SNA_128K_PAGE_SIZE + 4u;

    for (wz_byte_t page = 0u; page < WZ_SNA_128K_PAGE_COUNT; ++page) {
        if (wz_sna_128k_page_is_present(&parsed, page)) {
            continue;
        }
        for (size_t index = 0u; index < WZ_SNA_128K_PAGE_SIZE; ++index) {
            parsed.pages[page][index] = data[offset + index];
        }
        parsed.page_present[page] = 1u;
        offset += WZ_SNA_128K_PAGE_SIZE;
    }
    if (offset != WZ_SNA_128K_LENGTH) {
        return WZ_RESULT_PARSE_ERROR;
    }
    *image = parsed;
    return WZ_RESULT_OK;
}

wz_result_t wz_sna_128k_image_save(const wz_sna_128k_image_t* image,
                                    wz_byte_t* data,
                                    size_t capacity)
{
    wz_byte_t current_page;
    size_t offset;

    if (image == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (capacity < WZ_SNA_128K_LENGTH) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    current_page = (wz_byte_t)(image->paging_7ffd & 0x07u);
    if (current_page == 2u || current_page == 5u ||
        image->paging_7ffd > 0x3fu || image->trdos_active > 1u ||
        !wz_sna_128k_page_is_present(image, 2u) ||
        !wz_sna_128k_page_is_present(image, 5u) ||
        !wz_sna_128k_page_is_present(image, current_page)) {
        return WZ_RESULT_INVALID_STATE;
    }
    for (wz_byte_t page = 0u; page < WZ_SNA_128K_PAGE_COUNT; ++page) {
        if (!wz_sna_128k_page_is_present(image, page)) {
            return WZ_RESULT_INVALID_STATE;
        }
    }

    for (size_t index = 0u; index < sizeof(image->header); ++index) {
        data[index] = image->header[index];
    }
    offset = 27u;
    for (size_t index = 0u; index < WZ_SNA_128K_PAGE_SIZE; ++index) {
        data[offset + index] = image->pages[5u][index];
        data[offset + WZ_SNA_128K_PAGE_SIZE + index] = image->pages[2u][index];
        data[offset + (2u * WZ_SNA_128K_PAGE_SIZE) + index] =
            image->pages[current_page][index];
    }
    offset += 3u * WZ_SNA_128K_PAGE_SIZE;
    wz_write_le16(data + offset, image->program_counter);
    data[offset + 2u] = image->paging_7ffd;
    data[offset + 3u] = image->trdos_active;
    offset += 4u;
    for (wz_byte_t page = 0u; page < WZ_SNA_128K_PAGE_COUNT; ++page) {
        if (page == 2u || page == 5u || page == current_page) {
            continue;
        }
        for (size_t index = 0u; index < WZ_SNA_128K_PAGE_SIZE; ++index) {
            data[offset + index] = image->pages[page][index];
        }
        offset += WZ_SNA_128K_PAGE_SIZE;
    }
    return offset == WZ_SNA_128K_LENGTH ? WZ_RESULT_OK : WZ_RESULT_SERIALIZATION_FAILURE;
}

static wz_result_t wz_sna_128k_decode_cpu(const wz_byte_t* data,
                                          wz_z80_state_t* cpu,
                                          wz_byte_t* border_color)
{
    if (data == 0 || cpu == 0 || border_color == 0 ||
        (data[19u] != 0u && data[19u] != 4u) || data[25u] > 2u ||
        data[26u] > 7u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    cpu->alternate.h = (wz_byte_t)(wz_read_le16(data + 1u) >> 8u);
    cpu->alternate.l = (wz_byte_t)wz_read_le16(data + 1u);
    cpu->alternate.d = (wz_byte_t)(wz_read_le16(data + 3u) >> 8u);
    cpu->alternate.e = (wz_byte_t)wz_read_le16(data + 3u);
    cpu->alternate.b = (wz_byte_t)(wz_read_le16(data + 5u) >> 8u);
    cpu->alternate.c = (wz_byte_t)wz_read_le16(data + 5u);
    cpu->alternate.a = (wz_byte_t)(wz_read_le16(data + 7u) >> 8u);
    cpu->alternate.f = (wz_byte_t)wz_read_le16(data + 7u);
    cpu->main.h = (wz_byte_t)(wz_read_le16(data + 9u) >> 8u);
    cpu->main.l = (wz_byte_t)wz_read_le16(data + 9u);
    cpu->main.d = (wz_byte_t)(wz_read_le16(data + 11u) >> 8u);
    cpu->main.e = (wz_byte_t)wz_read_le16(data + 11u);
    cpu->main.b = (wz_byte_t)(wz_read_le16(data + 13u) >> 8u);
    cpu->main.c = (wz_byte_t)wz_read_le16(data + 13u);
    cpu->iy = wz_read_le16(data + 15u);
    cpu->ix = wz_read_le16(data + 17u);
    cpu->i = data[0u];
    cpu->iff1 = data[19u] == 4u ? 1u : 0u;
    cpu->iff2 = cpu->iff1;
    cpu->r = data[20u];
    cpu->main.f = data[21u];
    cpu->main.a = data[22u];
    cpu->stack_pointer = wz_read_le16(data + 23u);
    cpu->program_counter = wz_read_le16(data + 49179u);
    cpu->interrupt_mode = data[25u];
    *border_color = data[26u];
    return wz_z80_state_validate(cpu);
}

static void wz_sna_128k_apply_candidate(wz_machine_t* machine,
                                         const wz_machine_t* candidate)
{
    wz_byte_t* ram_128k = machine->ram_128k;

    for (size_t index = 0u; index < sizeof(machine->memory); ++index) {
        machine->memory[index] = candidate->memory[index];
    }
    for (size_t index = 0u;
         index < WZ_128K_RAM_BANK_COUNT * WZ_128K_RAM_BANK_SIZE; ++index) {
        ram_128k[index] = candidate->ram_128k[index];
    }
    machine->cpu = candidate->cpu;
    machine->has_48k_rom = candidate->has_48k_rom;
    machine->paging_7ffd = candidate->paging_7ffd;
    machine->paging_7ffd_locked = candidate->paging_7ffd_locked;
    machine->hardware_io_decode_enabled = candidate->hardware_io_decode_enabled;
    for (size_t index = 0u; index < sizeof(machine->keyboard_rows); ++index) {
        machine->keyboard_rows[index] = candidate->keyboard_rows[index];
    }
    machine->kempston = candidate->kempston;
    machine->beeper = candidate->beeper;
    machine->ay = candidate->ay;
    machine->tape_loading_mode = candidate->tape_loading_mode;
    machine->networking_mode = candidate->networking_mode;
    machine->ula_output = candidate->ula_output;
    machine->rom_identity = candidate->rom_identity;
    machine->master_tick = candidate->master_tick;
    machine->ula_output_tick = candidate->ula_output_tick;
    machine->border_color = candidate->border_color;
    for (size_t index = 0u; index < WZ_BORDER_EVENT_CAPACITY; ++index) {
        machine->border_events[index] = candidate->border_events[index];
    }
    machine->border_event_count = candidate->border_event_count;
    machine->maskable_interrupt_line_low = candidate->maskable_interrupt_line_low;
    machine->im0_injected_opcode = candidate->im0_injected_opcode;
    machine->im0_injected_opcode_pending = candidate->im0_injected_opcode_pending;
    machine->tape.segments = 0;
    machine->tape.segment_count = 0u;
    machine->tape_state = candidate->tape_state;
    machine->tape_state.tape = &machine->tape;
    machine->tape_mounted = 0u;
}

wz_result_t wz_state_load_sna_128k(wz_machine_t* machine,
                                   const wz_byte_t* data,
                                   size_t length)
{
    static wz_machine_t candidate;
    static wz_sna_128k_image_t image;
    wz_byte_t border_color = 0u;

    if (machine == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (machine->profile == 0 ||
        machine->profile->kind != WZ_MACHINE_128K_PAL ||
        machine->ram_128k == 0) {
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
    if (wz_sna_128k_image_load(&image, data, length) != WZ_RESULT_OK ||
        image.trdos_active != 0u ||
        wz_machine_init(&candidate, wz_machine_profile_128k_pal()) != WZ_RESULT_OK ||
        wz_sna_128k_decode_cpu(data, &candidate.cpu, &border_color) !=
            WZ_RESULT_OK) {
        wz_machine_destroy(&candidate);
        return WZ_RESULT_PARSE_ERROR;
    }
    for (wz_byte_t page = 0u; page < WZ_128K_RAM_BANK_COUNT; ++page) {
        for (size_t index = 0u; index < WZ_128K_RAM_BANK_SIZE; ++index) {
            candidate.ram_128k[page * WZ_128K_RAM_BANK_SIZE + index] =
                image.pages[page][index];
        }
    }
    candidate.paging_7ffd = image.paging_7ffd;
    candidate.paging_7ffd_locked = (wz_byte_t)((image.paging_7ffd >> 5u) & 1u);
    candidate.border_color = border_color;
    candidate.ula_output = border_color;
    wz_sna_128k_apply_candidate(machine, &candidate);
    wz_machine_destroy(&candidate);
    return WZ_RESULT_OK;
}

wz_result_t wz_state_save_sna_128k(const wz_machine_t* machine,
                                   wz_byte_t* data,
                                   size_t capacity)
{
    wz_byte_t current_page;
    size_t offset;

    if (machine == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (capacity < WZ_SNA_128K_LENGTH) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    if (machine->profile == 0 || machine->profile->kind != WZ_MACHINE_128K_PAL ||
        machine->ram_128k == 0 || machine->tape_loading_mode != WZ_TAPE_LOADING_NORMAL ||
        machine->networking_mode != WZ_NETWORKING_NONE ||
        machine->paging_7ffd > 0x3fu ||
        wz_z80_state_validate(&machine->cpu) != WZ_RESULT_OK) {
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
    current_page = (wz_byte_t)(machine->paging_7ffd & 0x07u);
    if (current_page == 2u || current_page == 5u || machine->border_color > 7u) {
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
    data[0u] = machine->cpu.i;
    wz_sna_write_u16(data, 1u, (wz_word_t)((wz_word_t)machine->cpu.alternate.h << 8u |
                                          machine->cpu.alternate.l));
    wz_sna_write_u16(data, 3u, (wz_word_t)((wz_word_t)machine->cpu.alternate.d << 8u |
                                          machine->cpu.alternate.e));
    wz_sna_write_u16(data, 5u, (wz_word_t)((wz_word_t)machine->cpu.alternate.b << 8u |
                                          machine->cpu.alternate.c));
    wz_sna_write_u16(data, 7u, (wz_word_t)((wz_word_t)machine->cpu.alternate.a << 8u |
                                          machine->cpu.alternate.f));
    wz_sna_write_u16(data, 9u, (wz_word_t)((wz_word_t)machine->cpu.main.h << 8u |
                                          machine->cpu.main.l));
    wz_sna_write_u16(data, 11u, (wz_word_t)((wz_word_t)machine->cpu.main.d << 8u |
                                           machine->cpu.main.e));
    wz_sna_write_u16(data, 13u, (wz_word_t)((wz_word_t)machine->cpu.main.b << 8u |
                                           machine->cpu.main.c));
    wz_sna_write_u16(data, 15u, machine->cpu.iy);
    wz_sna_write_u16(data, 17u, machine->cpu.ix);
    data[19u] = machine->cpu.iff2 == 0u ? 0u : 4u;
    data[20u] = machine->cpu.r;
    data[21u] = machine->cpu.main.f;
    data[22u] = machine->cpu.main.a;
    wz_sna_write_u16(data, 23u, machine->cpu.stack_pointer);
    data[25u] = machine->cpu.interrupt_mode;
    data[26u] = machine->border_color;
    for (size_t index = 0u; index < 3u * WZ_SNA_128K_PAGE_SIZE; ++index) {
        wz_byte_t page = index < WZ_SNA_128K_PAGE_SIZE ? 5u :
            index < 2u * WZ_SNA_128K_PAGE_SIZE ? 2u : current_page;
        size_t page_offset = index % WZ_SNA_128K_PAGE_SIZE;
        data[27u + index] = machine->ram_128k[
            page * WZ_128K_RAM_BANK_SIZE + page_offset];
    }
    offset = 27u + 3u * WZ_SNA_128K_PAGE_SIZE;
    wz_write_le16(data + offset, machine->cpu.program_counter);
    data[offset + 2u] = machine->paging_7ffd;
    data[offset + 3u] = 0u;
    offset += 4u;
    for (wz_byte_t page = 0u; page < WZ_128K_RAM_BANK_COUNT; ++page) {
        if (page == 2u || page == 5u || page == current_page) {
            continue;
        }
        for (size_t index = 0u; index < WZ_SNA_128K_PAGE_SIZE; ++index) {
            data[offset + index] = machine->ram_128k[
                page * WZ_128K_RAM_BANK_SIZE + index];
        }
        offset += WZ_SNA_128K_PAGE_SIZE;
    }
    return offset == WZ_SNA_128K_LENGTH ? WZ_RESULT_OK :
           WZ_RESULT_SERIALIZATION_FAILURE;
}

wz_result_t wz_snapshot_state_load_z80_v1(wz_snapshot_state_t* snapshot,
                                           const wz_byte_t* data,
                                           size_t length)
{
    static wz_machine_t candidate;
    size_t input_offset;
    size_t output_offset;
    bool compressed;
    bool marker_seen = false;

    if (snapshot == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length < WZ_Z80_V1_HEADER_LENGTH) {
        return WZ_RESULT_PARSE_ERROR;
    }
    if (data[6u] == 0u && data[7u] == 0u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    if (data[29u] > 2u || data[27u] > 1u || data[28u] > 1u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    compressed = (data[12u] & 0x20u) != 0u;
    if ((!compressed && length != WZ_Z80_V1_HEADER_LENGTH + WZ_Z80_V1_MEMORY_LENGTH) ||
        (compressed && length <= WZ_Z80_V1_HEADER_LENGTH + 4u)) {
        return WZ_RESULT_PARSE_ERROR;
    }
    if (wz_machine_init(&candidate, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_PROFILE;
    }

    candidate.cpu.main.a = data[0u];
    candidate.cpu.main.f = data[1u];
    candidate.cpu.main.c = data[2u];
    candidate.cpu.main.b = data[3u];
    candidate.cpu.main.l = data[4u];
    candidate.cpu.main.h = data[5u];
    candidate.cpu.program_counter = wz_read_le16(data + 6u);
    candidate.cpu.stack_pointer = wz_read_le16(data + 8u);
    candidate.cpu.i = data[10u];
    candidate.cpu.r = (wz_byte_t)((data[11u] & 0x7fu) |
                                  ((data[12u] & 0x01u) << 7u));
    candidate.border_color = (wz_byte_t)((data[12u] >> 1u) & 0x07u);
    candidate.ula_output = candidate.border_color;
    candidate.cpu.main.e = data[13u];
    candidate.cpu.main.d = data[14u];
    candidate.cpu.alternate.c = data[15u];
    candidate.cpu.alternate.b = data[16u];
    candidate.cpu.alternate.e = data[17u];
    candidate.cpu.alternate.d = data[18u];
    candidate.cpu.alternate.l = data[19u];
    candidate.cpu.alternate.h = data[20u];
    candidate.cpu.alternate.a = data[21u];
    candidate.cpu.alternate.f = data[22u];
    candidate.cpu.iy = wz_read_le16(data + 23u);
    candidate.cpu.ix = wz_read_le16(data + 25u);
    candidate.cpu.iff1 = data[27u];
    candidate.cpu.iff2 = data[28u];
    candidate.cpu.interrupt_mode = data[29u];

    input_offset = WZ_Z80_V1_HEADER_LENGTH;
    output_offset = 0u;
    while (input_offset < length) {
        size_t remaining = length - input_offset;
        if (remaining >= 4u && data[input_offset] == 0u &&
            data[input_offset + 1u] == 0xedu &&
            data[input_offset + 2u] == 0xedu &&
            data[input_offset + 3u] == 0u) {
            if (!compressed || output_offset != WZ_Z80_V1_MEMORY_LENGTH ||
                input_offset + 4u != length) {
                wz_machine_destroy(&candidate);
                return WZ_RESULT_PARSE_ERROR;
            }
            marker_seen = true;
            input_offset += 4u;
            break;
        }
        if (output_offset >= WZ_Z80_V1_MEMORY_LENGTH) {
            wz_machine_destroy(&candidate);
            return WZ_RESULT_PARSE_ERROR;
        }
        if (compressed && remaining >= 4u && data[input_offset] == 0xedu &&
            data[input_offset + 1u] == 0xedu) {
            wz_byte_t count = data[input_offset + 2u];
            wz_byte_t value = data[input_offset + 3u];
            if (count == 0u || output_offset + count > WZ_Z80_V1_MEMORY_LENGTH) {
                wz_machine_destroy(&candidate);
                return WZ_RESULT_PARSE_ERROR;
            }
            for (size_t repeat = 0u; repeat < count; ++repeat) {
                candidate.memory[0x4000u + output_offset++] = value;
            }
            input_offset += 4u;
        } else {
            candidate.memory[0x4000u + output_offset++] = data[input_offset++];
        }
    }
    if (output_offset != WZ_Z80_V1_MEMORY_LENGTH ||
        (compressed && !marker_seen) ||
        (!compressed && input_offset != length)) {
        wz_machine_destroy(&candidate);
        return WZ_RESULT_PARSE_ERROR;
    }
    if (wz_z80_state_validate(&candidate.cpu) != WZ_RESULT_OK ||
        wz_snapshot_state_capture(snapshot, &candidate) != WZ_RESULT_OK) {
        wz_machine_destroy(&candidate);
        return WZ_RESULT_INVALID_STATE;
    }
    wz_machine_destroy(&candidate);
    return WZ_RESULT_OK;
}

static wz_result_t wz_z80_v2_decode_page(wz_byte_t* destination,
                                         const wz_byte_t* data,
                                         size_t length)
{
    size_t input_offset = 0u;
    size_t output_offset = 0u;

    if (destination == 0 || data == 0 || length == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    while (input_offset < length) {
        if (input_offset + 4u <= length && data[input_offset] == 0xedu &&
            data[input_offset + 1u] == 0xedu) {
            wz_byte_t count = data[input_offset + 2u];
            wz_byte_t value = data[input_offset + 3u];
            if (count == 0u || output_offset + count > WZ_Z80_V2_PAGE_SIZE) {
                return WZ_RESULT_PARSE_ERROR;
            }
            for (size_t repeat = 0u; repeat < count; ++repeat) {
                destination[output_offset++] = value;
            }
            input_offset += 4u;
        } else {
            if (output_offset >= WZ_Z80_V2_PAGE_SIZE) {
                return WZ_RESULT_PARSE_ERROR;
            }
            destination[output_offset++] = data[input_offset++];
        }
    }
    return output_offset == WZ_Z80_V2_PAGE_SIZE ? WZ_RESULT_OK :
           WZ_RESULT_PARSE_ERROR;
}

static wz_result_t wz_z80_v2_map_header(wz_machine_t* machine,
                                        const wz_byte_t* data)
{
    if (machine == 0 || data == 0 || data[6u] != 0u || data[7u] != 0u ||
        wz_read_le16(data + 30u) != 23u || data[34u] != 0u ||
        data[35u] != 0u || data[36u] != 0u || data[54u] != 0u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    if (data[27u] > 1u || data[28u] > 1u || data[53u] > 15u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    for (size_t index = 37u; index < 53u; ++index) {
        if (data[index] != 0u) {
            return WZ_RESULT_PARSE_ERROR;
        }
    }

    machine->cpu.main.a = data[0u];
    machine->cpu.main.f = data[1u];
    machine->cpu.main.c = data[2u];
    machine->cpu.main.b = data[3u];
    machine->cpu.main.l = data[4u];
    machine->cpu.main.h = data[5u];
    machine->cpu.stack_pointer = wz_read_le16(data + 8u);
    machine->cpu.i = data[10u];
    machine->cpu.r = (wz_byte_t)((data[11u] & 0x7fu) |
                                  ((data[12u] & 0x01u) << 7u));
    machine->border_color = (wz_byte_t)((data[12u] >> 1u) & 0x07u);
    machine->ula_output = machine->border_color;
    machine->cpu.main.e = data[13u];
    machine->cpu.main.d = data[14u];
    machine->cpu.alternate.c = data[15u];
    machine->cpu.alternate.b = data[16u];
    machine->cpu.alternate.e = data[17u];
    machine->cpu.alternate.d = data[18u];
    machine->cpu.alternate.l = data[19u];
    machine->cpu.alternate.h = data[20u];
    machine->cpu.alternate.a = data[21u];
    machine->cpu.alternate.f = data[22u];
    machine->cpu.iy = wz_read_le16(data + 23u);
    machine->cpu.ix = wz_read_le16(data + 25u);
    machine->cpu.iff1 = data[27u];
    machine->cpu.iff2 = data[28u];
    machine->cpu.interrupt_mode = data[29u];
    machine->cpu.program_counter = wz_read_le16(data + 32u);
    return wz_z80_state_validate(&machine->cpu);
}

wz_result_t wz_snapshot_state_load_z80_v2(wz_snapshot_state_t* snapshot,
                                           const wz_byte_t* data,
                                           size_t length)
{
    static wz_machine_t candidate;
    wz_byte_t seen_pages[WZ_Z80_V2_PAGE_COUNT] = {0u};
    size_t offset = WZ_Z80_V2_HEADER_LENGTH;

    if (snapshot == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length < WZ_Z80_V2_HEADER_LENGTH) {
        return WZ_RESULT_PARSE_ERROR;
    }
    if (wz_machine_init(&candidate, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_z80_v2_map_header(&candidate, data) != WZ_RESULT_OK) {
        wz_machine_destroy(&candidate);
        return WZ_RESULT_PARSE_ERROR;
    }
    while (offset < length) {
        wz_word_t block_length;
        wz_byte_t page_number;
        size_t page_index;

        if (length - offset < 3u) {
            wz_machine_destroy(&candidate);
            return WZ_RESULT_PARSE_ERROR;
        }
        block_length = wz_read_le16(data + offset);
        page_number = data[offset + 2u];
        if (page_number == 8u) {
            page_index = 0u;
        } else if (page_number == 4u) {
            page_index = 1u;
        } else if (page_number == 5u) {
            page_index = 2u;
        } else {
            wz_machine_destroy(&candidate);
            return WZ_RESULT_PARSE_ERROR;
        }
        if (seen_pages[page_index] != 0u ||
            (block_length != WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH &&
             block_length == 0u) ||
            (block_length == WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH &&
             length - offset - 3u < WZ_Z80_V2_PAGE_SIZE) ||
            block_length != WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH &&
             (size_t)block_length > length - offset - 3u) {
            wz_machine_destroy(&candidate);
            return WZ_RESULT_PARSE_ERROR;
        }
        if (block_length == WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH) {
            for (size_t index = 0u; index < WZ_Z80_V2_PAGE_SIZE; ++index) {
                candidate.memory[0x4000u + (page_index * WZ_Z80_V2_PAGE_SIZE) + index] =
                    data[offset + 3u + index];
            }
        } else if (wz_z80_v2_decode_page(
                       candidate.memory + 0x4000u + (page_index * WZ_Z80_V2_PAGE_SIZE),
                       data + offset + 3u, (size_t)block_length) != WZ_RESULT_OK) {
            wz_machine_destroy(&candidate);
            return WZ_RESULT_PARSE_ERROR;
        }
        seen_pages[page_index] = 1u;
        offset += 3u + (block_length == WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH ?
                        WZ_Z80_V2_PAGE_SIZE : (size_t)block_length);
    }
    if (offset != length || seen_pages[0u] == 0u || seen_pages[1u] == 0u ||
        seen_pages[2u] == 0u || wz_snapshot_state_capture(snapshot, &candidate) !=
        WZ_RESULT_OK) {
        wz_machine_destroy(&candidate);
        return WZ_RESULT_PARSE_ERROR;
    }
    wz_machine_destroy(&candidate);
    return WZ_RESULT_OK;
}

static wz_result_t wz_z80_v3_map_header(wz_machine_t* machine,
                                        const wz_byte_t* data,
                                        size_t header_length)
{
    if (machine == 0 || data == 0 ||
        (header_length != WZ_Z80_V3_HEADER_LENGTH &&
         header_length != WZ_Z80_V3_HEADER_LENGTH_EXTENDED) ||
        data[6u] != 0u || data[7u] != 0u ||
        wz_read_le16(data + 30u) != (wz_word_t)(header_length - 32u) ||
        data[34u] != 0u || data[35u] != 0u || data[36u] != 0u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    for (size_t index = 37u; index < header_length; ++index) {
        if (data[index] != 0u) {
            return WZ_RESULT_PARSE_ERROR;
        }
    }

    machine->cpu.main.a = data[0u];
    machine->cpu.main.f = data[1u];
    machine->cpu.main.c = data[2u];
    machine->cpu.main.b = data[3u];
    machine->cpu.main.l = data[4u];
    machine->cpu.main.h = data[5u];
    machine->cpu.stack_pointer = wz_read_le16(data + 8u);
    machine->cpu.i = data[10u];
    machine->cpu.r = (wz_byte_t)((data[11u] & 0x7fu) |
                                  ((data[12u] & 0x01u) << 7u));
    machine->border_color = (wz_byte_t)((data[12u] >> 1u) & 0x07u);
    machine->ula_output = machine->border_color;
    machine->cpu.main.e = data[13u];
    machine->cpu.main.d = data[14u];
    machine->cpu.alternate.c = data[15u];
    machine->cpu.alternate.b = data[16u];
    machine->cpu.alternate.e = data[17u];
    machine->cpu.alternate.d = data[18u];
    machine->cpu.alternate.l = data[19u];
    machine->cpu.alternate.h = data[20u];
    machine->cpu.alternate.a = data[21u];
    machine->cpu.alternate.f = data[22u];
    machine->cpu.iy = wz_read_le16(data + 23u);
    machine->cpu.ix = wz_read_le16(data + 25u);
    machine->cpu.iff1 = data[27u];
    machine->cpu.iff2 = data[28u];
    machine->cpu.interrupt_mode = data[29u];
    machine->cpu.program_counter = wz_read_le16(data + 32u);
    return wz_z80_state_validate(&machine->cpu);
}

wz_result_t wz_snapshot_state_load_z80_v3(wz_snapshot_state_t* snapshot,
                                           const wz_byte_t* data,
                                           size_t length)
{
    static wz_machine_t candidate;
    wz_byte_t seen_pages[WZ_Z80_V2_PAGE_COUNT] = {0u};
    size_t header_length;
    size_t offset;

    if (snapshot == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length < WZ_Z80_V3_HEADER_LENGTH || data[6u] != 0u ||
        (wz_read_le16(data + 30u) != 54u &&
         wz_read_le16(data + 30u) != 55u)) {
        return WZ_RESULT_PARSE_ERROR;
    }
    header_length = 32u + (size_t)wz_read_le16(data + 30u);
    if (length < header_length ||
        wz_machine_init(&candidate, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_z80_v3_map_header(&candidate, data, header_length) != WZ_RESULT_OK) {
        wz_machine_destroy(&candidate);
        return WZ_RESULT_PARSE_ERROR;
    }
    offset = header_length;
    while (offset < length) {
        wz_word_t block_length;
        wz_byte_t page_number;
        size_t page_index;

        if (length - offset < 3u) {
            wz_machine_destroy(&candidate);
            return WZ_RESULT_PARSE_ERROR;
        }
        block_length = wz_read_le16(data + offset);
        page_number = data[offset + 2u];
        if (page_number == 8u) {
            page_index = 0u;
        } else if (page_number == 4u) {
            page_index = 1u;
        } else if (page_number == 5u) {
            page_index = 2u;
        } else {
            wz_machine_destroy(&candidate);
            return WZ_RESULT_PARSE_ERROR;
        }
        if (seen_pages[page_index] != 0u ||
            (block_length != WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH &&
             block_length == 0u) ||
            (block_length == WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH &&
             length - offset - 3u < WZ_Z80_V2_PAGE_SIZE) ||
            (block_length != WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH &&
             (size_t)block_length > length - offset - 3u)) {
            wz_machine_destroy(&candidate);
            return WZ_RESULT_PARSE_ERROR;
        }
        if (block_length == WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH) {
            for (size_t index = 0u; index < WZ_Z80_V2_PAGE_SIZE; ++index) {
                candidate.memory[0x4000u + (page_index * WZ_Z80_V2_PAGE_SIZE) + index] =
                    data[offset + 3u + index];
            }
        } else if (wz_z80_v2_decode_page(
                       candidate.memory + 0x4000u + (page_index * WZ_Z80_V2_PAGE_SIZE),
                       data + offset + 3u, (size_t)block_length) != WZ_RESULT_OK) {
            wz_machine_destroy(&candidate);
            return WZ_RESULT_PARSE_ERROR;
        }
        seen_pages[page_index] = 1u;
        offset += 3u + (block_length == WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH ?
                        WZ_Z80_V2_PAGE_SIZE : (size_t)block_length);
    }
    if (offset != length || seen_pages[0u] == 0u || seen_pages[1u] == 0u ||
        seen_pages[2u] == 0u || wz_snapshot_state_capture(snapshot, &candidate) !=
        WZ_RESULT_OK) {
        wz_machine_destroy(&candidate);
        return WZ_RESULT_PARSE_ERROR;
    }
    wz_machine_destroy(&candidate);
    return WZ_RESULT_OK;
}

wz_result_t wz_state_save_z80_v2_48k(const wz_machine_t* machine,
                                     wz_byte_t* data,
                                     size_t capacity)
{
    if (machine == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (capacity < WZ_Z80_V2_LENGTH) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    if (machine->profile == 0 || machine->profile->kind != WZ_MACHINE_48K_PAL ||
        wz_state_validate_historical_representability(
            machine, WZ_HISTORICAL_FORMAT_Z80) != WZ_RESULT_OK ||
        wz_z80_state_validate(&machine->cpu) != WZ_RESULT_OK) {
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
    for (size_t index = 0u; index < WZ_Z80_V2_LENGTH; ++index) {
        data[index] = 0u;
    }
    data[0u] = machine->cpu.main.a;
    data[1u] = machine->cpu.main.f;
    data[2u] = machine->cpu.main.c;
    data[3u] = machine->cpu.main.b;
    data[4u] = machine->cpu.main.l;
    data[5u] = machine->cpu.main.h;
    wz_write_le16(data + 8u, machine->cpu.stack_pointer);
    data[10u] = machine->cpu.i;
    data[11u] = (wz_byte_t)(machine->cpu.r & 0x7fu);
    data[12u] = (wz_byte_t)(((machine->cpu.r >> 7u) & 1u) |
                            (machine->border_color << 1u));
    data[13u] = machine->cpu.main.e;
    data[14u] = machine->cpu.main.d;
    data[15u] = machine->cpu.alternate.c;
    data[16u] = machine->cpu.alternate.b;
    data[17u] = machine->cpu.alternate.e;
    data[18u] = machine->cpu.alternate.d;
    data[19u] = machine->cpu.alternate.l;
    data[20u] = machine->cpu.alternate.h;
    data[21u] = machine->cpu.alternate.a;
    data[22u] = machine->cpu.alternate.f;
    wz_write_le16(data + 23u, machine->cpu.iy);
    wz_write_le16(data + 25u, machine->cpu.ix);
    data[27u] = machine->cpu.iff1;
    data[28u] = machine->cpu.iff2;
    data[29u] = machine->cpu.interrupt_mode;
    wz_write_le16(data + 30u, 23u);
    wz_write_le16(data + 32u, machine->cpu.program_counter);
    {
        const wz_byte_t pages[WZ_Z80_V2_PAGE_COUNT] = {8u, 4u, 5u};
        size_t offset = WZ_Z80_V2_HEADER_LENGTH;
        for (size_t page = 0u; page < WZ_Z80_V2_PAGE_COUNT; ++page) {
            wz_write_le16(data + offset, WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH);
            data[offset + 2u] = pages[page];
            for (size_t index = 0u; index < WZ_Z80_V2_PAGE_SIZE; ++index) {
                data[offset + 3u + index] =
                    machine->memory[0x4000u + (page * WZ_Z80_V2_PAGE_SIZE) + index];
            }
            offset += 3u + WZ_Z80_V2_PAGE_SIZE;
        }
    }
    return WZ_RESULT_OK;
}

static wz_result_t wz_z80_128k_map_header(wz_machine_t* machine,
                                           const wz_byte_t* data)
{
    if (machine == 0 || data == 0 || data[6u] != 0u || data[7u] != 0u ||
        wz_read_le16(data + 30u) != 23u || data[34u] != 4u ||
        data[35u] > 0x3fu || data[36u] != 0u || data[53u] > 15u ||
        data[54u] != 0u) {
        return WZ_RESULT_PARSE_ERROR;
    }

    machine->cpu.main.a = data[0u];
    machine->cpu.main.f = data[1u];
    machine->cpu.main.c = data[2u];
    machine->cpu.main.b = data[3u];
    machine->cpu.main.l = data[4u];
    machine->cpu.main.h = data[5u];
    machine->cpu.stack_pointer = wz_read_le16(data + 8u);
    machine->cpu.i = data[10u];
    machine->cpu.r = (wz_byte_t)((data[11u] & 0x7fu) |
                                  ((data[12u] & 0x01u) << 7u));
    machine->border_color = (wz_byte_t)((data[12u] >> 1u) & 0x07u);
    machine->ula_output = machine->border_color;
    machine->cpu.main.e = data[13u];
    machine->cpu.main.d = data[14u];
    machine->cpu.alternate.c = data[15u];
    machine->cpu.alternate.b = data[16u];
    machine->cpu.alternate.e = data[17u];
    machine->cpu.alternate.d = data[18u];
    machine->cpu.alternate.l = data[19u];
    machine->cpu.alternate.h = data[20u];
    machine->cpu.alternate.a = data[21u];
    machine->cpu.alternate.f = data[22u];
    machine->cpu.iy = wz_read_le16(data + 23u);
    machine->cpu.ix = wz_read_le16(data + 25u);
    machine->cpu.iff1 = data[27u];
    machine->cpu.iff2 = data[28u];
    machine->cpu.interrupt_mode = data[29u];
    machine->cpu.program_counter = wz_read_le16(data + 32u);
    machine->paging_7ffd = data[35u];
    machine->paging_7ffd_locked = (wz_byte_t)((data[35u] >> 5u) & 1u);
    machine->ay.selected_register = data[53u];
    for (size_t index = 0u; index < WZ_AY_REGISTER_COUNT; ++index) {
        machine->ay.registers[index] = data[37u + index];
    }
    return wz_z80_state_validate(&machine->cpu);
}

wz_result_t wz_state_load_z80_v2_128k(wz_machine_t* machine,
                                      const wz_byte_t* data,
                                      size_t length)
{
    static wz_machine_t candidate;
    wz_byte_t seen_pages[WZ_Z80_128K_PAGE_COUNT] = {0u};
    size_t offset = WZ_Z80_V2_HEADER_LENGTH;

    if (machine == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (machine->profile == 0 ||
        machine->profile->kind != WZ_MACHINE_128K_PAL ||
        machine->ram_128k == 0 || length < WZ_Z80_V2_HEADER_LENGTH ||
        wz_machine_init(&candidate, wz_machine_profile_128k_pal()) != WZ_RESULT_OK ||
        wz_z80_128k_map_header(&candidate, data) != WZ_RESULT_OK) {
        wz_machine_destroy(&candidate);
        return WZ_RESULT_PARSE_ERROR;
    }

    while (offset < length) {
        wz_word_t block_length;
        wz_byte_t page_number;
        size_t page_index;
        wz_byte_t* destination;

        if (length - offset < 3u) {
            wz_machine_destroy(&candidate);
            return WZ_RESULT_PARSE_ERROR;
        }
        block_length = wz_read_le16(data + offset);
        page_number = data[offset + 2u];
        if (page_number < 3u || page_number > 10u) {
            wz_machine_destroy(&candidate);
            return WZ_RESULT_PARSE_ERROR;
        }
        page_index = (size_t)(page_number - 3u);
        destination = candidate.ram_128k +
                      page_index * WZ_128K_RAM_BANK_SIZE;
        if (seen_pages[page_index] != 0u ||
            (block_length != WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH &&
             block_length == 0u) ||
            (block_length == WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH &&
             length - offset - 3u < WZ_Z80_V2_PAGE_SIZE) ||
            (block_length != WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH &&
             (size_t)block_length > length - offset - 3u)) {
            wz_machine_destroy(&candidate);
            return WZ_RESULT_PARSE_ERROR;
        }
        if (block_length == WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH) {
            for (size_t index = 0u; index < WZ_Z80_V2_PAGE_SIZE; ++index) {
                destination[index] = data[offset + 3u + index];
            }
            offset += 3u + WZ_Z80_V2_PAGE_SIZE;
        } else {
            if (wz_z80_v2_decode_page(destination, data + offset + 3u,
                                       (size_t)block_length) != WZ_RESULT_OK) {
                wz_machine_destroy(&candidate);
                return WZ_RESULT_PARSE_ERROR;
            }
            offset += 3u + (size_t)block_length;
        }
        seen_pages[page_index] = 1u;
    }
    for (size_t page = 0u; page < WZ_Z80_128K_PAGE_COUNT; ++page) {
        if (seen_pages[page] == 0u) {
            wz_machine_destroy(&candidate);
            return WZ_RESULT_PARSE_ERROR;
        }
    }
    wz_sna_128k_apply_candidate(machine, &candidate);
    wz_machine_destroy(&candidate);
    return WZ_RESULT_OK;
}

wz_result_t wz_state_save_z80_v2_128k(const wz_machine_t* machine,
                                      wz_byte_t* data,
                                      size_t capacity)
{
    if (machine == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (capacity < WZ_Z80_128K_V2_LENGTH) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    if (machine->profile == 0 ||
        machine->profile->kind != WZ_MACHINE_128K_PAL ||
        machine->ram_128k == 0 ||
        wz_state_validate_historical_representability(
            machine, WZ_HISTORICAL_FORMAT_Z80) != WZ_RESULT_OK ||
        machine->paging_7ffd > 0x3fu || machine->border_color > 7u ||
        wz_z80_state_validate(&machine->cpu) != WZ_RESULT_OK) {
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
    for (size_t index = 0u; index < WZ_Z80_128K_V2_LENGTH; ++index) {
        data[index] = 0u;
    }
    data[0u] = machine->cpu.main.a;
    data[1u] = machine->cpu.main.f;
    data[2u] = machine->cpu.main.c;
    data[3u] = machine->cpu.main.b;
    data[4u] = machine->cpu.main.l;
    data[5u] = machine->cpu.main.h;
    wz_write_le16(data + 8u, machine->cpu.stack_pointer);
    data[10u] = machine->cpu.i;
    data[11u] = (wz_byte_t)(machine->cpu.r & 0x7fu);
    data[12u] = (wz_byte_t)(((machine->cpu.r >> 7u) & 1u) |
                            (machine->border_color << 1u));
    data[13u] = machine->cpu.main.e;
    data[14u] = machine->cpu.main.d;
    data[15u] = machine->cpu.alternate.c;
    data[16u] = machine->cpu.alternate.b;
    data[17u] = machine->cpu.alternate.e;
    data[18u] = machine->cpu.alternate.d;
    data[19u] = machine->cpu.alternate.l;
    data[20u] = machine->cpu.alternate.h;
    data[21u] = machine->cpu.alternate.a;
    data[22u] = machine->cpu.alternate.f;
    wz_write_le16(data + 23u, machine->cpu.iy);
    wz_write_le16(data + 25u, machine->cpu.ix);
    data[27u] = machine->cpu.iff1;
    data[28u] = machine->cpu.iff2;
    data[29u] = machine->cpu.interrupt_mode;
    wz_write_le16(data + 30u, 23u);
    wz_write_le16(data + 32u, machine->cpu.program_counter);
    data[34u] = 4u;
    data[35u] = machine->paging_7ffd;
    data[53u] = machine->ay.selected_register;
    for (size_t index = 0u; index < WZ_AY_REGISTER_COUNT; ++index) {
        data[37u + index] = machine->ay.registers[index];
    }
    {
        size_t offset = WZ_Z80_V2_HEADER_LENGTH;
        for (wz_byte_t page = 0u; page < WZ_Z80_128K_PAGE_COUNT; ++page) {
            wz_write_le16(data + offset, WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH);
            data[offset + 2u] = (wz_byte_t)(page + 3u);
            for (size_t index = 0u; index < WZ_Z80_V2_PAGE_SIZE; ++index) {
                data[offset + 3u + index] =
                    machine->ram_128k[page * WZ_128K_RAM_BANK_SIZE + index];
            }
            offset += 3u + WZ_Z80_V2_PAGE_SIZE;
        }
    }
    return WZ_RESULT_OK;
}
