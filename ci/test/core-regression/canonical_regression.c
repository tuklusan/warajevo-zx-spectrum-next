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
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        return false;
    }
    for (size_t index = 0u; index < sizeof(program); ++index) {
        wz_machine_memory_write(&machine, (wz_word_t)(0x8000u + index),
                                program[index]);
    }
    machine.cpu.program_counter = 0x8000u;
    machine.cpu.main.h = 0xc0u;
    for (size_t index = 0u; index < CPU_STEPS; ++index) {
        if (wz_z80_step(&machine) != WZ_RESULT_OK) {
            wz_machine_destroy(&machine);
            return false;
        }
    }
    output->name = "cpu_execution";
    if (machine.cpu.program_counter != 0x8000u ||
        wz_state_hash_machine(&machine, &output->value) != WZ_RESULT_OK) {
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

    success = fingerprint_cpu(&actual[0]) &&
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
