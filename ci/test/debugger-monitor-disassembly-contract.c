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

static unsigned checks;

#define CHECK(expression) do { \
    ++checks; \
    if (!(expression)) { \
        fprintf(stderr, "debugger-monitor-disassembly-contract failed at %s:%d: %s\n", \
                __FILE__, __LINE__, #expression); \
        goto cleanup; \
    } \
} while (0)

#define DISPATCH(window, text) \
    wz_debugger_window_dispatch_monitor_command(&(window), (text), \
                                                 sizeof(text) - 1u)

int main(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_z80_state_t cpu_before;
    wz_master_tick_t tick_before;
    bool window_initialized = false;
    int status = 1;

    if (machine == NULL ||
        wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return 1;
    }
    wz_debugger_window_init(&window);
    window_initialized = true;
    machine->memory[0x8000u] = 0x3eu; /* LD A,$42 */
    machine->memory[0x8001u] = 0x42u;
    machine->memory[0x8002u] = 0xcbu; /* BIT 0,A */
    machine->memory[0x8003u] = 0x47u;
    machine->memory[0x8004u] = 0xddu; /* LD IX,$1234 */
    machine->memory[0x8005u] = 0x21u;
    machine->memory[0x8006u] = 0x34u;
    machine->memory[0x8007u] = 0x12u;
    machine->memory[0x8008u] = 0xfdu; /* LD IY,($7856) */
    machine->memory[0x8009u] = 0x2au;
    machine->memory[0x800au] = 0x56u;
    machine->memory[0x800bu] = 0x78u;
    machine->memory[0xfffeu] = 0x3eu;
    machine->memory[0xffffu] = 0x7eu;
    machine->memory[0u] = 0xa5u;
    cpu_before = machine->cpu;
    tick_before = machine->master_tick;
    CHECK(wz_debugger_window_open(&window, machine, false) ==
          WZ_DEBUGGER_WINDOW_OK);

    CHECK(DISPATCH(window, "D 8000 8008") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.memory_address == 0x8000u &&
          window.disassembly_instruction_count == 5u &&
          window.disassembly_length == 12u);
    CHECK(strstr(window.disassembly, "8000: LD A,$42") != NULL &&
          strstr(window.disassembly, "8002: BIT 0,A") != NULL &&
          strstr(window.disassembly, "8004: LD IX,$1234") != NULL &&
          strstr(window.disassembly, "8008: LD IY,($7856)") != NULL);
    CHECK(machine->master_tick == tick_before &&
          memcmp(&machine->cpu, &cpu_before, sizeof(cpu_before)) == 0 &&
          machine->memory[0x8000u] == 0x3eu &&
          machine->memory[0x800bu] == 0x78u);

    CHECK(DISPATCH(window, "D 8008 8008") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.disassembly_instruction_count == 1u &&
          window.disassembly_length == 4u &&
          strstr(window.disassembly, "8008: LD IY,($7856)") != NULL);
    CHECK(DISPATCH(window, "D 8000") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.disassembly_instruction_count ==
          WZ_DEBUGGER_WINDOW_DISASSEMBLY_LINES &&
          strstr(window.disassembly, "800C: NOP") != NULL);
    CHECK(DISPATCH(window, "D") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.memory_address == 0x8000u &&
          window.disassembly_instruction_count ==
              WZ_DEBUGGER_WINDOW_DISASSEMBLY_LINES);

    CHECK(DISPATCH(window, "D 8008 8000") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(window.memory_address == 0x8000u &&
          window.disassembly_instruction_count ==
              WZ_DEBUGGER_WINDOW_DISASSEMBLY_LINES);
    CHECK(DISPATCH(window, "D 10000") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(DISPATCH(window, "D 8000 8008 EXTRA") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);

    CHECK(DISPATCH(window, "D FFFE FFFF") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.disassembly_instruction_count == 1u &&
          window.disassembly_length == 2u &&
          strstr(window.disassembly, "FFFE: LD A,$7E") != NULL);
    machine->memory[0xffffu] = 0x3eu;
    CHECK(DISPATCH(window, "D FFFF FFFF") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.disassembly_instruction_count == 1u &&
          window.disassembly_length == 2u &&
          strstr(window.disassembly, "FFFF: LD A,$A5") != NULL);
    CHECK(machine->master_tick == tick_before &&
          memcmp(&machine->cpu, &cpu_before, sizeof(cpu_before)) == 0);

    wz_debugger_window_close(&window);
    CHECK(DISPATCH(window, "D 8000 8000") ==
          WZ_DEBUGGER_WINDOW_NOT_OPEN);
    status = 0;
cleanup:
    if (window_initialized) wz_debugger_window_close(&window);
    wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        printf("debugger-monitor-disassembly-contract assertions=%u status=pass\n",
               checks);
    }
    return status;
}
