/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_debugger.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool valid_range(wz_word_t address, size_t length);

static wz_result_t format_hex(char* output, size_t capacity,
                              const char* format, unsigned value)
{
    int written;
    if (output == 0 || capacity == 0u || format == 0) return WZ_RESULT_INVALID_ARGUMENT;
    written = snprintf(output, capacity, format, value);
    if (written < 0) return WZ_RESULT_PARSE_ERROR;
    if ((size_t)written >= capacity) {
        output[capacity - 1u] = '\0';
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_format_hex8(wz_byte_t value, char* output, size_t capacity)
{
    return format_hex(output, capacity, "%02X", (unsigned)value);
}

wz_result_t wz_debugger_format_hex16(wz_word_t value, char* output, size_t capacity)
{
    return format_hex(output, capacity, "%04X", (unsigned)value);
}

static wz_result_t disassembly_result(char* output, size_t capacity,
                                      size_t* consumed, size_t length,
                                      const char* text)
{
    int written;
    if (output == 0 || capacity == 0u || consumed == 0 || text == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    written = snprintf(output, capacity, "%s", text);
    if (written < 0 || (size_t)written >= capacity) {
        output[capacity - 1u] = '\0';
        *consumed = 0u;
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    *consumed = length;
    return WZ_RESULT_OK;
}

static const char* register_name(unsigned code)
{
    static const char* names[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
    return names[code & 7u];
}

static const char* pair_name(unsigned code)
{
    static const char* names[] = {"BC", "DE", "HL", "SP"};
    return names[code & 3u];
}

static wz_result_t disassemble_cb(const wz_byte_t* bytes, char* output,
                                  size_t capacity, size_t* consumed)
{
    unsigned op = bytes[1];
    unsigned group = op >> 6u;
    unsigned bit = (op >> 3u) & 7u;
    const char* operand = register_name(op & 7u);
    char text[64];
    if (group == 0u) {
        static const char* shifts[] = {"RLC", "RRC", "RL", "RR", "SLA", "SRA", "SLL", "SRL"};
        (void)snprintf(text, sizeof(text), "%s %s", shifts[bit], operand);
    } else if (group == 1u) {
        (void)snprintf(text, sizeof(text), "BIT %u,%s", bit, operand);
    } else if (group == 2u) {
        (void)snprintf(text, sizeof(text), "RES %u,%s", bit, operand);
    } else {
        (void)snprintf(text, sizeof(text), "SET %u,%s", bit, operand);
    }
    return disassembly_result(output, capacity, consumed, 2u, text);
}

static wz_result_t disassemble_ed(const wz_byte_t* bytes, char* output,
                                  size_t capacity, size_t* consumed)
{
    unsigned op = bytes[1];
    char text[64];
    if (op == 0x44u) (void)snprintf(text, sizeof(text), "NEG");
    else if (op == 0x45u || op == 0x55u || op == 0x65u || op == 0x75u)
        (void)snprintf(text, sizeof(text), "RETN");
    else if (op == 0x4du) (void)snprintf(text, sizeof(text), "RETI");
    else if (op == 0x46u || op == 0x4eu || op == 0x66u || op == 0x6eu)
        (void)snprintf(text, sizeof(text), "IM 0");
    else if (op == 0x56u || op == 0x76u) (void)snprintf(text, sizeof(text), "IM 1");
    else if (op == 0x5eu || op == 0x7eu) (void)snprintf(text, sizeof(text), "IM 2");
    else if (op == 0x47u) (void)snprintf(text, sizeof(text), "LD I,A");
    else if (op == 0x57u) (void)snprintf(text, sizeof(text), "LD A,I");
    else if (op == 0x5fu) (void)snprintf(text, sizeof(text), "LD A,R");
    else if (op == 0x67u) (void)snprintf(text, sizeof(text), "RRD");
    else if (op == 0x6fu) (void)snprintf(text, sizeof(text), "RLD");
    else return WZ_RESULT_UNSUPPORTED_OPERATION;
    return disassembly_result(output, capacity, consumed, 2u, text);
}

static wz_result_t disassemble_indexed(const wz_byte_t* bytes, char* output,
                                       size_t capacity, size_t* consumed,
                                       const char* index)
{
    unsigned op = bytes[1];
    char text[64];
    unsigned word;
    if (op == 0x21u) {
        word = (unsigned)bytes[2] | ((unsigned)bytes[3] << 8u);
        (void)snprintf(text, sizeof(text), "LD %s,$%04X", index, word);
        return disassembly_result(output, capacity, consumed, 4u, text);
    }
    if (op == 0xe9u) {
        (void)snprintf(text, sizeof(text), "JP (%s)", index);
        return disassembly_result(output, capacity, consumed, 2u, text);
    }
    if (op == 0x22u || op == 0x2au) {
        word = (unsigned)bytes[2] | ((unsigned)bytes[3] << 8u);
        (void)snprintf(text, sizeof(text), "%s ($%04X),%s", op == 0x22u ? "LD" : "LD", word,
                       op == 0x22u ? index : "");
        if (op == 0x2au) (void)snprintf(text, sizeof(text), "LD %s,($%04X)", index, word);
        else (void)snprintf(text, sizeof(text), "LD ($%04X),%s", word, index);
        return disassembly_result(output, capacity, consumed, 4u, text);
    }
    return WZ_RESULT_UNSUPPORTED_OPERATION;
}

wz_result_t wz_debugger_disassemble(const wz_machine_t* machine,
                                    wz_word_t address,
                                    char* output,
                                    size_t capacity,
                                    size_t* consumed)
{
    wz_byte_t bytes[4];
    unsigned op;
    char text[64];
    if (machine == 0 || output == 0 || capacity == 0u || consumed == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (!valid_range(address, sizeof(bytes))) return WZ_RESULT_BUFFER_TOO_SMALL;
    for (size_t i = 0u; i < sizeof(bytes); ++i) {
        bytes[i] = wz_machine_memory_read(machine, (wz_word_t)(address + i));
    }
    op = bytes[0];
    if (op == 0xcbu) return disassemble_cb(bytes, output, capacity, consumed);
    if (op == 0xedu) return disassemble_ed(bytes, output, capacity, consumed);
    if (op == 0xddu) return disassemble_indexed(bytes, output, capacity, consumed, "IX");
    if (op == 0xfdu) return disassemble_indexed(bytes, output, capacity, consumed, "IY");
    if (op == 0x00u) (void)snprintf(text, sizeof(text), "NOP");
    else if (op == 0x76u) (void)snprintf(text, sizeof(text), "HALT");
    else if ((op & 0xc7u) == 0x06u) {
        (void)snprintf(text, sizeof(text), "LD %s,$%02X", register_name((op >> 3u) & 7u), bytes[1]);
        return disassembly_result(output, capacity, consumed, 2u, text);
    } else if ((op & 0xc7u) == 0x01u) {
        unsigned word = (unsigned)bytes[1] | ((unsigned)bytes[2] << 8u);
        (void)snprintf(text, sizeof(text), "LD %s,$%04X", pair_name((op >> 4u) & 3u), word);
        return disassembly_result(output, capacity, consumed, 3u, text);
    } else if ((op & 0xc0u) == 0x40u) {
        (void)snprintf(text, sizeof(text), "LD %s,%s", register_name((op >> 3u) & 7u), register_name(op & 7u));
    } else if ((op & 0xc0u) == 0x80u) {
        static const char* alu[] = {"ADD A,", "ADC A,", "SUB ", "SBC A,", "AND ", "XOR ", "OR ", "CP "};
        (void)snprintf(text, sizeof(text), "%s%s", alu[(op >> 3u) & 7u], register_name(op & 7u));
    } else if (op == 0xc3u) {
        unsigned word = (unsigned)bytes[1] | ((unsigned)bytes[2] << 8u);
        (void)snprintf(text, sizeof(text), "JP $%04X", word);
        return disassembly_result(output, capacity, consumed, 3u, text);
    } else if (op == 0x18u) {
        (void)snprintf(text, sizeof(text), "JR %+d", (int)(int8_t)bytes[1]);
        return disassembly_result(output, capacity, consumed, 2u, text);
    } else if (op == 0xc9u) (void)snprintf(text, sizeof(text), "RET");
    else if (op == 0xafu) (void)snprintf(text, sizeof(text), "XOR A");
    else {
        (void)snprintf(text, sizeof(text), "DB $%02X", op);
    }
    return disassembly_result(output, capacity, consumed, 1u, text);
}

static bool valid_range(wz_word_t address, size_t length)
{
    return length <= 65536u - (size_t)address;
}

static bool writable_range(const wz_machine_t* machine,
                           wz_word_t address, size_t length)
{
    if (machine == 0 || machine->profile == 0 || !valid_range(address, length)) {
        return false;
    }
    if (address < WZ_48K_ROM_SIZE &&
        (machine->profile->kind == WZ_MACHINE_128K_PAL ||
         machine->has_48k_rom != 0u)) {
        return false;
    }
    return length == 0u || address >= WZ_48K_ROM_SIZE;
}

static void wz_debugger_trace_result(wz_machine_t* machine,
                                     wz_byte_t operation,
                                     wz_result_t result,
                                     const wz_z80_state_t* state)
{
    wz_trace_event_t event;

    if (machine == 0 || machine->timing_trace == 0) {
        return;
    }
    event.kind = WZ_TRACE_DEVELOPER_MARKER;
    event.master_tick = machine->master_tick;
    event.sequence = 0u;
    event.address = 0u;
    event.program_counter = state == 0 ? machine->cpu.program_counter
                                       : state->program_counter;
    event.stack_pointer = state == 0 ? machine->cpu.stack_pointer
                                     : state->stack_pointer;
    event.register_snapshot = 0u;
    event.value = (wz_byte_t)result;
    event.auxiliary = operation;
    event.cycle = 0u;
    event.t_states = 0u;
    wz_trace_emit_detail(machine->timing_trace, &event);
}

static void wz_debugger_trace_memory_result(wz_machine_t* machine,
                                            wz_word_t address,
                                            wz_byte_t value,
                                            wz_result_t result)
{
    wz_trace_event_t event;

    if (machine == 0 || machine->timing_trace == 0) {
        return;
    }
    event.kind = WZ_TRACE_DEVELOPER_MARKER;
    event.master_tick = machine->master_tick;
    event.sequence = 0u;
    event.address = address;
    event.program_counter = machine->cpu.program_counter;
    event.stack_pointer = machine->cpu.stack_pointer;
    event.register_snapshot = 0u;
    event.value = value;
    event.auxiliary = (wz_byte_t)result;
    event.cycle = WZ_DEBUGGER_TRACE_MEMORY_MUTATION;
    event.t_states = 0u;
    wz_trace_emit_detail(machine->timing_trace, &event);
}

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

wz_result_t wz_debugger_find_memory(const wz_machine_t* machine,
                                    wz_word_t address,
                                    size_t length,
                                    wz_byte_t value,
                                    wz_word_t* found_address)
{
    return wz_debugger_find_memory_pattern(machine, address, length, &value,
                                           0, 1u, found_address);
}

wz_result_t wz_debugger_find_memory_pattern(const wz_machine_t* machine,
                                            wz_word_t address,
                                            size_t length,
                                            const wz_byte_t* pattern,
                                            const wz_byte_t* mask,
                                            size_t pattern_length,
                                            wz_word_t* found_address)
{
    size_t offset;

    if (machine == 0 || found_address == 0 || pattern == 0 ||
        pattern_length == 0u || !valid_range(address, length)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (pattern_length > length) {
        return WZ_RESULT_NOT_FOUND;
    }
    for (offset = 0u; offset <= length - pattern_length; ++offset) {
        size_t byte_index;
        for (byte_index = 0u; byte_index < pattern_length; ++byte_index) {
            wz_byte_t expected = pattern[byte_index];
            wz_byte_t byte_mask = mask == 0 ? 0xffu : mask[byte_index];
            wz_byte_t actual = wz_machine_memory_read(
                machine, (wz_word_t)(address + offset + byte_index));
            if ((actual & byte_mask) != (expected & byte_mask)) {
                break;
            }
        }
        if (byte_index == pattern_length) {
            *found_address = (wz_word_t)(address + offset);
            return WZ_RESULT_OK;
        }
    }
    return WZ_RESULT_NOT_FOUND;
}

wz_result_t wz_debugger_set_access_mode(wz_machine_t* machine,
                                         wz_debugger_access_mode_t mode)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (mode != WZ_DEBUGGER_READ_ONLY &&
        mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_ACCESS_MODE,
                                 WZ_RESULT_INVALID_ARGUMENT, 0);
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    machine->debugger_access_mode = (wz_byte_t)mode;
    wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_ACCESS_MODE,
                             WZ_RESULT_OK, 0);
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_set_cpu_state(wz_machine_t* machine,
                                      const wz_z80_state_t* state)
{
    wz_result_t result;

    if (machine == 0 || state == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        result = WZ_RESULT_INVALID_STATE;
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_CPU_MUTATION,
                                 result, state);
        return result;
    }
    result = wz_z80_state_validate(state);
    if (result != WZ_RESULT_OK) {
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_CPU_MUTATION,
                                 result, state);
        return result;
    }
    machine->debugger_undo_cpu = machine->cpu;
    machine->debugger_undo_valid = 1u;
    machine->cpu = *state;
    wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_CPU_MUTATION,
                             WZ_RESULT_OK, state);
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_set_register(wz_machine_t* machine,
                                     wz_debugger_register_t reg,
                                     wz_word_t value)
{
    wz_z80_state_t state;
    wz_byte_t* byte_register = 0;

    if (machine == 0 || (int)reg < 0 || reg >= WZ_DEBUGGER_REGISTER_COUNT) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_CPU_MUTATION,
                                 WZ_RESULT_INVALID_STATE, &machine->cpu);
        return WZ_RESULT_INVALID_STATE;
    }
    state = machine->cpu;
    switch (reg) {
        case WZ_DEBUGGER_REGISTER_MAIN_A: byte_register = &state.main.a; break;
        case WZ_DEBUGGER_REGISTER_MAIN_F: byte_register = &state.main.f; break;
        case WZ_DEBUGGER_REGISTER_MAIN_B: byte_register = &state.main.b; break;
        case WZ_DEBUGGER_REGISTER_MAIN_C: byte_register = &state.main.c; break;
        case WZ_DEBUGGER_REGISTER_MAIN_D: byte_register = &state.main.d; break;
        case WZ_DEBUGGER_REGISTER_MAIN_E: byte_register = &state.main.e; break;
        case WZ_DEBUGGER_REGISTER_MAIN_H: byte_register = &state.main.h; break;
        case WZ_DEBUGGER_REGISTER_MAIN_L: byte_register = &state.main.l; break;
        case WZ_DEBUGGER_REGISTER_ALT_A: byte_register = &state.alternate.a; break;
        case WZ_DEBUGGER_REGISTER_ALT_F: byte_register = &state.alternate.f; break;
        case WZ_DEBUGGER_REGISTER_ALT_B: byte_register = &state.alternate.b; break;
        case WZ_DEBUGGER_REGISTER_ALT_C: byte_register = &state.alternate.c; break;
        case WZ_DEBUGGER_REGISTER_ALT_D: byte_register = &state.alternate.d; break;
        case WZ_DEBUGGER_REGISTER_ALT_E: byte_register = &state.alternate.e; break;
        case WZ_DEBUGGER_REGISTER_ALT_H: byte_register = &state.alternate.h; break;
        case WZ_DEBUGGER_REGISTER_ALT_L: byte_register = &state.alternate.l; break;
        case WZ_DEBUGGER_REGISTER_IX: state.ix = value; break;
        case WZ_DEBUGGER_REGISTER_IY: state.iy = value; break;
        case WZ_DEBUGGER_REGISTER_SP: state.stack_pointer = value; break;
        case WZ_DEBUGGER_REGISTER_PC: state.program_counter = value; break;
        case WZ_DEBUGGER_REGISTER_I: byte_register = &state.i; break;
        case WZ_DEBUGGER_REGISTER_R: byte_register = &state.r; break;
        case WZ_DEBUGGER_REGISTER_IFF1: byte_register = &state.iff1; break;
        case WZ_DEBUGGER_REGISTER_IFF2: byte_register = &state.iff2; break;
        case WZ_DEBUGGER_REGISTER_INTERRUPT_ENABLE_DELAY:
            byte_register = &state.interrupt_enable_delay;
            break;
        case WZ_DEBUGGER_REGISTER_INTERRUPT_MODE:
            if (value > WZ_Z80_INTERRUPT_MODE_2) return WZ_RESULT_INVALID_ARGUMENT;
            byte_register = &state.interrupt_mode;
            break;
        case WZ_DEBUGGER_REGISTER_COUNT:
        default:
            return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (byte_register != 0) {
        if (value > UINT8_MAX ||
            ((reg == WZ_DEBUGGER_REGISTER_IFF1 ||
              reg == WZ_DEBUGGER_REGISTER_IFF2 ||
              reg == WZ_DEBUGGER_REGISTER_INTERRUPT_ENABLE_DELAY) && value > 1u)) {
            return WZ_RESULT_INVALID_ARGUMENT;
        }
        *byte_register = (wz_byte_t)value;
    }
    return wz_debugger_set_cpu_state(machine, &state);
}

wz_result_t wz_debugger_jump(wz_machine_t* machine, wz_word_t address)
{
    if (machine == 0) return WZ_RESULT_INVALID_ARGUMENT;
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_JUMP,
                                 WZ_RESULT_INVALID_STATE, 0);
        return WZ_RESULT_INVALID_STATE;
    }
    machine->cpu.program_counter = address;
    wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_JUMP, WZ_RESULT_OK, 0);
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_undo_registers(wz_machine_t* machine)
{
    if (machine == 0) return WZ_RESULT_INVALID_ARGUMENT;
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_UNDO,
                                 WZ_RESULT_INVALID_STATE, 0);
        return WZ_RESULT_INVALID_STATE;
    }
    if (machine->debugger_undo_valid == 0u) {
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_UNDO,
                                 WZ_RESULT_NOT_FOUND, 0);
        return WZ_RESULT_NOT_FOUND;
    }
    machine->cpu = machine->debugger_undo_cpu;
    machine->debugger_undo_valid = 0u;
    wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_UNDO, WZ_RESULT_OK, 0);
    return WZ_RESULT_OK;
}

