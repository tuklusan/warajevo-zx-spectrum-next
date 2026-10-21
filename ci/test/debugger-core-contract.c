/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "core/wz_debugger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;

#define CHECK(expr) do { \
    checks++; \
    if (!(expr)) { \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expr); \
        return 1; \
    } \
} while (0)

static wz_machine_t* new_machine(const wz_machine_profile_t* profile)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    if (machine == NULL) return NULL;
    if (wz_machine_init(machine, profile) != WZ_RESULT_OK) {
        free(machine);
        return NULL;
    }
    return machine;
}

static void free_machine(wz_machine_t* machine)
{
    if (machine != NULL) {
        wz_machine_destroy(machine);
        free(machine);
    }
}

static int same_cpu(const wz_z80_state_t* left, const wz_z80_state_t* right)
{
    return left->main.a == right->main.a && left->main.f == right->main.f &&
        left->main.b == right->main.b && left->main.c == right->main.c &&
        left->main.d == right->main.d && left->main.e == right->main.e &&
        left->main.h == right->main.h && left->main.l == right->main.l &&
        left->alternate.a == right->alternate.a &&
        left->alternate.f == right->alternate.f &&
        left->alternate.b == right->alternate.b &&
        left->alternate.c == right->alternate.c &&
        left->alternate.d == right->alternate.d &&
        left->alternate.e == right->alternate.e &&
        left->alternate.h == right->alternate.h &&
        left->alternate.l == right->alternate.l && left->ix == right->ix &&
        left->iy == right->iy && left->stack_pointer == right->stack_pointer &&
        left->program_counter == right->program_counter &&
        left->memptr == right->memptr && left->i == right->i &&
        left->r == right->r && left->iff1 == right->iff1 &&
        left->iff2 == right->iff2 &&
        left->interrupt_enable_delay == right->interrupt_enable_delay &&
        left->interrupt_mode == right->interrupt_mode &&
        left->halted == right->halted && left->q == right->q;
}

static int test_inspection_and_disassembly(void)
{
    wz_machine_t* machine = new_machine(wz_machine_profile_48k_pal());
    wz_debugger_snapshot_t snapshot;
    wz_byte_t block[4] = {0u};
    wz_word_t found = 0u;
    char text[32];
    size_t consumed = 0u;
    wz_master_tick_t tick;
    wz_z80_state_t cpu;

    CHECK(machine != NULL);
    machine->memory[0x8000u] = 0x3eu; /* LD A,$42 */
    machine->memory[0x8001u] = 0x42u;
    machine->memory[0x8002u] = 0xcbu; /* BIT 0,A */
    machine->memory[0x8003u] = 0x47u;
    tick = machine->master_tick;
    cpu = machine->cpu;
    CHECK(wz_debugger_snapshot(machine, &snapshot) == WZ_RESULT_OK);
    CHECK(snapshot.cpu.program_counter == cpu.program_counter);
    CHECK(snapshot.master_tick == tick);
    CHECK(wz_debugger_read_memory_block(machine, 0x8000u, block,
                                        sizeof(block)) == WZ_RESULT_OK);
    CHECK(block[0] == 0x3eu && block[1] == 0x42u &&
          block[2] == 0xcbu && block[3] == 0x47u);
    CHECK(wz_debugger_find_memory(machine, 0x8000u, 4u, 0xcbu, &found) ==
          WZ_RESULT_OK);
    CHECK(found == 0x8002u);
    CHECK(wz_debugger_find_memory(machine, 0x8000u, 4u, 0xffu, &found) ==
          WZ_RESULT_NOT_FOUND);
    CHECK(wz_debugger_read_memory_block(machine, 0xffffu, block, 2u) ==
          WZ_RESULT_BUFFER_TOO_SMALL);
    CHECK(wz_debugger_disassemble(machine, 0x8000u, text, sizeof(text),
                                  &consumed) == WZ_RESULT_OK);
    CHECK(strcmp(text, "LD A,$42") == 0 && consumed == 2u);
    CHECK(wz_debugger_disassemble(machine, 0x8002u, text, sizeof(text),
                                  &consumed) == WZ_RESULT_OK);
    CHECK(strcmp(text, "BIT 0,A") == 0 && consumed == 2u);
    CHECK(wz_debugger_disassemble(machine, 0x8000u, text, 2u, &consumed) ==
          WZ_RESULT_BUFFER_TOO_SMALL);
    CHECK(wz_debugger_format_hex8(0xafu, text, sizeof(text)) == WZ_RESULT_OK);
    CHECK(strcmp(text, "AF") == 0);
    CHECK(wz_debugger_format_hex16(0x1234u, text, sizeof(text)) ==
          WZ_RESULT_OK);
    CHECK(strcmp(text, "1234") == 0);
    CHECK(machine->master_tick == tick && same_cpu(&machine->cpu, &cpu));
    free_machine(machine);
    return 0;
}

