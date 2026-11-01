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

static wz_debugger_window_result_t command(wz_debugger_window_t* window,
                                           const char* text)
{
    return wz_debugger_window_dispatch_monitor_command(window, text,
                                                        strlen(text));
}

#define CHECK(expression) do { \
    ++assertions; \
    if (!(expression)) { \
        fprintf(stderr, "monitor jump contract %d: %s\n", \
                __LINE__, #expression); \
        goto done; \
    } \
} while (0)

static int instruction_length_matrix(wz_machine_t* machine)
{
    static const struct {
        wz_word_t address;
        wz_byte_t bytes[5];
        size_t length;
    } cases[] = {
        {0x8200u, {0x01u, 0x34u, 0x12u}, 3u},
        {0x8210u, {0x20u, 0xfeu}, 2u},
        {0x8220u, {0xccu, 0x34u, 0x12u}, 3u},
        {0x8230u, {0xdbu, 0xfeu}, 2u},
        {0x8240u, {0xedu, 0x4bu, 0x34u, 0x12u}, 4u},
        {0x8250u, {0xddu, 0x21u, 0x34u, 0x12u}, 4u},
        {0x8260u, {0xfdu, 0x46u, 0x05u}, 3u},
        {0x8270u, {0xddu, 0x36u, 0x05u, 0x42u}, 4u},
        {0x8280u, {0xddu, 0xcbu, 0x05u, 0x46u}, 4u},
        {0x8290u, {0xddu, 0xfdu, 0x21u, 0x34u, 0x12u}, 5u}
    };

    for (size_t i = 0u; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        size_t actual = 0u;
        for (size_t j = 0u; j < cases[i].length; ++j) {
            wz_machine_memory_write(machine,
                (wz_word_t)((size_t)cases[i].address + j), cases[i].bytes[j]);
        }
        ++assertions;
        if (wz_debugger_instruction_length(machine, cases[i].address,
                &actual) != WZ_RESULT_OK || actual != cases[i].length) {
            fprintf(stderr, "monitor jump length contract %d: case %u expected %u got %u\n",
                    __LINE__, (unsigned)i, (unsigned)cases[i].length,
                    (unsigned)actual);
            return 1;
        }
    }
    return 0;
}