bool wz_debugger_undo_available(const wz_machine_t* machine)
{
    return machine != 0 && machine->debugger_undo_valid != 0u;
}

wz_result_t wz_debugger_read_page_info(const wz_machine_t* machine,
                                       wz_debugger_page_info_t* info)
{
    if (machine == 0 || info == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    info->paging_value = wz_machine_128k_paging_value(machine);
    info->screen_bank = wz_machine_128k_screen_bank(machine);
    info->rom_bank = wz_machine_128k_rom_bank(machine);
    info->paging_locked = machine->profile != 0 &&
        machine->profile->kind == WZ_MACHINE_128K_PAL &&
        machine->paging_7ffd_locked != 0u;
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_set_page_config(wz_machine_t* machine,
                                        wz_byte_t paging_value)
{
    wz_result_t result;

    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_PAGE,
                                 WZ_RESULT_INVALID_STATE, 0);
        return WZ_RESULT_INVALID_STATE;
    }
    if (machine->profile == 0 ||
        machine->profile->kind != WZ_MACHINE_128K_PAL) {
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_PAGE,
                                 WZ_RESULT_UNSUPPORTED_OPERATION, 0);
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
    if ((paging_value & (wz_byte_t)~0x3fu) != 0u) {
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_PAGE,
                                 WZ_RESULT_INVALID_ARGUMENT, 0);
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    result = wz_machine_128k_paging_write(machine, 0x7ffdu, paging_value);
    wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_PAGE, result, 0);
    return result;
}

