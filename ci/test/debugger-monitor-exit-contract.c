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
static wz_debugger_window_result_t command(wz_debugger_window_t *w,
                                            const char *s)
{
    return wz_debugger_window_dispatch_monitor_command(w, s, strlen(s));
}

#define CHECK(x) do { \
    ++assertions; \
    if (!(x)) { fprintf(stderr, "exit contract %d: %s\n", __LINE__, #x); \
        goto done; } \
} while (0)

int main(void)
{
    wz_machine_t *m = calloc(1u, sizeof(*m));
    wz_debugger_window_t w;
    wz_headless_runner_t r;
    wz_z80_state_t cpu;
    wz_master_tick_t tick;
    bool initialized = false;
    int status = 1;

    wz_debugger_window_init(&w);
    if (m == NULL || wz_machine_init(m, wz_machine_profile_48k_pal()) !=
            WZ_RESULT_OK) { free(m); return 1; }
    initialized = true;
    m->cpu.program_counter = 0x8000u;
    wz_machine_memory_write(m, 0x8000u, 0x00u);
    wz_machine_memory_write(m, 0x8001u, 0x76u);
    CHECK(wz_debugger_window_open(&w, m, true) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(command(&w, "E EXTRA") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(command(&w, "R SETREG PC 8000") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(command(&w, "B 8001") == WZ_DEBUGGER_WINDOW_OK);
    CHECK(w.snapshot.cpu.program_counter == 0x8000u && w.breakpoint_active &&
          w.breakpoint_address == 0x8001u);
    cpu = m->cpu; tick = m->master_tick;
    CHECK(command(&w, " e ") == WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK(wz_debugger_window_is_open(&w) && wz_debugger_window_is_paused(&w) &&
          m->debugger_access_mode == WZ_DEBUGGER_PAUSED_MUTATION);
    CHECK(memcmp(&m->cpu, &cpu, sizeof(cpu)) == 0 && m->master_tick == tick);
    CHECK(wz_headless_runner_init(&r, m, NULL) == WZ_RESULT_OK);
    CHECK(wz_debugger_window_set_paused(&w, false) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(!w.paused && m->debugger_access_mode == WZ_DEBUGGER_READ_ONLY);
    CHECK(wz_host_machine_frame_execute(&r, 256u, false, NULL, NULL) ==
          WZ_RESULT_BREAKPOINT_HIT);
    CHECK(m->cpu.program_counter == 0x8001u && m->master_tick > tick &&
          wz_debugger_breakpoint_active(m) && wz_debugger_breakpoint_hit(m));
    CHECK(wz_machine_memory_read(m, 0x8000u) == 0x00u &&
          wz_machine_memory_read(m, 0x8001u) == 0x76u);
    CHECK(wz_debugger_window_is_open(&w) &&
          !wz_debugger_window_is_paused(&w));
    tick = m->master_tick;
    CHECK(wz_debugger_window_pause_at_breakpoint(&w) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(w.paused && w.snapshot.cpu.program_counter == 0x8001u &&
          w.breakpoint_hit && w.breakpoint_active);
    CHECK(m->master_tick == tick);
    CHECK(command(&w, "E EXTRA") == WZ_DEBUGGER_WINDOW_INVALID_ARGUMENT);
    CHECK(command(&w, "E") == WZ_DEBUGGER_WINDOW_RESUME_REQUESTED);
    CHECK(m->cpu.program_counter == 0x8001u && m->debugger_breakpoint_hit);
    CHECK(wz_debugger_window_set_paused(&w, false) == WZ_DEBUGGER_WINDOW_OK);
    CHECK(command(&w, "E") == WZ_DEBUGGER_WINDOW_NOT_PAUSED);
    wz_debugger_window_close(&w);
    CHECK(command(&w, "E") == WZ_DEBUGGER_WINDOW_NOT_OPEN);
    status = 0;
done:
    wz_debugger_window_close(&w);
    if (initialized) wz_machine_destroy(m);
    free(m);
    if (status == 0)
        printf("debugger-monitor-exit-contract assertions=%u status=pass\n",
               assertions);
    return status;
}