static int default_call_step_over(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_headless_runner_t runner;
    wz_master_tick_t tick;
    size_t disassembled_length = 0u;
    char instruction[WZ_DEBUGGER_WINDOW_DISASSEMBLY_CAPACITY];
    bool initialized = false;
    int status = 1;

    wz_debugger_window_init(&window);
    if (machine == NULL) return 1;
    if (wz_machine_init(machine, wz_machine_profile_48k_pal()) !=
        WZ_RESULT_OK) goto done;
    initialized = true;
    machine->cpu.program_counter = 0x8000u;
    machine->cpu.stack_pointer = 0x9000u;
    wz_machine_memory_write(machine, 0x8000u, 0xcdu);
    wz_machine_memory_write(machine, 0x8001u, 0x00u);
    wz_machine_memory_write(machine, 0x8002u, 0x81u);
    wz_machine_memory_write(machine, 0x8003u, 0x3eu);
    wz_machine_memory_write(machine, 0x8004u, 0x42u);
    wz_machine_memory_write(machine, 0x8100u, 0x3cu);
    wz_machine_memory_write(machine, 0x8101u, 0xc9u);

    CHECK(wz_debugger_disassemble(machine, 0x8000u, instruction,
              sizeof(instruction), &disassembled_length) == WZ_RESULT_OK &&
          disassembled_length == 3u);
    if (instruction_length_matrix(machine) != 0) goto done;

    CHECK(wz_debugger_window_open(&window, machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(command(&window, "B 8003") == WZ_DEBUGGER_WINDOW_OK);
    tick = machine->master_tick;
    CHECK(command(&window, "J") == WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK(machine->debugger_step_target_active != 0u &&
          machine->debugger_step_target_address == 0x8003u &&
          machine->debugger_step_target_mapping ==
              wz_machine_debugger_mapping_key(machine, 0x8003u));
    CHECK(machine->cpu.program_counter == 0x8000u &&
          machine->cpu.stack_pointer == 0x9000u &&
          machine->master_tick == tick && machine->cpu.main.a == 0u);
    CHECK(wz_machine_memory_read(machine, 0x8000u) == 0xcdu &&
          wz_machine_memory_read(machine, 0x8003u) == 0x3eu);
    CHECK(wz_headless_runner_init(&runner, machine, NULL) == WZ_RESULT_OK);
    CHECK(wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_host_machine_frame_execute(&runner, 256u, false, NULL, NULL) ==
          WZ_RESULT_BREAKPOINT_HIT);
    CHECK(machine->cpu.program_counter == 0x8003u &&
          machine->cpu.stack_pointer == 0x9000u && machine->cpu.main.a == 1u &&
          machine->master_tick == tick + 31u);
    CHECK(wz_machine_memory_read(machine, 0x8003u) == 0x3eu &&
          machine->cpu.main.a == 1u);
    CHECK(wz_debugger_window_pause_at_breakpoint(&window) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.paused && window.breakpoint_hit &&
          window.memory_address == 0x8003u &&
          window.snapshot.cpu.program_counter == 0x8003u);
    CHECK(machine->debugger_step_target_active == 0u &&
          wz_debugger_breakpoint_active(machine) &&
          wz_debugger_breakpoint_hit(machine) &&
          machine->debugger_breakpoint_address == 0x8003u);
    status = 0;

done:
    wz_debugger_window_close(&window);
    if (initialized) wz_machine_destroy(machine);
    free(machine);
    return status;
}

static int explicit_target_and_cancellation(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_headless_runner_t runner;
    wz_master_tick_t tick;
    bool initialized = false;
    int status = 1;

    wz_debugger_window_init(&window);
    if (machine == NULL) return 1;
    if (wz_machine_init(machine, wz_machine_profile_48k_pal()) !=
        WZ_RESULT_OK) goto done;
    initialized = true;
    machine->cpu.program_counter = 0x8000u;
    wz_machine_memory_write(machine, 0x8000u, 0x00u);
    wz_machine_memory_write(machine, 0x8001u, 0x00u);
    wz_machine_memory_write(machine, 0x8002u, 0x00u);
    wz_machine_memory_write(machine, 0x8003u, 0x3cu);
    wz_machine_memory_write(machine, 0x8004u, 0x00u);
    wz_machine_memory_write(machine, 0x8005u, 0x00u);

    CHECK(wz_debugger_window_open(&window, machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(command(&window, "J EXTRA") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(command(&window, "J 10000") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(command(&window, "J 8000 9000") ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    tick = machine->master_tick;
    CHECK(command(&window, "J 8003") == WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK(machine->debugger_step_target_active != 0u &&
          machine->debugger_step_target_address == 0x8003u &&
          machine->cpu.program_counter == 0x8000u &&
          machine->master_tick == tick);
    CHECK(wz_headless_runner_init(&runner, machine, NULL) == WZ_RESULT_OK);
    CHECK(wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_host_machine_frame_execute(&runner, 256u, false, NULL, NULL) ==
          WZ_RESULT_BREAKPOINT_HIT);
    CHECK(machine->cpu.program_counter == 0x8003u &&
          machine->master_tick == tick + 12u && machine->cpu.main.a == 0u);
    CHECK(wz_debugger_window_pause_at_breakpoint(&window) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.paused && window.memory_address == 0x8003u &&
          window.snapshot.cpu.program_counter == 0x8003u);

    tick = machine->master_tick;
    CHECK(command(&window, "J 8010") == WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK(wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(machine->debugger_step_target_active == 0u &&
          machine->cpu.program_counter == 0x8003u &&
          machine->master_tick == tick);

    CHECK(command(&window, "B 8005") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(command(&window, "J 8008") == WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK(wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_host_machine_frame_execute(&runner, 256u, false, NULL, NULL) ==
          WZ_RESULT_BREAKPOINT_HIT);
    CHECK(machine->cpu.program_counter == 0x8005u &&
          machine->cpu.main.a == 1u && machine->master_tick == tick + 8u);
    CHECK(wz_debugger_window_pause_at_breakpoint(&window) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(machine->debugger_step_target_active == 0u &&
          wz_debugger_breakpoint_active(machine) &&
          wz_debugger_breakpoint_hit(machine) &&
          machine->debugger_breakpoint_address == 0x8005u &&
          window.snapshot.cpu.program_counter == 0x8005u);
    status = 0;

done:
    wz_debugger_window_close(&window);
    if (initialized) wz_machine_destroy(machine);
    free(machine);
    return status;
}

static int validation_wrap_and_mapping(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_master_tick_t tick;
    bool initialized = false;
    int status = 1;

    wz_debugger_window_init(&window);
    if (machine == NULL) return 1;
    if (wz_machine_init(machine, wz_machine_profile_48k_pal()) !=
        WZ_RESULT_OK) goto done;
    initialized = true;
    machine->cpu.program_counter = 0xffffu;
    wz_machine_memory_write(machine, 0xffffu, 0x3eu);
    CHECK(wz_debugger_window_open(&window, machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    tick = machine->master_tick;
    CHECK(command(&window, "J") == WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK(machine->debugger_step_target_active != 0u &&
          machine->debugger_step_target_address == 0x0001u &&
          machine->debugger_step_target_mapping ==
              wz_machine_debugger_mapping_key(machine, 0x0001u));
    CHECK(machine->cpu.program_counter == 0xffffu && machine->master_tick == tick);
    CHECK(wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(command(&window, "J 8000") == WZ_DEBUGGER_WINDOW_NOT_PAUSED);
    CHECK(wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(machine->debugger_step_target_active == 0u &&
          machine->cpu.program_counter == 0xffffu && machine->master_tick == tick);
    wz_debugger_window_close(&window);
    CHECK(command(&window, "J") == WZ_DEBUGGER_WINDOW_NOT_OPEN);
    status = 0;

done:
    wz_debugger_window_close(&window);
    if (initialized) wz_machine_destroy(machine);
    free(machine);
    return status;
}

static int mapped_target_identity(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    bool initialized = false;
    int status = 1;

    wz_debugger_window_init(&window);
    if (machine == NULL) return 1;
    if (wz_machine_init(machine, wz_machine_profile_128k_pal()) !=
        WZ_RESULT_OK) goto done;
    initialized = true;
    machine->cpu.program_counter = 0x8000u;
    CHECK(wz_debugger_window_open(&window, machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(command(&window, "J C000") == WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK(machine->debugger_step_target_active != 0u &&
          machine->debugger_step_target_address == 0xc000u &&
          machine->debugger_step_target_mapping ==
              wz_machine_debugger_mapping_key(machine, 0xc000u));
    CHECK(wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(machine->debugger_step_target_active == 0u);
    status = 0;

done:
    wz_debugger_window_close(&window);
    if (initialized) wz_machine_destroy(machine);
    free(machine);
    return status;
}

int main(void)
{
    if (default_call_step_over() != 0 ||
        explicit_target_and_cancellation() != 0 ||
        validation_wrap_and_mapping() != 0 ||
        mapped_target_identity() != 0) {
        return 1;
    }
    printf("debugger-monitor-jump-contract assertions=%u status=pass\n",
           assertions);
    return 0;
}
