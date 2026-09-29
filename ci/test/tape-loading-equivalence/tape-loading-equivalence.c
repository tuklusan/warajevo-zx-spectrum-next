/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/wz_keyboard_matrix.h"
#include "core/wz_machine.h"
#include "core/wz_runner.h"
#include "core/wz_state.h"
#include "core/wz_tape.h"
#include "core/wz_trace.h"

#include <stdio.h>
#include <stdlib.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "tape loading equivalence failed at line %d: %s\n", \
                __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

static wz_byte_t* read_file(const char* path, size_t* length)
{
    FILE* file;
    long size;
    wz_byte_t* bytes;
    if (path == NULL || length == NULL || (file = fopen(path, "rb")) == NULL)
        return NULL;
    if (fseek(file, 0L, SEEK_END) != 0 || (size = ftell(file)) <= 0L ||
        fseek(file, 0L, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    bytes = (wz_byte_t*)malloc((size_t)size);
    if (bytes == NULL || fread(bytes, 1u, (size_t)size, file) != (size_t)size) {
        free(bytes);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *length = (size_t)size;
    return bytes;
}

static int run_frames(wz_headless_runner_t* runner,
                      wz_master_tick_t frame_ticks,
                      wz_master_tick_t frames)
{
    if (frames != 0u && frame_ticks > UINT64_MAX / frames) return 0;
    return wz_headless_runner_execute(runner, frame_ticks * frames) ==
           WZ_RESULT_OK;
}

static int tap_key(wz_machine_t* machine, wz_headless_runner_t* runner,
                   wz_master_tick_t frame_ticks, wz_keyboard_key_t key)
{
    size_t row;
    size_t column;
    if (!wz_keyboard_matrix_key_position(key, &row, &column)) return 0;
    if (key == WZ_KEY_ENTER) {
        if (wz_tape_state_at_end(&machine->tape_state) &&
            wz_machine_rewind_tape(machine) != WZ_RESULT_OK) return 0;
        if (wz_machine_set_tape_motor(machine, true) != WZ_RESULT_OK) return 0;
    }
    if (wz_machine_set_keyboard_key(machine, (wz_byte_t)row,
                                    (wz_byte_t)column, true) != WZ_RESULT_OK ||
        (wz_machine_ula_port_fe_read(machine,
            (wz_word_t)(0xfffeu & ~(1u << (8u + row)))) &
            (wz_byte_t)(1u << column)) != 0u ||
        !run_frames(runner, frame_ticks, 8u)) return 0;
    if (wz_machine_set_keyboard_key(machine, (wz_byte_t)row,
                                    (wz_byte_t)column, false) != WZ_RESULT_OK ||
        !run_frames(runner, frame_ticks, 8u)) return 0;
    return 1;
}

static wz_word_t find_basic_payload(wz_machine_t* machine)
{
    static const wz_byte_t prefix[] = {
        0x00u, 0x0au, 0x1bu, 0x00u, 0xe7u, 0xc3u, 0xa7u, 0x3au
    };
    for (wz_word_t address = 0x4000u;
         address <= (wz_word_t)(0x10000u - sizeof(prefix)); ++address) {
        size_t index;
        for (index = 0u; index < sizeof(prefix); ++index) {
            if (wz_machine_memory_read(machine,
                    (wz_word_t)(address + index)) != prefix[index]) break;
        }
        if (index == sizeof(prefix)) return address;
    }
    return 0u;
}

static int load_basic(const char* rom_path, const char* tape_path,
                      wz_tape_loading_mode_t mode, wz_qword_t* state_hash,
                      wz_word_t* basic_address, size_t* tape_segment)
{
    wz_machine_t machine = {0};
    wz_headless_runner_t runner;
    wz_trace_sink_t trace;
    wz_byte_t* rom = NULL;
    wz_byte_t* tap = NULL;
    wz_tape_segment_t* segments = NULL;
    size_t rom_length = 0u;
    size_t tap_length = 0u;
    size_t segment_count = 0u;
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    wz_master_tick_t frame_ticks;
    int initialized = 0;
    int result = 0;

    rom = read_file(rom_path, &rom_length);
    tap = read_file(tape_path, &tap_length);
    if (rom == NULL || tap == NULL) goto cleanup;
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) goto cleanup;
    initialized = 1;
    if (wz_machine_load_48k_rom(&machine, rom, rom_length) != WZ_RESULT_OK ||
        wz_machine_set_tape_loading_mode(&machine, mode) != WZ_RESULT_OK ||
        wz_tape_parse_standard_tap(tap, tap_length,
            profile->master_ticks_per_cpu_tstate, NULL, 0u, &segment_count) !=
            WZ_RESULT_BUFFER_TOO_SMALL ||
        segment_count == 0u || segment_count > SIZE_MAX / sizeof(*segments)) {
        goto cleanup;
    }
    segments = (wz_tape_segment_t*)malloc(segment_count * sizeof(*segments));
    if (segments == NULL ||
        wz_tape_parse_standard_tap(tap, tap_length,
            profile->master_ticks_per_cpu_tstate, segments, segment_count,
            &segment_count) != WZ_RESULT_OK ||
        wz_machine_mount_tape(&machine, segments, segment_count) != WZ_RESULT_OK)
        goto cleanup;
    if (mode == WZ_TAPE_LOADING_INSTANT_TRAP &&
        wz_machine_tape_trap_reason(&machine) !=
            WZ_TAPE_TRAP_REASON_NO_RECOGNIZED_LOADER) goto cleanup;

    wz_trace_sink_init(&trace, NULL, NULL);
    if (wz_headless_runner_init(&runner, &machine, &trace) != WZ_RESULT_OK)
        goto cleanup;
    frame_ticks = (wz_master_tick_t)profile->tstates_per_frame *
                  profile->master_ticks_per_cpu_tstate;
    if (!run_frames(&runner, frame_ticks, 160u) ||
        !tap_key(&machine, &runner, frame_ticks, WZ_KEY_J)) goto cleanup;

    /* Enter the ROM's LOAD command using the same matrix sequence as the
       established ROM-loader regression. */
    if (wz_machine_set_keyboard_key(&machine, 7u, 1u, true) != WZ_RESULT_OK ||
        !run_frames(&runner, frame_ticks, 8u) ||
        wz_machine_set_keyboard_key(&machine, 5u, 0u, true) != WZ_RESULT_OK ||
        !run_frames(&runner, frame_ticks, 8u) ||
        wz_machine_set_keyboard_key(&machine, 5u, 0u, false) != WZ_RESULT_OK ||
        !run_frames(&runner, frame_ticks, 8u) ||
        wz_machine_set_keyboard_key(&machine, 5u, 0u, true) != WZ_RESULT_OK ||
        !run_frames(&runner, frame_ticks, 8u) ||
        wz_machine_set_keyboard_key(&machine, 5u, 0u, false) != WZ_RESULT_OK ||
        !run_frames(&runner, frame_ticks, 8u) ||
        wz_machine_set_keyboard_key(&machine, 7u, 1u, false) != WZ_RESULT_OK ||
        !run_frames(&runner, frame_ticks, 8u) ||
        !tap_key(&machine, &runner, frame_ticks, WZ_KEY_ENTER)) goto cleanup;

    for (wz_master_tick_t frame = 0u;
         frame < 6000u && machine.tape_state.motor_on; ++frame) {
        if (!run_frames(&runner, frame_ticks, 1u)) goto cleanup;
    }
    if (machine.tape_state.motor_on || wz_tape_state_at_end(&machine.tape_state))
        goto cleanup;
    *basic_address = (wz_word_t)(wz_machine_memory_read(&machine, 0x5c53u) |
        ((wz_word_t)wz_machine_memory_read(&machine, 0x5c54u) << 8u));
    if (*basic_address < 0x5ccbu || *basic_address >= 0xfffcu) goto cleanup;
    if (find_basic_payload(&machine) != *basic_address) goto cleanup;
    *tape_segment = machine.tape_state.segment_index;

    /* Loading preference is host policy, not Spectrum-visible machine state. */
    if (wz_machine_set_tape_loading_mode(&machine, WZ_TAPE_LOADING_NORMAL) !=
        WZ_RESULT_OK || wz_state_hash_machine(&machine, state_hash) !=
        WZ_RESULT_OK) goto cleanup;
    result = 1;
cleanup:
    if (initialized) wz_machine_destroy(&machine);
    free(segments);
    free(tap);
    free(rom);
    return result;
}

int main(int argc, char** argv)
{
    wz_qword_t normal_hash = 0u;
    wz_qword_t instant_hash = 0u;
    wz_word_t normal_address = 0u;
    wz_word_t instant_address = 0u;
    size_t normal_segment = 0u;
    size_t instant_segment = 0u;
    if (argc != 3 ||
        !load_basic(argv[1], argv[2], WZ_TAPE_LOADING_NORMAL, &normal_hash,
                    &normal_address, &normal_segment) ||
        !load_basic(argv[1], argv[2], WZ_TAPE_LOADING_INSTANT_TRAP,
                    &instant_hash, &instant_address, &instant_segment) ||
        normal_hash != instant_hash || normal_address != instant_address ||
        normal_segment != instant_segment) return 1;
    printf("tape-loading-equivalence cases=2 status=pass hash=%016llx\n",
           (unsigned long long)normal_hash);
    return 0;
}