static int test_memory_edit_gate_and_ranges(void)
{
    wz_machine_t* machine = new_machine(wz_machine_profile_48k_pal());
    wz_byte_t value = 0u;
    wz_byte_t before[8];

    CHECK(machine != NULL);
    CHECK(wz_debugger_set_access_mode(machine, WZ_DEBUGGER_READ_ONLY) ==
          WZ_RESULT_OK);
    CHECK(wz_debugger_write_memory(machine, 0x8000u, 0x5au) ==
          WZ_RESULT_INVALID_STATE);
    CHECK(wz_debugger_write_memory_word(machine, 0x8000u, 0x1234u) ==
          WZ_RESULT_INVALID_STATE);
    CHECK(wz_debugger_fill_memory(machine, 0x8000u, 4u, 0x33u) ==
          WZ_RESULT_INVALID_STATE);
    CHECK(wz_debugger_copy_memory(machine, 0x8000u, 0x8010u, 4u) ==
          WZ_RESULT_INVALID_STATE);
    CHECK(wz_debugger_set_access_mode(machine, WZ_DEBUGGER_PAUSED_MUTATION) ==
          WZ_RESULT_OK);
    CHECK(wz_debugger_write_memory(machine, 0x3fffu, 0x5au) == WZ_RESULT_OK);
    CHECK(wz_debugger_read_memory(machine, 0x3fffu, &value) == WZ_RESULT_OK &&
          value == 0x5au);
    CHECK(wz_debugger_write_memory(machine, 0x8000u, 0x5au) == WZ_RESULT_OK);
    CHECK(wz_debugger_write_memory_word(machine, 0x8001u, 0x1234u) ==
          WZ_RESULT_OK);
    CHECK(wz_debugger_read_memory(machine, 0x8001u, &value) == WZ_RESULT_OK &&
          value == 0x34u);
    CHECK(wz_debugger_read_memory(machine, 0x8002u, &value) == WZ_RESULT_OK &&
          value == 0x12u);
    CHECK(wz_debugger_fill_memory(machine, 0x8010u, 4u, 0xa5u) ==
          WZ_RESULT_OK);
    CHECK(wz_debugger_read_memory_block(machine, 0x8010u, before, 4u) ==
          WZ_RESULT_OK);
    CHECK(before[0] == 0xa5u && before[1] == 0xa5u &&
          before[2] == 0xa5u && before[3] == 0xa5u);
    machine->memory[0x8020u] = 1u;
    machine->memory[0x8021u] = 2u;
    machine->memory[0x8022u] = 3u;
    machine->memory[0x8023u] = 4u;
    CHECK(wz_debugger_copy_memory(machine, 0x8020u, 0x8021u, 3u) ==
          WZ_RESULT_OK);
    CHECK(machine->memory[0x8020u] == 1u &&
          machine->memory[0x8021u] == 1u &&
          machine->memory[0x8022u] == 2u &&
          machine->memory[0x8023u] == 3u);
    CHECK(wz_debugger_fill_memory(machine, 0xfffeu, 3u, 0u) ==
          WZ_RESULT_INVALID_ARGUMENT);
    CHECK(wz_debugger_copy_memory(machine, 0x8000u, 0xfffeu, 3u) ==
          WZ_RESULT_INVALID_ARGUMENT);
    free_machine(machine);
    return 0;
}

