/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_state.h"

#include "core/wz_machine.h"

#define WZ_STATE_VERSION 9u
#define WZ_STATE_HEADER_LENGTH 71u
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

wz_result_t wz_state_deserialize_machine(wz_machine_t* machine,
                                         const wz_byte_t* data,
                                         size_t length)
{
    const wz_machine_profile_t* profile;
    wz_z80_state_t cpu;
    wz_qword_t tick = 0u;
    wz_qword_t ula_output_tick = 0u;

    if (machine == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length != WZ_STATE_MACHINE_LENGTH || data[0] != WZ_STATE_VERSION) {
        return WZ_RESULT_INVALID_STATE;
    }
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
    for (size_t index = 0u; index < sizeof(machine->memory); ++index) {
        machine->memory[index] = data[WZ_STATE_HEADER_LENGTH + index];
    }
    return WZ_RESULT_OK;
}
