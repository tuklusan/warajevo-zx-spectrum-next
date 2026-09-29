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

#include "app/wz_host_audio_policy.h"
#include "app/wz_host_machine_frame.h"
#include "core/wz_state.h"

typedef struct {
    wz_machine_t* machine;
    wz_master_tick_t prior_tick;
    bool expect_audio_capture;
    wz_byte_t expected_beeper_level;
    unsigned calls;
    bool ordering_ok;
} output_probe_t;

static bool reject_output(void* context, wz_master_tick_t start_tick,
                          wz_byte_t initial_beeper_level,
                          const wz_ay_t* initial_ay)
{
    output_probe_t* probe = (output_probe_t*)context;
    if (probe == NULL || probe->machine == NULL) return false;
    probe->ordering_ok = probe->machine->master_tick > start_tick &&
        start_tick == probe->prior_tick &&
        ((initial_ay != NULL) == probe->expect_audio_capture) &&
        initial_beeper_level == probe->expected_beeper_level;
    probe->prior_tick = probe->machine->master_tick;
    ++probe->calls;
    return false;
}

int main(void)
{
    wz_machine_t machine;
    wz_machine_t reference;
    wz_headless_runner_t runner;
    wz_headless_runner_t reference_runner;
    output_probe_t probe;
    const wz_master_tick_t frame_ticks = 128u;
    const wz_dword_t frames_per_speed = 8u;
    wz_dword_t initial_noise_lfsr;
    bool success = false;

    memset(&machine, 0, sizeof(machine));
    memset(&reference, 0, sizeof(reference));
    memset(&runner, 0, sizeof(runner));
    memset(&reference_runner, 0, sizeof(reference_runner));
    memset(&probe, 0, sizeof(probe));
    if (wz_machine_init(&machine, wz_machine_profile_128k_pal()) !=
            WZ_RESULT_OK ||
        wz_machine_init(&reference, wz_machine_profile_128k_pal()) !=
            WZ_RESULT_OK ||
        wz_headless_runner_init(&runner, &machine, NULL) != WZ_RESULT_OK ||
        wz_headless_runner_init(&reference_runner, &reference, NULL) !=
            WZ_RESULT_OK) {
        goto cleanup;
    }
    probe.machine = &machine;
    probe.prior_tick = machine.master_tick;
    initial_noise_lfsr = machine.ay.noise_lfsr;

    for (wz_speed_policy_t speed = WZ_SPEED_25;
         speed < WZ_SPEED_COUNT; ++speed) {
        bool expected_audio = speed == WZ_SPEED_50 ||
            speed == WZ_SPEED_100 || speed == WZ_SPEED_200;
        if (wz_host_audio_enabled(speed) != expected_audio) {
            fprintf(stderr, "audio speed policy mismatch at %u\n",
                    (unsigned)speed);
            goto cleanup;
        }
        for (wz_dword_t frame = 0u; frame < frames_per_speed; ++frame) {
            wz_master_tick_t start_tick = machine.master_tick;
            wz_dword_t prior_noise_lfsr = machine.ay.noise_lfsr;
            wz_qword_t machine_hash;
            wz_qword_t reference_hash;
            wz_byte_t beeper = (frame & 1u) == 0u ? 0x08u : 0x00u;

            if (machine.master_tick != reference.master_tick) {
                fprintf(stderr, "machine timeline diverged before speed %u\n",
                        (unsigned)speed);
                goto cleanup;
            }
            wz_machine_ula_port_fe_write(&machine, 0u, beeper, start_tick);
            wz_machine_ula_port_fe_write(&reference, 0u, beeper,
                                         start_tick);
            probe.expect_audio_capture = expected_audio;
            probe.expected_beeper_level = (wz_byte_t)((beeper >> 3u) & 1u);
            probe.ordering_ok = false;
            if (wz_host_machine_frame_execute(
                    &runner, frame_ticks, expected_audio, reject_output,
                    &probe) != WZ_RESULT_OK ||
                wz_headless_runner_execute(&reference_runner, frame_ticks) !=
                    WZ_RESULT_OK ||
                machine.master_tick <= start_tick ||
                machine.master_tick != reference.master_tick ||
                machine.ay.noise_lfsr == prior_noise_lfsr ||
                !probe.ordering_ok ||
                probe.calls != (unsigned)speed * frames_per_speed + frame + 1u ||
                wz_state_hash_machine(&machine, &machine_hash) != WZ_RESULT_OK ||
                wz_state_hash_machine(&reference, &reference_hash) !=
                    WZ_RESULT_OK ||
                machine_hash != reference_hash) {
                fprintf(stderr,
                        "tick/audio independence failed at speed %u frame %u\n",
                        (unsigned)speed, (unsigned)frame);
                goto cleanup;
            }
        }
    }
    if (machine.master_tick <
            WZ_SPEED_COUNT * frames_per_speed * frame_ticks ||
        machine.ay.noise_lfsr == initial_noise_lfsr ||
        wz_host_machine_frame_execute(&runner, 0u, false, reject_output,
                                      &probe) != WZ_RESULT_INVALID_ARGUMENT) {
        fprintf(stderr, "speed sweep, AY progression, or validation failed\n");
        goto cleanup;
    }
    puts("PASS all speed modes preserve machine time through audio rejection");
    success = true;

cleanup:
    wz_machine_destroy(&machine);
    wz_machine_destroy(&reference);
    return success ? 0 : 1;
}