wz_result_t wz_debugger_write_memory(wz_machine_t* machine,
                                     wz_word_t address,
                                     wz_byte_t value)
{
    wz_result_t result;

    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        result = WZ_RESULT_INVALID_STATE;
        wz_debugger_trace_memory_result(machine, address, value, result);
        return result;
    }
    if (machine->profile == 0 ||
        (address < WZ_48K_ROM_SIZE &&
         (machine->profile->kind == WZ_MACHINE_128K_PAL ||
          machine->has_48k_rom != 0u))) {
        result = WZ_RESULT_UNSUPPORTED_OPERATION;
        wz_debugger_trace_memory_result(machine, address, value, result);
        return result;
    }
    wz_machine_memory_write(machine, address, value);
    wz_debugger_trace_memory_result(machine, address, value, WZ_RESULT_OK);
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_write_memory_bytes(wz_machine_t* machine,
                                           wz_word_t address,
                                           const wz_byte_t* values,
                                           const bool* write_mask,
                                           size_t length)
{
    wz_result_t result;

    if (machine == 0 || values == 0 || length == 0u ||
        !valid_range(address, length)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        result = WZ_RESULT_INVALID_STATE;
    } else if (!writable_range(machine, address, length)) {
        result = WZ_RESULT_UNSUPPORTED_OPERATION;
    } else {
        for (size_t index = 0u; index < length; ++index) {
            if (write_mask == 0 || write_mask[index]) {
                wz_word_t current = (wz_word_t)(address + index);
                wz_machine_memory_write(machine, current, values[index]);
                wz_debugger_trace_memory_result(machine, current,
                                                values[index], WZ_RESULT_OK);
            }
        }
        return WZ_RESULT_OK;
    }
    wz_debugger_trace_memory_result(machine, address, values[0], result);
    return result;
}

