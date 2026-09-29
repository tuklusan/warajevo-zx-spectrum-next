/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app/wz_host_machine_frame.h"

typedef struct {
    wz_machine_t* machine;
    wz_master_tick_t prior_tick;
    bool expect_audio_capture;
    unsigned calls;
    bool ordering_ok;
} output_probe_t;

static bool reject_output(void* context, wz_master_tick_t start_tick,
                          wz_byte_t initial_beeper_level,
                          const wz_ay_t* initial_ay)
{
    output_probe_t* probe = (output_probe_t*)context;
    (void)initial_beeper_level;
    if (probe == NULL || probe->machine == NULL) return false;
    probe->ordering_ok = probe->machine->master_tick > start_tick &&
        start_tick == probe->prior_tick &&
        ((initial_ay != NULL) == probe->expect_audio_capture);
    probe->prior_tick = probe->machine->master_tick;
    ++probe->calls;
    return false;
}

int main(void)
{
    wz_machine_t machine;
    wz_headless_runner_t runner;
    output_probe_t probe;
    const wz_master_tick_t frame_ticks = 128u;
    bool success = false;

    memset(&machine, 0, sizeof(machine));
    memset(&runner, 0, sizeof(runner));
    memset(&probe, 0, sizeof(probe));
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) !=
            WZ_RESULT_OK ||
        wz_headless_runner_init(&runner, &machine, NULL) != WZ_RESULT_OK) {
        goto cleanup;
    }
    probe.machine = &machine;
    probe.prior_tick = machine.master_tick;

    for (unsigned frame = 0u; frame < 8u; ++frame) {
        wz_master_tick_t start_tick = machine.master_tick;
        probe.expect_audio_capture = (frame & 1u) == 0u;
        probe.ordering_ok = false;
        if (wz_host_machine_frame_execute(
                &runner, frame_ticks, probe.expect_audio_capture,
                reject_output, &probe) != WZ_RESULT_OK ||
            machine.master_tick <= start_tick || !probe.ordering_ok ||
            probe.calls != frame + 1u) {
            fprintf(stderr, "machine frame stopped or reordered at frame %u\n",
                    frame);
            goto cleanup;
        }
    }
    if (machine.master_tick < 8u * frame_ticks ||
        wz_host_machine_frame_execute(&runner, 0u, false, reject_output,
                                      &probe) != WZ_RESULT_INVALID_ARGUMENT) {
        fprintf(stderr, "frame tick continuity or validation failed\n");
        goto cleanup;
    }
    puts("PASS machine ticks advance across muted audio and rejected output");
    success = true;

cleanup:
    wz_machine_destroy(&machine);
    return success ? 0 : 1;
}
