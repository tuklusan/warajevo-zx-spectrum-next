/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app/wz_host_pacing.h"
#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"
#include "core/wz_runner.h"

typedef struct {
    wz_master_tick_t mic_ticks[4];
    wz_byte_t mic_levels[4];
    size_t mic_count;
    wz_qword_t first_frame_wait;
    wz_qword_t changed_speed_wait;
    size_t tape_segment;
    wz_master_tick_t tape_elapsed;
    wz_byte_t ear_level;
    wz_master_tick_t final_tick;
} cassette_result_t;

static wz_qword_t expected_wait(wz_master_tick_t ticks,
                                wz_qword_t ticks_per_second,
                                unsigned percent)
{
    wz_qword_t elapsed = ticks * UINT64_C(1000000000) / ticks_per_second;
    return elapsed * 100u / percent;
}

static bool run_speed_transition(wz_speed_policy_t initial_speed,
                                 wz_speed_policy_t changed_speed,
                                 cassette_result_t* output)
{
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    wz_tape_segment_t segments[16];
    wz_machine_t machine;
    wz_headless_runner_t runner;
    wz_host_pacing_t pacing;
    wz_master_tick_t frame_ticks;
    wz_master_tick_t first_boundary;
    wz_master_tick_t second_boundary;
    wz_qword_t host_elapsed;
    wz_qword_t requested_wait;
    wz_mic_event_t events[4];
    size_t event_count;
    bool success = false;

    memset(&machine, 0, sizeof(machine));
    memset(output, 0, sizeof(*output));
    if (profile == NULL || profile->master_hz_den == 0u ||
        profile->master_hz_num % profile->master_hz_den != 0u ||
        wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        wz_machine_destroy(&machine);
        return false;
    }
    for (size_t index = 0u; index < sizeof(segments) / sizeof(segments[0]); ++index) {
        segments[index].duration = UINT64_C(50000) +
            (wz_master_tick_t)(index % 3u) * UINT64_C(10000);
        segments[index].ear_level = (wz_byte_t)(index & 1u);
        segments[index].motor_stop_after = 0u;
    }
    if (wz_machine_mount_tape(&machine, segments,
            sizeof(segments) / sizeof(segments[0])) != WZ_RESULT_OK ||
        wz_machine_set_tape_motor(&machine, true) != WZ_RESULT_OK ||
        wz_headless_runner_init(&runner, &machine, NULL) != WZ_RESULT_OK ||
        !wz_host_pacing_init(&pacing,
            profile->master_hz_num / profile->master_hz_den,
            initial_speed, 0u, 0u) ||
        wz_machine_mic_capture_begin(&machine) != WZ_RESULT_OK) {
        goto cleanup;
    }

    /* A real Z80 OUT sequence generates the MIC transitions. */
    wz_machine_memory_write(&machine, 0x8000u, 0x3eu); /* LD A,10h */
    wz_machine_memory_write(&machine, 0x8001u, 0x10u);
    wz_machine_memory_write(&machine, 0x8002u, 0xd3u); /* OUT (FEh),A */
    wz_machine_memory_write(&machine, 0x8003u, 0xfeu);
    wz_machine_memory_write(&machine, 0x8004u, 0xafu); /* XOR A */
    wz_machine_memory_write(&machine, 0x8005u, 0xd3u);
    wz_machine_memory_write(&machine, 0x8006u, 0xfeu);
    machine.cpu.program_counter = 0x8000u;
    if (wz_headless_runner_execute(&runner, 66u) != WZ_RESULT_OK) {
        goto cleanup;
    }

    frame_ticks = (wz_master_tick_t)profile->tstates_per_frame *
        profile->master_ticks_per_cpu_tstate;
    first_boundary = frame_ticks + 2u;
    if (wz_headless_runner_execute(&runner,
            first_boundary - machine.master_tick) != WZ_RESULT_OK) {
        goto cleanup;
    }
    if (!wz_host_pacing_wait(&pacing, 0u, machine.master_tick, NULL, NULL,
                             &requested_wait) ||
        requested_wait != expected_wait(machine.master_tick,
            profile->master_hz_num / profile->master_hz_den,
            wz_speed_policy_percent(initial_speed))) {
        goto cleanup;
    }
    output->first_frame_wait = requested_wait;
    host_elapsed = requested_wait;
    if (!wz_host_pacing_wait(&pacing, host_elapsed, machine.master_tick,
                             NULL, NULL, &requested_wait) ||
        requested_wait != 0u ||
        !wz_host_pacing_set_speed(&pacing, changed_speed) ||
        !wz_host_pacing_wait(&pacing, host_elapsed, machine.master_tick,
                             NULL, NULL, &requested_wait) ||
        requested_wait != 0u) {
        goto cleanup;
    }

    second_boundary = first_boundary + frame_ticks + 2u;
    machine.cpu.program_counter = 0x8000u;
    if (wz_headless_runner_execute(&runner, 66u) != WZ_RESULT_OK ||
        wz_headless_runner_execute(&runner,
            second_boundary - machine.master_tick) != WZ_RESULT_OK ||
        !wz_host_pacing_wait(&pacing, host_elapsed, machine.master_tick,
                             NULL, NULL, &requested_wait) ||
        requested_wait != expected_wait(frame_ticks + 2u,
            profile->master_hz_num / profile->master_hz_den,
            wz_speed_policy_percent(changed_speed))) {
        goto cleanup;
    }
    output->changed_speed_wait = requested_wait;
    output->final_tick = machine.master_tick;
    output->tape_segment = machine.tape_state.segment_index;
    output->tape_elapsed = machine.tape_state.segment_elapsed;
    output->ear_level = wz_machine_tape_ear_level(&machine);
    event_count = wz_machine_mic_events(&machine, events, 4u);
    if (event_count != 4u) {
        goto cleanup;
    }
    output->mic_count = event_count;
    for (size_t index = 0u; index < 4u; ++index) {
        output->mic_ticks[index] = events[index].master_tick;
        output->mic_levels[index] = events[index].level;
    }
    if (output->mic_ticks[0] != 28u || output->mic_ticks[1] != 58u ||
        output->mic_ticks[2] != first_boundary + 28u ||
        output->mic_ticks[3] != first_boundary + 58u) {
        goto cleanup;
    }
    success = true;

cleanup:
    wz_machine_destroy(&machine);
    return success;
}

