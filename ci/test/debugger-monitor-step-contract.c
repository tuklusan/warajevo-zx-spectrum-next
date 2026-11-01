/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_debugger_window.h"
#include "app/wz_host_machine_frame.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned assertions;

static int wz_debugger_monitor_step_target_contract(unsigned* count);
static int wz_debugger_monitor_step_system_contract(unsigned* count);

static wz_debugger_window_result_t command(wz_debugger_window_t* window,
                                            const char* text)
{
    return wz_debugger_window_dispatch_monitor_command(window, text,
                                                        strlen(text));
}

#define CHECK(expression) do { \
    ++assertions; \
    if (!(expression)) { \
        fprintf(stderr, "monitor step contract %d: %s\n", \
                __LINE__, #expression); \
        goto done; \
    } \
} while (0)

#define CHECK_SHARED(count, expression) do { \
    ++(*(count)); \
    if (!(expression)) { \
        fprintf(stderr, "monitor step shared contract %d: %s\n", \
                __LINE__, #expression); \
        goto done; \
    } \
} while (0)

int main(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_master_tick_t tick;
    bool initialized = false;
    int status = 1;

    wz_debugger_window_init(&window);
    if (machine == NULL || wz_machine_init(machine,
            wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return 1;
    }
    initialized = true;
    machine->cpu.program_counter = 0x8000u;
    wz_machine_memory_write(machine, 0x8000u, 0x00u);
    wz_machine_memory_write(machine, 0x8001u, 0x00u);
    wz_machine_memory_write(machine, 0x8002u, 0x00u);
    wz_machine_memory_write(machine, 0x8003u, 0x3cu);
    wz_machine_memory_write(machine, 0x8004u, 0x00u);
    CHECK(wz_debugger_window_open(&window, machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(command(&window, "I EXTRA") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(command(&window, "I 10000") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    tick = machine->master_tick;
    CHECK(command(&window, "I") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.executed == 1u && machine->cpu.program_counter == 0x8001u &&
          machine->master_tick == tick + 8u);
    CHECK(window.snapshot.cpu.program_counter == 0x8001u &&
          window.snapshot.cpu.r == 1u);
    wz_debugger_window_close(&window);
    wz_machine_destroy(machine);
    initialized = false;
    if (wz_debugger_monitor_step_target_contract(&assertions) != 0 ||
        wz_debugger_monitor_step_system_contract(&assertions) != 0) {
        goto done;
    }
    status = 0;

done:
    wz_debugger_window_close(&window);
    if (initialized) wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        printf("debugger-monitor-step-contract assertions=%u status=pass\n",
               assertions);
    }
    return status;
}

static int wz_debugger_monitor_step_target_contract(unsigned* count)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_headless_runner_t runner;
    wz_master_tick_t tick;
    bool initialized = false;
    int status = 1;

    wz_debugger_window_init(&window);
    if (machine == NULL || wz_machine_init(machine,
            wz_machine_profile_48k_pal()) != WZ_RESULT_OK) goto done;
    initialized = true;
    machine->cpu.program_counter = 0x8001u;
    wz_machine_memory_write(machine, 0x8001u, 0x00u);
    wz_machine_memory_write(machine, 0x8002u, 0x00u);
    wz_machine_memory_write(machine, 0x8003u, 0x3cu);
    wz_machine_memory_write(machine, 0x8004u, 0x00u);
    CHECK_SHARED(count, wz_debugger_window_open(&window, machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, command(&window, "B 8004") == WZ_DEBUGGER_WINDOW_OK);
    tick = machine->master_tick;
    CHECK_SHARED(count, command(&window, "I 8003") ==
          WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK_SHARED(count, machine->debugger_step_target_active != 0u &&
          machine->debugger_step_target_address == 0x8003u &&
          wz_debugger_breakpoint_active(machine) &&
          machine->debugger_breakpoint_address == 0x8004u);
    CHECK_SHARED(count, machine->cpu.program_counter == 0x8001u &&
          machine->master_tick == tick);
    CHECK_SHARED(count, wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, machine->debugger_step_target_active == 0u &&
          wz_debugger_breakpoint_active(machine) &&
          machine->cpu.program_counter == 0x8001u && machine->master_tick == tick);
    CHECK_SHARED(count, command(&window, "I 8003") ==
          WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK_SHARED(count, machine->debugger_step_target_active != 0u);
    CHECK_SHARED(count, wz_headless_runner_init(&runner, machine, NULL) == WZ_RESULT_OK);
    CHECK_SHARED(count, wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, wz_host_machine_frame_execute(&runner, 256u, false, NULL, NULL) ==
          WZ_RESULT_BREAKPOINT_HIT);
    CHECK_SHARED(count, machine->cpu.program_counter == 0x8003u &&
          machine->master_tick == tick + 16u && machine->cpu.main.a == 0u);
    CHECK_SHARED(count, wz_debugger_window_pause_at_breakpoint(&window) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, window.paused && window.snapshot.cpu.program_counter == 0x8003u &&
          window.memory_address == 0x8003u && window.breakpoint_hit);
    CHECK_SHARED(count, machine->debugger_step_target_active == 0u &&
          wz_debugger_breakpoint_hit(machine) &&
          wz_debugger_breakpoint_active(machine) &&
          machine->debugger_breakpoint_address == 0x8004u);
    CHECK_SHARED(count, command(&window, "I") == WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, machine->cpu.main.a == 1u && machine->cpu.program_counter == 0x8004u &&
          machine->master_tick == tick + 24u &&
          machine->debugger_breakpoint_address == 0x8004u);
    tick = machine->master_tick;
    CHECK_SHARED(count, command(&window, "I 9000") ==
          WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK_SHARED(count, machine->debugger_step_target_active != 0u &&
          machine->debugger_breakpoint_address == 0x8004u);
    CHECK_SHARED(count, wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, wz_host_machine_frame_execute(&runner, 256u, false, NULL, NULL) ==
          WZ_RESULT_BREAKPOINT_HIT);
    CHECK_SHARED(count, machine->cpu.program_counter == 0x8004u &&
          machine->master_tick == tick && wz_debugger_breakpoint_hit(machine));
    CHECK_SHARED(count, wz_debugger_window_pause_at_breakpoint(&window) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, machine->debugger_step_target_active == 0u &&
          wz_debugger_breakpoint_hit(machine) && window.breakpoint_hit &&
          wz_debugger_breakpoint_active(machine) &&
          machine->debugger_breakpoint_address == 0x8004u);
    status = 0;

done:
    wz_debugger_window_close(&window);
    if (initialized) wz_machine_destroy(machine);
    free(machine);
    return status;
}

static int run_interrupt(unsigned* count)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_headless_runner_t runner;
    bool initialized = false;
    int status = 1;

    wz_debugger_window_init(&window);
    if (machine == NULL || wz_machine_init(machine,
            wz_machine_profile_48k_pal()) != WZ_RESULT_OK) goto done;
    initialized = true;
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.stack_pointer = 0x9000u;
    machine->cpu.iff1 = 1u;
    machine->cpu.iff2 = 1u;
    machine->cpu.interrupt_mode = WZ_Z80_INTERRUPT_MODE_1;
    wz_machine_memory_write(machine, 0x8000u, 0x00u);
    wz_machine_memory_write(machine, 0x0038u, 0x00u);
    CHECK_SHARED(count, wz_debugger_window_open(&window, machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, command(&window, "I 0038") ==
          WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK_SHARED(count, machine->master_tick == 0u && machine->cpu.program_counter == 0x8000u);
    CHECK_SHARED(count, wz_headless_runner_init(&runner, machine, NULL) == WZ_RESULT_OK);
    CHECK_SHARED(count, wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, wz_host_machine_frame_execute(&runner, 256u, false, NULL, NULL) ==
          WZ_RESULT_BREAKPOINT_HIT);
    CHECK_SHARED(count, machine->cpu.program_counter == 0x0038u &&
          machine->cpu.stack_pointer == 0x8ffeu &&
          machine->cpu.iff1 == 0u && machine->cpu.iff2 == 0u &&
          machine->master_tick == 26u);
    CHECK_SHARED(count, wz_machine_memory_read(machine, 0x8ffeu) == 0x00u &&
          wz_machine_memory_read(machine, 0x8fffu) == 0x80u);
    CHECK_SHARED(count, wz_debugger_window_pause_at_breakpoint(&window) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, window.breakpoint_hit && window.memory_address == 0x0038u &&
          window.snapshot.cpu.program_counter == 0x0038u &&
          machine->debugger_step_target_active == 0u);
    status = 0;

done:
    wz_debugger_window_close(&window);
    if (initialized) wz_machine_destroy(machine);
    free(machine);
    return status;
}

static int run_mapping(unsigned* count)
{
    static const wz_byte_t program[] = {
        0x3eu, 0x01u, 0x01u, 0xfdu, 0x7fu, 0xedu, 0x79u, 0xc3u, 0x00u, 0xc0u
    };
    static const wz_byte_t switch_back[] = {
        0x3eu, 0x00u, 0x01u, 0xfdu, 0x7fu, 0xedu, 0x79u
    };
    static const wz_byte_t jump_to_target[] = {0xc3u, 0x00u, 0xc0u};
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_headless_runner_t runner;
    bool initialized = false;
    int status = 1;

    wz_debugger_window_init(&window);
    if (machine == NULL || wz_machine_init(machine,
            wz_machine_profile_128k_pal()) != WZ_RESULT_OK) goto done;
    initialized = true;
    machine->cpu.program_counter = 0x8000u;
    memcpy(machine->ram_128k + 2u * WZ_128K_RAM_BANK_SIZE,
           program, sizeof(program));
    memcpy(machine->ram_128k + WZ_128K_RAM_BANK_SIZE,
           switch_back, sizeof(switch_back));
    memcpy(machine->ram_128k + 7u, jump_to_target, sizeof(jump_to_target));
    CHECK_SHARED(count, wz_debugger_window_open(&window, machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, command(&window, "I C000") ==
          WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK_SHARED(count, machine->debugger_step_target_active != 0u &&
          machine->debugger_step_target_address == 0xc000u &&
          machine->debugger_step_target_mapping == 0x20u);
    CHECK_SHARED(count, wz_headless_runner_init(&runner, machine, NULL) == WZ_RESULT_OK);
    CHECK_SHARED(count, wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, wz_host_machine_frame_execute(&runner, 256u, false, NULL, NULL) ==
          WZ_RESULT_BREAKPOINT_HIT);
    CHECK_SHARED(count, machine->cpu.program_counter == 0xc000u &&
          machine->paging_7ffd == 0u && machine->master_tick > 0u);
    CHECK_SHARED(count, wz_debugger_window_pause_at_breakpoint(&window) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK_SHARED(count, window.paused && window.memory_address == 0xc000u &&
          window.breakpoint_hit && machine->debugger_step_target_active == 0u);
    status = 0;

done:
    wz_debugger_window_close(&window);
    if (initialized) wz_machine_destroy(machine);
    free(machine);
    return status;
}

static int wz_debugger_monitor_step_system_contract(unsigned* count)
{
    return run_interrupt(count) == 0 && run_mapping(count) == 0 ? 0 : 1;
}