wz_result_t wz_debugger_write_memory_word(wz_machine_t* machine,
                                          wz_word_t address,
                                          wz_word_t value)
{
    wz_result_t result;
    if (machine == 0 || !valid_range(address, 2u)) return WZ_RESULT_INVALID_ARGUMENT;
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        result = WZ_RESULT_INVALID_STATE;
    } else if (!writable_range(machine, address, 2u)) {
        result = WZ_RESULT_UNSUPPORTED_OPERATION;
    } else {
        wz_machine_memory_write(machine, address, (wz_byte_t)value);
        wz_machine_memory_write(machine, (wz_word_t)(address + 1u),
                                (wz_byte_t)(value >> 8u));
        result = WZ_RESULT_OK;
    }
    wz_debugger_trace_memory_result(machine, address, (wz_byte_t)value, result);
    return result;
}

wz_result_t wz_debugger_fill_memory(wz_machine_t* machine,
                                    wz_word_t address,
                                    size_t length,
                                    wz_byte_t value)
{
    wz_result_t result;
    if (machine == 0 || !valid_range(address, length)) return WZ_RESULT_INVALID_ARGUMENT;
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        result = WZ_RESULT_INVALID_STATE;
    } else if (!writable_range(machine, address, length)) {
        result = WZ_RESULT_UNSUPPORTED_OPERATION;
    } else {
        for (size_t index = 0u; index < length; ++index) {
            wz_machine_memory_write(machine, (wz_word_t)(address + index), value);
        }
        result = WZ_RESULT_OK;
    }
    wz_debugger_trace_memory_result(machine, address, value, result);
    return result;
}

