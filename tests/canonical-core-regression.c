/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/audio/wz_audio_mixer.h"
#include "core/wz_machine.h"
#include "core/wz_raster.h"
#include "core/wz_state.h"
#include "canonical-fingerprints.h"

#define CPU_STEPS 250005u
#define AUDIO_SAMPLES 2000000u
#define TAPE_SEGMENTS 131072u
#define TAPE_TICKS 4000000u

typedef struct {
    const char* name;
    uint64_t value;
} fingerprint_t;

static uint64_t hash_bytes(const void* data, size_t length)
{
    const unsigned char* bytes = (const unsigned char*)data;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t index = 0u; index < length; ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static bool check_fingerprint(const fingerprint_t* actual,
                              const fingerprint_t* expected)
{
    if (actual->value != expected->value) {
        fprintf(stderr, "%s fingerprint mismatch: expected %016" PRIx64
                        ", got %016" PRIx64 "\n",
                actual->name, expected->value, actual->value);
        return false;
    }
    printf("PASS %s %016" PRIx64 "\n", actual->name, actual->value);
    return true;
}

static bool fingerprint_cpu(fingerprint_t* output)
{
    static const wz_byte_t program[] = {
        0x3eu, 0x01u, 0x23u, 0xafu, 0x85u,
        0x32u, 0x00u, 0xc0u, 0x34u, 0xc3u, 0x00u, 0x80u
    };
    wz_machine_t machine;

    memset(&machine, 0, sizeof(machine));
    wz_result_t result = wz_machine_init(&machine, wz_machine_profile_48k_pal());
    if (result != WZ_RESULT_OK) {
        fprintf(stderr, "cpu_execution machine init failed: %d\n", (int)result);
        return false;
    }
    for (size_t index = 0u; index < sizeof(program); ++index) {
        wz_machine_memory_write(&machine, (wz_word_t)(0x8000u + index),
                                program[index]);
    }
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.main.h = 0xc0u;
    for (size_t index = 0u; index < CPU_STEPS; ++index) {
        result = wz_z80_step(&machine);
        if (result != WZ_RESULT_OK) {
            fprintf(stderr, "cpu_execution step %zu failed: %d at PC %04x\n",
                    index, (int)result, machine.cpu.program_counter);
            wz_machine_destroy(&machine);
            return false;
        }
    }
    output->name = "cpu_execution";
    if (machine.cpu.program_counter != 0x8000u) {
        fprintf(stderr, "cpu_execution ended at unexpected PC %04x\n",
                machine.cpu.program_counter);
        wz_machine_destroy(&machine);
        return false;
    }
    result = wz_state_hash_machine(&machine, &output->value);
    if (result != WZ_RESULT_OK) {
        fprintf(stderr, "cpu_execution state hash failed: %d\n", (int)result);
        wz_machine_destroy(&machine);
        return false;
    }
    wz_machine_destroy(&machine);
    return true;
}

static bool fingerprint_raster(fingerprint_t* output)
{
    const size_t raster_size =
        (size_t)WZ_RASTER_CANONICAL_WIDTH * WZ_RASTER_CANONICAL_HEIGHT;
    wz_machine_t machine;
    wz_raster_buffer_t raster;
    wz_byte_t* pixels = (wz_byte_t*)malloc(raster_size);
    wz_qword_t before_hash;
    wz_qword_t after_hash;
    bool success = false;

    memset(&machine, 0, sizeof(machine));
    memset(&raster, 0, sizeof(raster));
    if (pixels == NULL ||
        wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(pixels);
        return false;
    }
    for (size_t address = 0x4000u; address < 0x5b00u; ++address) {
        wz_byte_t value = (wz_byte_t)((address * 37u +
                                       (address >> 7u) * 11u) & 0xffu);
        wz_machine_memory_write(&machine, (wz_word_t)address, value);
    }
    machine.border_color = 5u;
    machine.master_tick =
        (wz_master_tick_t)machine.profile->tstates_per_frame *
        machine.profile->master_ticks_per_cpu_tstate * 16u;
    if (wz_raster_buffer_init(&raster, WZ_RASTER_CANONICAL_WIDTH,
                              WZ_RASTER_CANONICAL_HEIGHT, pixels,
                              raster_size) != WZ_RESULT_OK ||
        wz_state_hash_machine(&machine, &before_hash) != WZ_RESULT_OK ||
        wz_machine_render_raster(&machine, &raster) != WZ_RESULT_OK ||
        wz_state_hash_machine(&machine, &after_hash) != WZ_RESULT_OK ||
        before_hash != after_hash) {
        goto cleanup;
    }
    output->name = "raster_generation";
    output->value = hash_bytes(pixels, raster_size);
    success = true;

cleanup:
    wz_machine_destroy(&machine);
    free(pixels);
    return success;
}

static bool verify_border_event_timing(void)
{
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    wz_master_tick_t line_ticks;
    wz_master_tick_t frame_ticks;
    wz_master_tick_t active_line;
    wz_border_event_t expected_events[5];
    const size_t raster_size =
        (size_t)WZ_RASTER_CANONICAL_WIDTH * WZ_RASTER_CANONICAL_HEIGHT;
    wz_machine_t machine;
    wz_raster_buffer_t raster;
    wz_border_event_t recorded[sizeof(expected_events) / sizeof(expected_events[0])];
    wz_byte_t* pixels = (wz_byte_t*)malloc(raster_size);
    bool success = false;

    memset(&machine, 0, sizeof(machine));
    memset(&raster, 0, sizeof(raster));
    if (profile == 0 || pixels == 0) {
        free(pixels);
        return false;
    }
    line_ticks = (wz_master_tick_t)profile->tstates_per_line *
        profile->master_ticks_per_cpu_tstate;
    frame_ticks = (wz_master_tick_t)profile->tstates_per_frame *
        profile->master_ticks_per_cpu_tstate;
    active_line = line_ticks * 64u;
    expected_events[0] = (wz_border_event_t){0u, 2u};
    expected_events[1] = (wz_border_event_t){1u, 3u};
    expected_events[2] = (wz_border_event_t){active_line + 256u, 6u};
    expected_events[3] = (wz_border_event_t){active_line + 280u, 1u};
    expected_events[4] = (wz_border_event_t){active_line + 352u, 4u};
    if (line_ticks != WZ_RASTER_CANONICAL_WIDTH ||
        frame_ticks != (wz_master_tick_t)WZ_RASTER_CANONICAL_WIDTH *
                           WZ_RASTER_CANONICAL_HEIGHT) {
        free(pixels);
        return false;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        wz_machine_destroy(&machine);
        free(pixels);
        return false;
    }

    wz_machine_ula_port_fe_write(&machine, 0u, 2u, expected_events[0].master_tick);
    wz_machine_ula_port_fe_write(&machine, 0u, 3u, expected_events[1].master_tick);
    wz_machine_ula_port_fe_write(&machine, 0u, 6u, expected_events[2].master_tick);
    wz_machine_ula_port_fe_write(&machine, 0u, 1u, expected_events[3].master_tick);
    wz_machine_ula_port_fe_write(&machine, 0u, 4u, expected_events[4].master_tick);
    machine.master_tick = frame_ticks;

    if (wz_machine_border_events(&machine, recorded,
                                 sizeof(recorded) / sizeof(recorded[0])) !=
            sizeof(expected_events) / sizeof(expected_events[0])) {
        goto cleanup;
    }
    for (size_t index = 0u;
         index < sizeof(expected_events) / sizeof(expected_events[0]); ++index) {
        if (recorded[index].master_tick != expected_events[index].master_tick ||
            recorded[index].color != expected_events[index].color) {
            goto cleanup;
        }
    }

    if (wz_raster_buffer_init(&raster, WZ_RASTER_CANONICAL_WIDTH,
                              WZ_RASTER_CANONICAL_HEIGHT, pixels,
                              raster_size) != WZ_RESULT_OK ||
        wz_machine_render_raster(&machine, &raster) != WZ_RESULT_OK) {
        goto cleanup;
    }

    /* Frame top border follows writes at adjacent master ticks. */
    if (pixels[0u * WZ_RASTER_CANONICAL_WIDTH + 96u] != 0x12u ||
        pixels[0u * WZ_RASTER_CANONICAL_WIDTH + 97u] != 0x13u ||
        /* First active line's right border follows its timestamped changes. */
        pixels[64u * WZ_RASTER_CANONICAL_WIDTH + 352u] != 0x16u ||
        pixels[64u * WZ_RASTER_CANONICAL_WIDTH + 376u] != 0x11u ||
        /* The final horizontal phase wraps into the left border. */
        pixels[64u * WZ_RASTER_CANONICAL_WIDTH] != 0x14u ||
        /* A border event must never overwrite an active display sample. */
        pixels[64u * WZ_RASTER_CANONICAL_WIDTH + 200u] > WZ_RASTER_ACTIVE_MAX) {
        goto cleanup;
    }

    puts("PASS border_event_timing");
    success = true;

cleanup:
    wz_machine_destroy(&machine);
    free(pixels);
    return success;
}

static bool verify_flash_bright_semantics(void)
{
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    const size_t raster_size =
        (size_t)WZ_RASTER_CANONICAL_WIDTH * WZ_RASTER_CANONICAL_HEIGHT;
    wz_machine_t machine;
    wz_raster_buffer_t raster;
    wz_master_tick_t frame_ticks;
    wz_byte_t sample;
    wz_byte_t* pixels = (wz_byte_t*)malloc(raster_size);
    bool success = false;

    memset(&machine, 0, sizeof(machine));
    memset(&raster, 0, sizeof(raster));
    if (profile == 0 || pixels == 0 ||
        wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        goto cleanup;
    }
    frame_ticks = (wz_master_tick_t)profile->tstates_per_frame *
        profile->master_ticks_per_cpu_tstate;

    if (wz_raster_decode_attribute_phase(0x91u, true, false, &sample) !=
            WZ_RESULT_OK || sample != 1u ||
        wz_raster_decode_attribute_phase(0x91u, false, false, &sample) !=
            WZ_RESULT_OK || sample != 2u ||
        wz_raster_decode_attribute_phase(0x91u, true, true, &sample) !=
            WZ_RESULT_OK || sample != 2u ||
        wz_raster_decode_attribute_phase(0x91u, false, true, &sample) !=
            WZ_RESULT_OK || sample != 1u ||
        wz_raster_decode_attribute_phase(0xd1u, true, false, &sample) !=
            WZ_RESULT_OK || sample != 9u ||
        wz_raster_decode_attribute_phase(0xd1u, false, false, &sample) !=
            WZ_RESULT_OK || sample != 10u ||
        wz_raster_decode_attribute_phase(0xd1u, true, true, &sample) !=
            WZ_RESULT_OK || sample != 10u ||
        wz_raster_decode_attribute_phase(0xd1u, false, true, &sample) !=
            WZ_RESULT_OK || sample != 9u ||
        wz_raster_decode_attribute_phase(0x51u, true, true, &sample) !=
            WZ_RESULT_OK || sample != 9u ||
        wz_raster_decode_attribute_phase(0x51u, false, true, &sample) !=
            WZ_RESULT_OK || sample != 10u) {
        goto cleanup;
    }

    if (wz_machine_flash_phase(&machine, frame_ticks * 16u - 1u) ||
        !wz_machine_flash_phase(&machine, frame_ticks * 16u) ||
        !wz_machine_flash_phase(&machine, frame_ticks * 32u - 1u) ||
        wz_machine_flash_phase(&machine, frame_ticks * 32u)) {
        goto cleanup;
    }

    if (wz_machine_memory_write(&machine, 0x4000u, 0x80u) != WZ_RESULT_OK ||
        wz_machine_memory_write(&machine, 0x5800u, 0xd1u) != WZ_RESULT_OK ||
        wz_raster_buffer_init(&raster, WZ_RASTER_CANONICAL_WIDTH,
                              WZ_RASTER_CANONICAL_HEIGHT, pixels,
                              raster_size) != WZ_RESULT_OK) {
        goto cleanup;
    }
    machine.master_tick = frame_ticks * 16u + 1u;
    if (wz_machine_render_raster(&machine, &raster) != WZ_RESULT_OK ||
        pixels[64u * WZ_RASTER_CANONICAL_WIDTH + 96u] != 10u) {
        goto cleanup;
    }
    machine.master_tick = frame_ticks * 32u + 1u;
    if (wz_machine_render_raster(&machine, &raster) != WZ_RESULT_OK ||
        pixels[64u * WZ_RASTER_CANONICAL_WIDTH + 96u] != 9u) {
        goto cleanup;
    }

    puts("PASS flash_bright_semantics");
    success = true;

cleanup:
    wz_machine_destroy(&machine);
    free(pixels);
    return success;
}

static bool verify_ula_fetch_schedule(void)
{
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    wz_machine_profile_t invalid_profile;
    wz_machine_t machine;
    wz_ula_fetch_event_t events[2];
    wz_master_tick_t first_fetch_tick;
    wz_master_tick_t frame_ticks;
    size_t count = 0u;
    wz_result_t init_result;
    bool success = false;

    memset(&machine, 0, sizeof(machine));
    if (profile == 0) {
        return false;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        wz_machine_destroy(&machine);
        return false;
    }
    first_fetch_tick = (wz_master_tick_t)profile->ula_fetch_start_tstate *
        profile->master_ticks_per_cpu_tstate;
    frame_ticks = (wz_master_tick_t)profile->tstates_per_frame *
        profile->master_ticks_per_cpu_tstate;
    wz_machine_memory_write(&machine, 0x4000u, 0xa5u);
    wz_machine_memory_write(&machine, 0x5800u, 0x16u);
    wz_machine_memory_write(&machine, 0x4001u, 0x3cu);
    wz_machine_memory_write(&machine, 0x5801u, 0x25u);
    wz_machine_memory_write(&machine, 0x4100u, 0x81u);

    if (wz_machine_ula_fetches_at_tick(
            &machine, first_fetch_tick - profile->master_ticks_per_cpu_tstate,
            events, 2u, &count) != WZ_RESULT_OK || count != 0u ||
        wz_machine_ula_fetches_at_tick(&machine, first_fetch_tick, events, 2u,
                                       &count) != WZ_RESULT_OK || count != 2u ||
        events[0].kind != WZ_ULA_FETCH_BITMAP ||
        events[0].master_tick != first_fetch_tick ||
        events[0].address != 0x4000u || events[0].value != 0xa5u ||
        events[1].kind != WZ_ULA_FETCH_ATTRIBUTE ||
        events[1].master_tick != first_fetch_tick + 2u ||
        events[1].address != 0x5800u || events[1].value != 0x16u ||
        wz_machine_floating_bus_value(&machine, first_fetch_tick) != 0xa5u ||
        wz_machine_floating_bus_value(&machine, first_fetch_tick + 2u) != 0x16u) {
        goto cleanup;
    }

    if (wz_machine_ula_fetches_at_tick(
            &machine, first_fetch_tick +
                4u * profile->master_ticks_per_cpu_tstate,
            events, 2u, &count) != WZ_RESULT_OK || count != 2u ||
        events[0].address != 0x4001u || events[0].value != 0x3cu ||
        events[1].address != 0x5801u || events[1].value != 0x25u) {
        goto cleanup;
    }

    if (wz_machine_ula_fetches_at_tick(
            &machine, first_fetch_tick +
                (wz_master_tick_t)profile->tstates_per_line *
                    profile->master_ticks_per_cpu_tstate,
            events, 2u, &count) != WZ_RESULT_OK || count != 2u ||
        events[0].address != 0x4100u || events[0].value != 0x81u ||
        events[1].address != 0x5800u) {
        goto cleanup;
    }

    if (wz_machine_ula_fetches_at_tick(
            &machine, first_fetch_tick +
                (wz_master_tick_t)profile->ula_fetch_line_count *
                    profile->tstates_per_line *
                    profile->master_ticks_per_cpu_tstate,
            events, 2u, &count) != WZ_RESULT_OK || count != 0u) {
        goto cleanup;
    }

    if (wz_machine_ula_fetches_at_tick(
            &machine, frame_ticks + first_fetch_tick, events, 2u, &count) !=
            WZ_RESULT_OK || count != 2u ||
        events[0].master_tick != frame_ticks + first_fetch_tick ||
        events[0].address != 0x4000u || events[0].value != 0xa5u ||
        events[1].master_tick != frame_ticks + first_fetch_tick + 2u ||
        events[1].address != 0x5800u || events[1].value != 0x16u) {
        goto cleanup;
    }

    invalid_profile = *profile;
    invalid_profile.ula_fetch_line_count = WZ_ULA_CAPTURE_LINE_COUNT + 1u;
    init_result = wz_machine_init(&machine, &invalid_profile);
    if (init_result != WZ_RESULT_INVALID_PROFILE || machine.profile != profile) {
        goto cleanup;
    }
    invalid_profile = *profile;
    invalid_profile.ula_fetches_per_line =
        WZ_ULA_CAPTURE_CELLS_PER_LINE + 1u;
    init_result = wz_machine_init(&machine, &invalid_profile);
    if (init_result != WZ_RESULT_INVALID_PROFILE || machine.profile != profile) {
        goto cleanup;
    }

    puts("PASS ula_fetch_schedule");
    success = true;

cleanup:
    wz_machine_destroy(&machine);
    return success;
}

static bool verify_raster_write_fetch_order(void)
{
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    const size_t raster_size =
        (size_t)WZ_RASTER_CANONICAL_WIDTH * WZ_RASTER_CANONICAL_HEIGHT;
    wz_machine_t machine;
    wz_raster_buffer_t raster;
    wz_byte_t* pixels = (wz_byte_t*)malloc(raster_size);
    wz_master_tick_t first_fetch_tick;
    wz_master_tick_t frame_ticks;
    wz_master_tick_t final_fetch_tick;
    bool success = false;

    memset(&machine, 0, sizeof(machine));
    memset(&raster, 0, sizeof(raster));
    if (profile == 0 || pixels == 0) {
        free(pixels);
        return false;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        wz_machine_destroy(&machine);
        free(pixels);
        return false;
    }
    first_fetch_tick = (wz_master_tick_t)profile->ula_fetch_start_tstate *
        profile->master_ticks_per_cpu_tstate;
    frame_ticks = (wz_master_tick_t)profile->tstates_per_frame *
        profile->master_ticks_per_cpu_tstate;
    final_fetch_tick = ((wz_master_tick_t)profile->ula_fetch_start_tstate +
        (wz_master_tick_t)(profile->ula_fetch_line_count - 1u) *
            profile->tstates_per_line +
        (wz_master_tick_t)(profile->ula_fetches_per_line - 1u) *
            profile->ula_fetch_interval_tstates +
        profile->ula_attribute_offset_tstates) *
            profile->master_ticks_per_cpu_tstate;
    wz_machine_memory_write(&machine, 0x4000u, 0x80u);
    wz_machine_memory_write(&machine, 0x5800u, 0x01u);
    wz_machine_memory_write(&machine, 0x4001u, 0x00u);
    wz_machine_memory_write(&machine, 0x5801u, 0x01u);

    /* A write after both fetches must preserve the values already seen. */
    if (wz_machine_memory_write_at_tick(&machine, 0x4000u, 0x00u,
                                        first_fetch_tick + 4u) != WZ_RESULT_OK ||
        wz_machine_memory_write_at_tick(&machine, 0x5800u, 0x02u,
                                        first_fetch_tick + 4u) != WZ_RESULT_OK ||
        /* Same-tick CPU writes are visible to the following ULA fetch. */
        wz_machine_memory_write_at_tick(&machine, 0x4001u, 0x80u,
                                        first_fetch_tick + 8u) != WZ_RESULT_OK ||
        wz_machine_memory_write_at_tick(&machine, 0x5801u, 0x02u,
                                        first_fetch_tick + 10u) != WZ_RESULT_OK) {
        goto cleanup;
    }
    if (wz_machine_memory_write_at_tick(&machine, 0x4000u, 0xffu,
                                        final_fetch_tick) != WZ_RESULT_OK ||
        wz_machine_memory_write_at_tick(&machine, 0x4000u, 0x55u,
                                        frame_ticks - 1u) != WZ_RESULT_OK) {
        goto cleanup;
    }
    machine.master_tick = frame_ticks;
    if (wz_raster_buffer_init(&raster, WZ_RASTER_CANONICAL_WIDTH,
                              WZ_RASTER_CANONICAL_HEIGHT, pixels,
                              raster_size) != WZ_RESULT_OK ||
        wz_machine_render_raster(&machine, &raster) != WZ_RESULT_OK ||
        pixels[64u * WZ_RASTER_CANONICAL_WIDTH + 96u] != 1u ||
        pixels[64u * WZ_RASTER_CANONICAL_WIDTH + 97u] != 0u ||
        pixels[64u * WZ_RASTER_CANONICAL_WIDTH + 104u] != 2u) {
        goto cleanup;
    }

    puts("PASS raster_write_fetch_order");
    success = true;

cleanup:
    wz_machine_destroy(&machine);
    free(pixels);
    return success;
}

static bool fingerprint_audio(fingerprint_t* output)
{
    wz_ay_t ay;
    uint64_t digest = UINT64_C(14695981039346656037);

    wz_ay_init(&ay);
    if (wz_ay_select_register(&ay, 7u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x38u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 8u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x0fu, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 9u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x0bu, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 10u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x07u, 0u) != WZ_RESULT_OK) {
        return false;
    }
    for (size_t index = 0u; index < AUDIO_SAMPLES; ++index) {
        int32_t beeper = (int32_t)((index * 7919u) % 131071u) - 65535;
        wz_audio_sample_t sample = wz_audio_mixer_sample(beeper, &ay);
        digest ^= (uint32_t)sample;
        digest *= UINT64_C(1099511628211);
    }
    output->name = "audio_mixing";
    output->value = digest;
    return true;
}

static bool fingerprint_tape(fingerprint_t* output)
{
    wz_tape_t tape;
    wz_tape_state_t state;
    wz_tape_segment_t* segments = (wz_tape_segment_t*)malloc(
        sizeof(*segments) * TAPE_SEGMENTS);
    bool success = false;

    memset(&tape, 0, sizeof(tape));
    if (segments == NULL) {
        return false;
    }
    for (size_t index = 0u; index < TAPE_SEGMENTS; ++index) {
        segments[index].duration = 8u + (wz_master_tick_t)(index % 57u);
        segments[index].ear_level = (wz_byte_t)((index / 3u) & 1u);
    }
    if (wz_tape_mount(&tape, segments, TAPE_SEGMENTS) != WZ_RESULT_OK ||
        wz_tape_state_init(&state, &tape) != WZ_RESULT_OK ||
        wz_tape_state_set_motor(&state, true) != WZ_RESULT_OK) {
        goto cleanup;
    }
    for (size_t tick = 0u; tick < TAPE_TICKS; ++tick) {
        if (wz_tape_state_advance(&state, 1u) != WZ_RESULT_OK) {
            goto cleanup;
        }
    }
    output->name = "tape_playback";
    output->value = (uint64_t)state.segment_index ^
        ((uint64_t)state.segment_elapsed << 17u) ^
        ((uint64_t)state.ear_level << 61u) ^
        ((uint64_t)state.motor_on << 62u) ^
        ((uint64_t)state.at_end << 63u);
    success = true;

cleanup:
    free(segments);
    return success;
}

static bool fingerprint_snapshot(fingerprint_t* output)
{
    wz_machine_t machine;
    wz_machine_t restored;
    wz_state_writer_t writer;
    wz_qword_t original_hash;
    wz_qword_t restored_hash;
    wz_byte_t* bytes = (wz_byte_t*)malloc(WZ_STATE_MACHINE_LENGTH);
    bool initialized = false;
    bool success = false;

    memset(&machine, 0, sizeof(machine));
    memset(&restored, 0, sizeof(restored));
    if (bytes == NULL ||
        wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(bytes);
        return false;
    }
    wz_state_writer_init(&writer, bytes, WZ_STATE_MACHINE_LENGTH);
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK ||
        wz_state_hash_machine(&machine, &original_hash) != WZ_RESULT_OK ||
        wz_machine_init(&restored, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        goto cleanup;
    }
    initialized = true;
    if (wz_state_deserialize_machine(&restored, bytes, writer.length) !=
            WZ_RESULT_OK ||
        wz_state_hash_machine(&restored, &restored_hash) != WZ_RESULT_OK ||
        original_hash != restored_hash) {
        goto cleanup;
    }
    output->name = "snapshot_serialization";
    output->value = hash_bytes(bytes, writer.length);
    success = true;

cleanup:
    if (initialized) {
        wz_machine_destroy(&restored);
    }
    wz_machine_destroy(&machine);
    free(bytes);
    return success;
}

int main(void)
{
    static const char* const names[] = {
        "cpu_execution", "raster_generation", "audio_mixing",
        "tape_playback", "snapshot_serialization"
    };
    fingerprint_t actual[sizeof(names) / sizeof(names[0])];
    const fingerprint_t expected[] = {
        {"cpu_execution", WZ_EXPECTED_CPU_EXECUTION},
        {"raster_generation", WZ_EXPECTED_RASTER_GENERATION},
        {"audio_mixing", WZ_EXPECTED_AUDIO_MIXING},
        {"tape_playback", WZ_EXPECTED_TAPE_PLAYBACK},
        {"snapshot_serialization", WZ_EXPECTED_SNAPSHOT_SERIALIZATION}
    };
    bool success;

    success = verify_ula_fetch_schedule() &&
        verify_raster_write_fetch_order() &&
        verify_flash_bright_semantics() &&
        verify_border_event_timing() &&
        fingerprint_cpu(&actual[0]) &&
        fingerprint_raster(&actual[1]) &&
        fingerprint_audio(&actual[2]) &&
        fingerprint_tape(&actual[3]) &&
        fingerprint_snapshot(&actual[4]);
    if (!success) {
        fprintf(stderr, "canonical core workload failed\n");
        return 1;
    }
    for (size_t index = 0u; index < sizeof(names) / sizeof(names[0]); ++index) {
        if (!check_fingerprint(&actual[index], &expected[index])) {
            return 1;
        }
    }
    return 0;
}
