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
#include "app/wz_host_audio_push.h"
#include "app/wz_host_machine_frame.h"
#include "app/wz_host_pacing.h"
#include "app/wz_sokol_audio.h"
#include "core/wz_state.h"
#include "sokol_audio.h"

static bool test_audio_backend_valid;
static unsigned test_audio_backend_rejected_frames;

void saudio_setup(const saudio_desc* description)
{
    test_audio_backend_valid = description != NULL &&
        description->num_channels == 1;
}

void saudio_shutdown(void)
{
    test_audio_backend_valid = false;
}

bool saudio_isvalid(void)
{
    return test_audio_backend_valid;
}

int saudio_push(const float* frames, int num_frames)
{
    (void)frames;
    if (num_frames > 0) {
        test_audio_backend_rejected_frames += (unsigned)num_frames;
    }
    return 0;
}

typedef struct {
    wz_machine_t* machine;
    wz_sokol_audio_t* audio;
    wz_master_tick_t prior_tick;
    bool expect_audio_capture;
    wz_byte_t expected_beeper_level;
    wz_speed_policy_t speed;
    size_t expected_pending_samples;
    unsigned calls;
    bool ordering_ok;
    bool delivery_ok;
} output_probe_t;

typedef struct {
    unsigned calls;
    wz_qword_t last_nanoseconds;
} pacing_probe_t;

static bool record_sleep(wz_qword_t nanoseconds, void* context)
{
    pacing_probe_t* probe = (pacing_probe_t*)context;
    if (probe == NULL) return false;
    ++probe->calls;
    probe->last_nanoseconds = nanoseconds;
    return true;
}

