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

static unsigned assertions;

#define CHECK(expression) do { \
    ++assertions; \
    if (!(expression)) { \
        fprintf(stderr, "debugger-monitor-breakpoint-contract failed at %s:%d: %s\n", \
                __FILE__, __LINE__, #expression); \
        goto cleanup; \
    } \
} while (0)

int main(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_debugger_page_info_t page_info;
    wz_master_tick_t stop_tick;
    bool machine_initialized = false;
    int status = 1;

    wz_debugger_window_init(&window);
    if (machine == NULL ||
        wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return 1;
    }
    machine_initialized = true;
    machine->cpu.program_counter = 0x8000u;
    wz_machine_memory_write(machine, 0x8000u, 0x00u);
    wz_machine_memory_write(machine, 0x8001u, 0x76u);
    CHECK(wz_debugger_window_open(&window, machine, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_set_breakpoint(&window, 0x8000u) ==
          WZ_DEBUGGER_WINDOW_NOT_PAUSED);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "B 8000", 6u) ==
          WZ_DEBUGGER_WINDOW_NOT_PAUSED);
    CHECK(!window.breakpoint_active && !wz_debugger_breakpoint_active(machine));

    CHECK(wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "B", 1u) ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "B 10000", 7u) ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "B 8000 EXTRA", 12u) ==
          WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, " B 8000 ", 8u) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.breakpoint_active && !window.breakpoint_hit &&
          window.breakpoint_address == 0x8000u &&
          wz_debugger_breakpoint_active(machine));
    CHECK(wz_machine_memory_read(machine, 0x8000u) == 0x00u);

    stop_tick = machine->master_tick;
    CHECK(wz_debugger_window_continue(&window, 8u) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.executed == 0u && window.paused && window.breakpoint_active &&
          window.breakpoint_hit && window.snapshot.cpu.program_counter == 0x8000u);
    CHECK(machine->master_tick == stop_tick &&
          wz_debugger_breakpoint_hit(machine));
    CHECK(wz_debugger_window_step(&window) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.executed == 0u && window.breakpoint_hit &&
          machine->cpu.program_counter == 0x8000u);

    CHECK(wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_clear_breakpoint(&window) ==
          WZ_DEBUGGER_WINDOW_NOT_PAUSED);
    CHECK(wz_debugger_window_set_paused(&window, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "B CLEAR", 7u) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(!window.breakpoint_active && !window.breakpoint_hit &&
          !wz_debugger_breakpoint_active(machine) &&
          !wz_debugger_breakpoint_hit(machine));
    CHECK(wz_debugger_window_continue(&window, 2u) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.executed == 2u && window.paused && !window.breakpoint_hit &&
          machine->cpu.program_counter == 0x8001u && machine->cpu.halted != 0u);
    CHECK(wz_machine_memory_read(machine, 0x8000u) == 0x00u &&
          wz_machine_memory_read(machine, 0x8001u) == 0x76u);
    CHECK(wz_debugger_window_clear_breakpoint(&window) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(machine, 0x8000u) == 0x00u);

    wz_debugger_window_close(&window);
    CHECK(wz_debugger_window_set_breakpoint(&window, 0x8000u) ==
          WZ_DEBUGGER_WINDOW_NOT_OPEN);

    wz_machine_destroy(machine);
    free(machine);
    machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    machine_initialized = false;
    CHECK(machine != NULL &&
          wz_machine_init(machine, wz_machine_profile_128k_pal()) == WZ_RESULT_OK);
    machine_initialized = true;
    CHECK(wz_machine_debugger_mapping_key(machine, 0x0000u) == 0x10u);
    CHECK(wz_machine_debugger_mapping_key(machine, 0x4000u) == 0x25u);
    CHECK(wz_machine_debugger_mapping_key(machine, 0x8000u) == 0x22u);
    CHECK(wz_machine_debugger_mapping_key(machine, 0xc000u) == 0x20u);
    machine->cpu.program_counter = 0xc000u;
    wz_machine_memory_write(machine, 0xc000u, 0x00u);
    wz_debugger_window_init(&window);
    CHECK(wz_debugger_window_open(&window, machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_debugger_set_page_config(machine, 0x06u) == WZ_RESULT_OK);
    CHECK(wz_debugger_read_page_info(machine, &page_info) == WZ_RESULT_OK &&
          page_info.paging_value == 0x06u);
    CHECK(wz_machine_debugger_mapping_key(machine, 0xc000u) == 0x26u);
    wz_machine_memory_write(machine, 0xc000u, 0x00u);
    stop_tick = machine->master_tick;
    CHECK(wz_debugger_window_dispatch_monitor_command(&window, "B C000", 6u) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(wz_machine_memory_read(machine, 0xc000u) == 0x00u);
    CHECK(wz_debugger_set_page_config(machine, 0x0fu) == WZ_RESULT_OK);
    wz_machine_memory_write(machine, 0xc000u, 0x00u);
    CHECK(wz_debugger_read_page_info(machine, &page_info) == WZ_RESULT_OK &&
          page_info.paging_value == 0x0fu && page_info.screen_bank == 7u);
    CHECK(wz_machine_debugger_mapping_key(machine, 0xc000u) == 0x27u);
    CHECK(wz_debugger_window_continue(&window, 1u) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.executed == 1u && !window.breakpoint_hit &&
          machine->cpu.program_counter == 0xc001u);
    CHECK(wz_debugger_jump(machine, 0xc000u) == WZ_RESULT_OK);
    CHECK(wz_debugger_set_page_config(machine, 0x06u) == WZ_RESULT_OK);
    stop_tick = machine->master_tick;
    CHECK(wz_debugger_window_continue(&window, 1u) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(window.breakpoint_hit && window.breakpoint_active &&
          window.breakpoint_address == 0xc000u && window.executed == 0u &&
          machine->cpu.program_counter == 0xc000u &&
          machine->master_tick == stop_tick && window.paging.paging_value == 0x06u);
    CHECK(wz_machine_memory_read(machine, 0xc000u) == 0x00u);
    CHECK(wz_debugger_window_clear_breakpoint(&window) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(!window.breakpoint_active && !wz_debugger_breakpoint_active(machine));
    CHECK(wz_debugger_set_page_config(machine, 0x0fu) == WZ_RESULT_OK &&
          wz_machine_memory_read(machine, 0xc000u) == 0x00u);
    status = 0;

cleanup:
    wz_debugger_window_close(&window);
    if (machine_initialized) wz_machine_destroy(machine);
    free(machine);
    if (status == 0) {
        printf("debugger-monitor-breakpoint-contract assertions=%u status=pass\n",
               assertions);
    }
    return status;
}
