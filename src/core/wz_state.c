/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_state.h"

#include "core/wz_machine.h"

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

    if (wz_state_write_u8(writer, 1u) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, (wz_byte_t)machine->profile->kind) != WZ_RESULT_OK ||
        wz_state_write_u16(writer, machine->cpu.program_counter) != WZ_RESULT_OK ||
        wz_state_write_u16(writer, machine->cpu.stack_pointer) != WZ_RESULT_OK ||
        wz_state_write_u8(writer, machine->cpu.interrupt_enabled) != WZ_RESULT_OK ||
        wz_state_write_u64(writer, machine->master_tick) != WZ_RESULT_OK ||
        wz_state_write(writer, machine->memory, sizeof(machine->memory)) != WZ_RESULT_OK) {
        return WZ_RESULT_SERIALIZATION_FAILURE;
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_state_hash_machine(const wz_machine_t* machine,
                                  wz_qword_t* hash)
{
    wz_byte_t bytes[65536u + 32u];
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
    wz_qword_t tick = 0u;

    if (machine == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length != 65551u || data[0] != 1u) {
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
        tick |= (wz_qword_t)data[7u + index] << (index * 8u);
    }
    machine->profile = profile;
    machine->cpu.program_counter = wz_read_le16(data + 2u);
    machine->cpu.stack_pointer = wz_read_le16(data + 4u);
    machine->cpu.interrupt_enabled = data[6];
    machine->master_tick = tick;
    for (size_t index = 0u; index < sizeof(machine->memory); ++index) {
        machine->memory[index] = data[15u + index];
    }
    return WZ_RESULT_OK;
}
