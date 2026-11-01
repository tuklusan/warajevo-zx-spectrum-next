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
#define CHECK(x) do { ++assertions; if (!(x)) { \
    fprintf(stderr, "monitor F contract %d: %s\n", __LINE__, #x); \
    goto done; } } while (0)
#define RUN(text) wz_debugger_window_dispatch_monitor_command( \
    &window, (text), sizeof(text) - 1u)

int main(void)
{
    wz_machine_t *machine = calloc(1u, sizeof(*machine));
    wz_debugger_window_t window;
    wz_z80_state_t cpu;
    wz_master_tick_t tick;
    wz_byte_t before[16];
    wz_byte_t after[16];
    bool initialized = false;
    int status = 1;
    wz_debugger_window_init(&window);
    if (machine == NULL || wz_machine_init(machine,
            wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return 1;
    }
    initialized = true;
    wz_machine_memory_write(machine, 0x8000u, 0x00u);
    wz_machine_memory_write(machine, 0x8001u, 0x34u);
    wz_machine_memory_write(machine, 0x8002u, 0x12u);
    wz_machine_memory_write(machine, 0x8003u, 0x00u);
    wz_machine_memory_write(machine, 0x8004u, 0xaau);
    wz_machine_memory_write(machine, 0x8005u, 0x12u);
    wz_machine_memory_write(machine, 0x8006u, 0x34u);
    wz_machine_memory_write(machine, 0x8007u, 0xfeu);
    wz_machine_memory_write(machine, 0xffffu, 0xaau);
    for (size_t index = 0u; index < sizeof(before); ++index)
        before[index] = wz_machine_memory_read(machine,
            (wz_word_t)(0x8000u + index));
    cpu = machine->cpu;
    tick = machine->master_tick;
    CHECK(wz_debugger_window_open(&window, machine, true) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(RUN("F 8000 800F 1234") == WZ_DEBUGGER_WINDOW_OK &&
          window.memory_address == 0x8001u &&
          wz_debugger_window_memory(&window)[0] == 0x34u);
    CHECK(RUN("F 8000 800F 12 34") == WZ_DEBUGGER_WINDOW_OK &&
          window.memory_address == 0x8005u);
    CHECK(RUN("F 8000 800F -1 34 12") == WZ_DEBUGGER_WINDOW_OK &&
          window.memory_address == 0x8000u);
    CHECK(RUN("F 8000 800F FE AD") == WZ_DEBUGGER_WINDOW_NOT_FOUND);
    CHECK(window.memory_address == 0x8000u);
    CHECK(RUN("F FFFF FFFF 1234") == WZ_DEBUGGER_WINDOW_NOT_FOUND);
    CHECK(RUN("F FFFF FFFF AA") == WZ_DEBUGGER_WINDOW_OK &&
          window.memory_address == 0xffffu);
    CHECK(RUN("F FFFF 0000 AA") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(RUN("F 8000 800F") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(RUN("F 8000 800F GG") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(wz_debugger_window_set_paused(&window, false) ==
          WZ_DEBUGGER_WINDOW_OK);
    CHECK(RUN("F 8000 800F 12") == WZ_DEBUGGER_WINDOW_OK &&
          window.memory_address == 0x8002u);
    CHECK(machine->master_tick == tick &&
          memcmp(&machine->cpu, &cpu, sizeof(cpu)) == 0 &&
          memcmp(&window.snapshot.cpu, &cpu, sizeof(cpu)) == 0 &&
          wz_debugger_read_memory_block(machine, 0x8000u, after,
              sizeof(after)) == WZ_RESULT_OK &&
          memcmp(before, after, sizeof(before)) == 0 &&
          wz_machine_memory_read(machine, 0xffffu) == 0xaau);
    wz_debugger_window_close(&window);
    CHECK(RUN("F 8000 800F 12") == WZ_DEBUGGER_WINDOW_NOT_OPEN);
    status = 0;
done:
    wz_debugger_window_close(&window);
    if (initialized) wz_machine_destroy(machine);
    free(machine);
    if (status == 0)
        printf("debugger-monitor-find-contract assertions=%u status=pass\n",
               assertions);
    return status;
}