static bool same_emulated_waveform(const cassette_result_t* left,
                                   const cassette_result_t* right)
{
    return left->mic_count == right->mic_count &&
        memcmp(left->mic_ticks, right->mic_ticks, sizeof(left->mic_ticks)) == 0 &&
        memcmp(left->mic_levels, right->mic_levels, sizeof(left->mic_levels)) == 0 &&
        left->tape_segment == right->tape_segment &&
        left->tape_elapsed == right->tape_elapsed &&
        left->ear_level == right->ear_level &&
        left->final_tick == right->final_tick;
}

int main(void)
{
    cassette_result_t half_to_double;
    cassette_result_t double_to_half;
    cassette_result_t normal_to_quad;

    if (!run_speed_transition(WZ_SPEED_50, WZ_SPEED_200, &half_to_double) ||
        !run_speed_transition(WZ_SPEED_200, WZ_SPEED_50, &double_to_half) ||
        !run_speed_transition(WZ_SPEED_100, WZ_SPEED_400, &normal_to_quad) ||
        !same_emulated_waveform(&half_to_double, &double_to_half) ||
        !same_emulated_waveform(&half_to_double, &normal_to_quad) ||
        half_to_double.first_frame_wait !=
            2u * normal_to_quad.first_frame_wait ||
        double_to_half.changed_speed_wait !=
            4u * half_to_double.changed_speed_wait ||
        half_to_double.changed_speed_wait !=
            2u * normal_to_quad.changed_speed_wait) {
        fputs("cassette playback or MIC timing changed with runtime speed\n",
              stderr);
        return 1;
    }
    puts("PASS cassette master-time and speed-scaled host pacing");
    return 0;
}