wz_result_t wz_debugger_copy_memory(wz_machine_t* machine,
                                    wz_word_t source_address,
                                    wz_word_t destination_address,
                                    size_t length)
{
    wz_result_t result;
    wz_byte_t* copy = 0;
    if (machine == 0 || !valid_range(source_address, length) ||
        !valid_range(destination_address, length)) return WZ_RESULT_INVALID_ARGUMENT;
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        result = WZ_RESULT_INVALID_STATE;
    } else if (!writable_range(machine, destination_address, length)) {
        result = WZ_RESULT_UNSUPPORTED_OPERATION;
    } else {
        bool interface1_overlay =
            machine->networking_mode == WZ_NETWORKING_INTERFACE1 &&
            machine->has_interface1_rom != 0u &&
            machine->interface1_rom_page != 0u;
        if (machine->profile != 0 &&
            machine->profile->kind != WZ_MACHINE_128K_PAL &&
            !interface1_overlay) {
            memmove(&machine->memory[destination_address],
                    &machine->memory[source_address], length);
        } else if (length > 0u) {
            copy = (wz_byte_t*)malloc(length);
            if (copy == 0) {
                wz_debugger_trace_memory_result(machine, destination_address,
                                                0u, WZ_RESULT_OUT_OF_MEMORY);
                return WZ_RESULT_OUT_OF_MEMORY;
            }
            for (size_t index = 0u; index < length; ++index) {
                copy[index] = wz_machine_memory_read(
                    machine, (wz_word_t)(source_address + index));
            }
            for (size_t index = 0u; index < length; ++index) {
                wz_machine_memory_write(
                    machine, (wz_word_t)(destination_address + index),
                    copy[index]);
            }
            free(copy);
        }
        result = WZ_RESULT_OK;
    }
    wz_debugger_trace_memory_result(machine, destination_address,
                                    length == 0u ? 0u : wz_machine_memory_read(
                                        machine, destination_address),
                                    result);
    return result;
}