static int test_cpu_edit_undo_and_execution(void)
{
    wz_machine_t* machine = new_machine(wz_machine_profile_48k_pal());
    wz_z80_state_t original;
    wz_z80_state_t edited;
    size_t executed = 99u;

    CHECK(machine != NULL);
    original = machine->cpu;
    edited = original;
    edited.program_counter = 0x8000u;
    edited.main.a = 0x21u;
    CHECK(wz_debugger_set_cpu_state(machine, &edited) == WZ_RESULT_INVALID_STATE);
    CHECK(wz_debugger_jump(machine, 0x8000u) == WZ_RESULT_INVALID_STATE);
    CHECK(wz_debugger_step(machine, &executed) == WZ_RESULT_INVALID_STATE);
    CHECK(wz_debugger_set_access_mode(machine, WZ_DEBUGGER_PAUSED_MUTATION) ==
          WZ_RESULT_OK);
    CHECK(wz_debugger_set_cpu_state(machine, &edited) == WZ_RESULT_OK);
    CHECK(wz_debugger_undo_available(machine));
    CHECK(machine->cpu.program_counter == 0x8000u && machine->cpu.main.a == 0x21u);
    CHECK(wz_debugger_undo_registers(machine) == WZ_RESULT_OK);
    CHECK(!wz_debugger_undo_available(machine));
    CHECK(same_cpu(&machine->cpu, &original));
    CHECK(wz_debugger_undo_registers(machine) == WZ_RESULT_NOT_FOUND);
    machine->memory[0x8000u] = 0x3eu; /* LD A,$42 */
    machine->memory[0x8001u] = 0x42u;
    machine->memory[0x8002u] = 0x00u; /* NOP */
    machine->memory[0x8003u] = 0x76u; /* HALT */
    CHECK(wz_debugger_jump(machine, 0x8000u) == WZ_RESULT_OK);
    CHECK(wz_debugger_step(machine, &executed) == WZ_RESULT_OK && executed == 1u);
    CHECK(machine->cpu.main.a == 0x42u && machine->cpu.program_counter == 0x8002u);
    CHECK(wz_debugger_set_breakpoint(machine, 0x8003u) == WZ_RESULT_OK);
    CHECK(wz_debugger_continue(machine, 4u, &executed) ==
          WZ_RESULT_BREAKPOINT_HIT);
    CHECK(executed == 1u && wz_debugger_breakpoint_hit(machine));
    CHECK(machine->cpu.program_counter == 0x8003u);
    wz_debugger_clear_breakpoint_hit(machine);
    CHECK(!wz_debugger_breakpoint_hit(machine));
    CHECK(wz_debugger_clear_breakpoint(machine) == WZ_RESULT_OK);
    CHECK(!wz_debugger_breakpoint_active(machine));
    CHECK(wz_debugger_continue(machine, 1u, &executed) == WZ_RESULT_OK &&
          executed == 1u);
    CHECK(machine->cpu.halted != 0u);
    free_machine(machine);
    return 0;
}

static int test_128k_page_inspection_and_editing(void)
{
    wz_machine_t* machine = new_machine(wz_machine_profile_128k_pal());
    wz_debugger_page_info_t page;
    wz_byte_t value = 0u;

    CHECK(machine != NULL);
    CHECK(wz_debugger_read_page_info(machine, &page) == WZ_RESULT_OK);
    CHECK(page.paging_value == 0u && page.screen_bank == 5u &&
          page.rom_bank == 0u && page.paging_locked == 0u);
    CHECK(wz_debugger_set_page_config(machine, 0x1fu) == WZ_RESULT_INVALID_STATE);
    CHECK(wz_debugger_set_access_mode(machine, WZ_DEBUGGER_PAUSED_MUTATION) ==
          WZ_RESULT_OK);
    CHECK(wz_debugger_set_page_config(machine, 0x40u) ==
          WZ_RESULT_INVALID_ARGUMENT);
    CHECK(wz_debugger_write_memory(machine, 0x3fffu, 0u) ==
          WZ_RESULT_UNSUPPORTED_OPERATION);
    CHECK(wz_debugger_set_page_config(machine, 0x1fu) == WZ_RESULT_OK);
    CHECK(wz_debugger_read_page_info(machine, &page) == WZ_RESULT_OK);
    CHECK(page.paging_value == 0x1fu && page.screen_bank == 7u &&
          page.rom_bank == 1u && page.paging_locked == 0u);
    CHECK(wz_debugger_write_memory(machine, 0xc000u, 0x71u) == WZ_RESULT_OK);
    CHECK(wz_debugger_read_memory(machine, 0xc000u, &value) == WZ_RESULT_OK &&
          value == 0x71u);
    CHECK(wz_debugger_set_page_config(machine, 0x02u) == WZ_RESULT_OK);
    CHECK(wz_debugger_read_memory(machine, 0xc000u, &value) == WZ_RESULT_OK &&
          value == 0u);
    CHECK(wz_debugger_write_memory(machine, 0xc000u, 0x22u) == WZ_RESULT_OK);
    CHECK(wz_debugger_set_page_config(machine, 0x20u) == WZ_RESULT_OK);
    CHECK(wz_debugger_read_page_info(machine, &page) == WZ_RESULT_OK);
    CHECK(page.paging_value == 0x20u && page.paging_locked == 1u);
    CHECK(wz_debugger_set_page_config(machine, 0x00u) == WZ_RESULT_OK);
    CHECK(wz_debugger_read_page_info(machine, &page) == WZ_RESULT_OK);
    CHECK(page.paging_value == 0x20u && page.paging_locked == 1u);
    CHECK(wz_debugger_read_memory(machine, 0xc000u, &value) == WZ_RESULT_OK &&
          value == 0x22u);
    free_machine(machine);
    return 0;
}

int main(void)
{
    if (test_inspection_and_disassembly() != 0 ||
        test_memory_edit_gate_and_ranges() != 0 ||
        test_cpu_edit_undo_and_execution() != 0 ||
        test_128k_page_inspection_and_editing() != 0) {
        return 1;
    }
    printf("debugger-core-contract assertions=%u status=pass\n", checks);
    return 0;
}
