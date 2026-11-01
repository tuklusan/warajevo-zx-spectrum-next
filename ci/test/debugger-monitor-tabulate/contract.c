/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_debugger_window.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned assertions;

static wz_debugger_window_result_t command(wz_debugger_window_t* window,
                                           const char* text)
{
    return wz_debugger_window_dispatch_monitor_command(window, text,
                                                        strlen(text));
}

static size_t output_lines(const char* output)
{
    size_t count = 0u;
    for (const char* cursor = output; *cursor != '\0'; ++cursor) {
        if (*cursor == '\n') ++count;
    }
    return count;
}

#define CHECK(expression) do { \
    ++assertions; \
    if (!(expression)) { \
        fprintf(stderr, "monitor T contract %d: %s\n", \
                __LINE__, #expression); \
        goto done; \
    } \
} while (0)

int main(void)
{
    static const wz_byte_t sample[] = {
        0x41u, 0x20u, 0x1fu, 0xc2u, 0x00u, 0x7fu, 0x7eu, 0x5au,
        0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u, 0x16u, 0x17u
    };
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_master_tick_t tick;
    wz_z80_state_t cpu;
    wz_byte_t readback[sizeof(sample)];
    bool initialized = false;
    int status = 1;

    wz_debugger_window_init(&window);
    if (machine == NULL) return 1;
    if (wz_machine_init(machine, wz_machine_profile_48k_pal()) !=
        WZ_RESULT_OK) goto done;
    initialized = true;
    machine->cpu.program_counter = 0x8000u;
    for (size_t index = 0u; index < sizeof(sample); ++index) {
        wz_machine_memory_write(machine, (wz_word_t)(0x8000u + index),
                                sample[index]);
    }
    CHECK(wz_debugger_window_open(&window, machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);

    tick = machine->master_tick;
    cpu = machine->cpu;
    CHECK(command(&window, "T") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.tabulation_active && window.disassembly_range_active);
    CHECK(window.memory_address == 0x8000u &&
          window.disassembly_range_end == 0xffffu);
    CHECK(window.disassembly_instruction_count == 24u &&
          window.disassembly_length == 192u &&
          output_lines(wz_debugger_window_disassembly(&window)) == 24u);
    CHECK(strstr(wz_debugger_window_disassembly(&window),
          "8000: 41 20 1F C2 00 7F 7E 5A  |A .B..~Z|\n") != NULL);
    CHECK(strstr(wz_debugger_window_disassembly(&window), "80B8:") != NULL &&
          strstr(wz_debugger_window_disassembly(&window), "80C0:") == NULL);
    CHECK(machine->master_tick == tick &&
          memcmp(&machine->cpu, &cpu, sizeof(cpu)) == 0 &&
          wz_debugger_read_memory_block(machine, 0x8000u, readback,
              sizeof(readback)) == WZ_RESULT_OK &&
          memcmp(readback, sample, sizeof(sample)) == 0);

    CHECK(command(&window, "T 8001 800A") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.memory_address == 0x8001u &&
          window.disassembly_instruction_count == 2u &&
          window.disassembly_length == 10u &&
          output_lines(wz_debugger_window_disassembly(&window)) == 2u);
    CHECK(strstr(wz_debugger_window_disassembly(&window), "8001:") != NULL &&
          strstr(wz_debugger_window_disassembly(&window),
                 "8009: 11 12  |..|\n") != NULL &&
          strstr(wz_debugger_window_disassembly(&window), "800B:") == NULL);

    CHECK(command(&window, "T 8000 80FF 2") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.disassembly_instruction_count == 2u &&
          window.disassembly_length == 16u &&
          output_lines(wz_debugger_window_disassembly(&window)) == 2u &&
          strstr(wz_debugger_window_disassembly(&window), "8008:") != NULL &&
          strstr(wz_debugger_window_disassembly(&window), "8010:") == NULL);
    CHECK(command(&window, "T 8000 800A 0") == WZ_DEBUGGER_WINDOW_OK &&
          window.disassembly_instruction_count == 2u &&
          window.disassembly_length == 11u);
    CHECK(command(&window, "T 8000 80FF -1") == WZ_DEBUGGER_WINDOW_OK &&
          window.disassembly_instruction_count == 24u &&
          window.disassembly_length == 192u);

    CHECK(wz_debugger_window_select_memory(&window, 0x8100u) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(command(&window, "T") == WZ_DEBUGGER_WINDOW_OK &&
          window.memory_address == 0x8100u &&
          strstr(wz_debugger_window_disassembly(&window), "8100:") ==
              wz_debugger_window_disassembly(&window));

    wz_machine_memory_write(machine, 0xfffbu, 0x41u);
    wz_machine_memory_write(machine, 0xfffcu, 0x20u);
    wz_machine_memory_write(machine, 0xfffdu, 0x01u);
    wz_machine_memory_write(machine, 0xfffeu, 0xc2u);
    wz_machine_memory_write(machine, 0xffffu, 0x7eu);
    CHECK(command(&window, "T FFFB FFFF 24") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.disassembly_instruction_count == 1u &&
          window.disassembly_length == 5u &&
          strstr(wz_debugger_window_disassembly(&window),
                 "FFFB: 41 20 01 C2 7E  |A .B~|\n") != NULL &&
          strstr(wz_debugger_window_disassembly(&window), "0000:") == NULL);

    tick = machine->master_tick;
    cpu = machine->cpu;
    CHECK(command(&window, "T 9000 8FFF") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(command(&window, "T 8000 8010 25") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(command(&window, "T 8000 8010 -2") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(command(&window, "T 10000") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(command(&window, "T 8000 GG") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(command(&window, "T 8000 8010 1 EXTRA") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(machine->master_tick == tick &&
          machine->cpu.program_counter == cpu.program_counter &&
          machine->cpu.stack_pointer == cpu.stack_pointer &&
          wz_machine_memory_read(machine, 0xfffbu) == 0x41u);

    CHECK(wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    tick = machine->master_tick;
    cpu = machine->cpu;
    CHECK(command(&window, "T 8000 8007") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(!window.paused && machine->master_tick == tick &&
          memcmp(&machine->cpu, &cpu, sizeof(cpu)) == 0 &&
          wz_machine_memory_read(machine, 0x8000u) == sample[0]);
    status = 0;

done:
    wz_debugger_window_close(&window);
    if (initialized) wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        printf("debugger-monitor-tabulate-contract assertions=%u status=pass\n",
               assertions);
    }
    return status;
}