wz_result_t wz_debugger_set_breakpoint(wz_machine_t* machine,
                                       wz_word_t address)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    machine->debugger_breakpoint_address = address;
    machine->debugger_breakpoint_active = 1u;
    machine->debugger_breakpoint_hit = 0u;
    wz_debugger_trace_result(machine, WZ_TRACE_DEBUGGER_BREAKPOINT_HIT,
                             WZ_RESULT_OK, 0);
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_clear_breakpoint(wz_machine_t* machine)
{
    if (machine == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    machine->debugger_breakpoint_active = 0u;
    machine->debugger_breakpoint_hit = 0u;
    wz_debugger_trace_result(machine, WZ_TRACE_DEBUGGER_BREAKPOINT_HIT,
                             WZ_RESULT_OK, 0);
    return WZ_RESULT_OK;
}

bool wz_debugger_breakpoint_active(const wz_machine_t* machine)
{
    return machine != 0 && machine->debugger_breakpoint_active != 0u;
}

bool wz_debugger_breakpoint_hit(const wz_machine_t* machine)
{
    return machine != 0 && machine->debugger_breakpoint_hit != 0u;
}

void wz_debugger_clear_breakpoint_hit(wz_machine_t* machine)
{
    if (machine != 0) {
        machine->debugger_breakpoint_hit = 0u;
    }
}

wz_result_t wz_debugger_step(wz_machine_t* machine, size_t* executed)
{
    wz_result_t result;

    if (machine == 0 || executed == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *executed = 0u;
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        return WZ_RESULT_INVALID_STATE;
    }
    result = wz_z80_step(machine);
    if (result != WZ_RESULT_OK) {
        return result;
    }
    if (machine->debugger_breakpoint_hit != 0u) {
        wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_STEP,
                                 WZ_RESULT_BREAKPOINT_HIT, 0);
        return WZ_RESULT_BREAKPOINT_HIT;
    }
    *executed = 1u;
    wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_STEP,
                             WZ_RESULT_OK, 0);
    return WZ_RESULT_OK;
}

wz_result_t wz_debugger_continue(wz_machine_t* machine,
                                 size_t max_instructions,
                                 size_t* executed)
{
    wz_result_t result;

    if (machine == 0 || executed == 0 || max_instructions == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *executed = 0u;
    if (machine->debugger_access_mode != WZ_DEBUGGER_PAUSED_MUTATION) {
        return WZ_RESULT_INVALID_STATE;
    }
    while (*executed < max_instructions) {
        result = wz_z80_step(machine);
        if (result != WZ_RESULT_OK) {
            return result;
        }
        if (machine->debugger_breakpoint_hit != 0u) {
            wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_CONTINUE,
                                     WZ_RESULT_BREAKPOINT_HIT, 0);
            return WZ_RESULT_BREAKPOINT_HIT;
        }
        ++*executed;
    }
    wz_debugger_trace_result(machine, WZ_DEBUGGER_TRACE_CONTINUE,
                             WZ_RESULT_OK, 0);
    return WZ_RESULT_OK;
}