static bool reject_output(void* context, wz_master_tick_t start_tick,
                          wz_byte_t initial_beeper_level,
                          const wz_ay_t* initial_ay)
{
    output_probe_t* probe = (output_probe_t*)context;
    wz_audio_sample_t sample = 1000;
    size_t submitted;
    if (probe == NULL || probe->machine == NULL || probe->audio == NULL) {
        return false;
    }
    probe->ordering_ok = probe->machine->master_tick > start_tick &&
        start_tick == probe->prior_tick &&
        ((initial_ay != NULL) == probe->expect_audio_capture) &&
        initial_beeper_level == probe->expected_beeper_level;
    if (probe->expect_audio_capture) {
        submitted = wz_sokol_audio_push(probe->audio, probe->speed, &sample, 1u);
    } else {
        wz_sokol_audio_discard_pending(probe->audio);
        submitted = 0u;
    }
    probe->delivery_ok = submitted == 0u &&
        wz_host_audio_queued(&probe->audio->pending) ==
            probe->expected_pending_samples;
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
    wz_sokol_audio_t audio;
    wz_host_pacing_t pacing;
    output_probe_t probe;
    pacing_probe_t sleep_probe;
    const wz_master_tick_t frame_ticks = 128u;
    const wz_dword_t frames_per_speed = 8u;
    const wz_qword_t fake_ticks_per_second = UINT64_C(1000000);
    wz_qword_t fake_host_nanoseconds = 0u;
    size_t expected_pending_samples = 0u;
    wz_dword_t initial_noise_lfsr;
    bool success = false;

    memset(&machine, 0, sizeof(machine));
    memset(&reference, 0, sizeof(reference));
    memset(&runner, 0, sizeof(runner));
    memset(&reference_runner, 0, sizeof(reference_runner));
    memset(&audio, 0, sizeof(audio));
    memset(&pacing, 0, sizeof(pacing));
    memset(&probe, 0, sizeof(probe));
    memset(&sleep_probe, 0, sizeof(sleep_probe));
    if (wz_machine_init(&machine, wz_machine_profile_128k_pal()) !=
            WZ_RESULT_OK ||
        wz_machine_init(&reference, wz_machine_profile_128k_pal()) !=
            WZ_RESULT_OK ||
        wz_headless_runner_init(&runner, &machine, NULL) != WZ_RESULT_OK ||
        wz_headless_runner_init(&reference_runner, &reference, NULL) !=
            WZ_RESULT_OK || !wz_sokol_audio_init(&audio) ||
        !wz_host_pacing_init(&pacing, fake_ticks_per_second, WZ_SPEED_25,
                             fake_host_nanoseconds, machine.master_tick)) {
        goto cleanup;
    }
    probe.machine = &machine;
    probe.audio = &audio;
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
        if (speed != WZ_SPEED_25 &&
            !wz_host_pacing_set_speed(&pacing, speed)) {
            fprintf(stderr, "pacing speed update failed at %u\n",
                    (unsigned)speed);
            goto cleanup;
        }
        if (speed != WZ_SPEED_25) {
            wz_qword_t reanchor_sleep = UINT64_MAX;
            if (!wz_host_pacing_wait(&pacing, fake_host_nanoseconds,
                                     machine.master_tick, NULL, NULL,
                                     &reanchor_sleep) || reanchor_sleep != 0u) {
                fprintf(stderr, "pacing failed to re-anchor at speed %u\n",
                        (unsigned)speed);
                goto cleanup;
            }
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
            probe.speed = speed;
            if (expected_audio) {
                ++expected_pending_samples;
            } else {
                expected_pending_samples = 0u;
            }
            probe.expected_pending_samples = expected_pending_samples;
            probe.ordering_ok = false;
            probe.delivery_ok = false;
            if (wz_host_machine_frame_execute(
                    &runner, frame_ticks, expected_audio, reject_output,
                    &probe) != WZ_RESULT_OK ||
                wz_headless_runner_execute(&reference_runner, frame_ticks) !=
                    WZ_RESULT_OK ||
                machine.master_tick <= start_tick ||
                machine.master_tick != reference.master_tick ||
                machine.ay.noise_lfsr == prior_noise_lfsr ||
                !probe.ordering_ok || !probe.delivery_ok ||
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
            {
                wz_qword_t requested_sleep = UINT64_MAX;
                wz_qword_t expected_sleep = 0u;
                if (!wz_speed_policy_is_unlimited(speed)) {
                    wz_qword_t elapsed_ticks = machine.master_tick -
                        pacing.anchor_machine_tick;
                    wz_qword_t target_nanoseconds = elapsed_ticks *
                        UINT64_C(1000000000) / fake_ticks_per_second;
                    wz_qword_t host_elapsed = fake_host_nanoseconds -
                        pacing.anchor_host_nanoseconds;
                    target_nanoseconds = target_nanoseconds * 100u /
                        wz_speed_policy_percent(speed);
                    expected_sleep = target_nanoseconds > host_elapsed
                        ? target_nanoseconds - host_elapsed
                        : 0u;
                }
                unsigned prior_sleep_calls = sleep_probe.calls;
                if (!wz_host_pacing_wait(&pacing, fake_host_nanoseconds,
                                         machine.master_tick, record_sleep,
                                         &sleep_probe, &requested_sleep) ||
                    requested_sleep != expected_sleep ||
                    sleep_probe.calls != prior_sleep_calls +
                        (expected_sleep == 0u ? 0u : 1u) ||
                    (expected_sleep != 0u &&
                     sleep_probe.last_nanoseconds != expected_sleep)) {
                    fprintf(stderr,
                            "audio backpressure changed pacing at speed %u frame %u\n",
                            (unsigned)speed, (unsigned)frame);
                    goto cleanup;
                }
                fake_host_nanoseconds += requested_sleep;
            }
        }
    }
    if (machine.master_tick <
            WZ_SPEED_COUNT * frames_per_speed * frame_ticks ||
        machine.ay.noise_lfsr == initial_noise_lfsr ||
        test_audio_backend_rejected_frames == 0u ||
        wz_host_audio_queued(&audio.pending) != 0u ||
        wz_sokol_audio_degraded(&audio) ||
        wz_host_machine_frame_execute(&runner, 0u, false, reject_output,
                                      &probe) != WZ_RESULT_INVALID_ARGUMENT) {
        fprintf(stderr, "speed sweep, AY progression, or validation failed\n");
        goto cleanup;
    }
    puts("PASS all speeds preserve emulation and pacing under audio backpressure");
    success = true;

cleanup:
    wz_sokol_audio_shutdown(&audio);
    wz_machine_destroy(&machine);
    wz_machine_destroy(&reference);
    return success ? 0 : 1;
}
