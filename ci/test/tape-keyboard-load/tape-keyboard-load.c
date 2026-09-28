/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdio.h>
#include <stdlib.h>

#include "core/wz_keyboard_matrix.h"
#include "core/wz_machine.h"
#include "core/wz_runner.h"

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "DIZZY4K tape load regression failed at line %d: %s\n", \
                __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

static wz_byte_t* read_file(const char* path, size_t* length)
{
    FILE* file;
    long size;
    wz_byte_t* bytes;

    if (path == NULL || length == NULL || (file = fopen(path, "rb")) == NULL) {
        return NULL;
    }
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

static int run_until_tape_stops(wz_machine_t* machine,
                                wz_headless_runner_t* runner,
                                wz_master_tick_t frame_ticks)
{
    for (wz_master_tick_t frame = 0u;
         frame < 6000u && machine->tape_state.motor_on; ++frame) {
        if (!run_frames(runner, frame_ticks, 1u)) return 0;
    }
    return !machine->tape_state.motor_on;
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

static int tap_key(wz_machine_t* machine, wz_headless_runner_t* runner,
                   wz_master_tick_t frame_ticks, wz_keyboard_key_t key)
{
    size_t row;
    size_t column;

    if (!wz_keyboard_matrix_key_position(key, &row, &column)) return 0;
    if (key == WZ_KEY_ENTER) {
        if (wz_tape_state_at_end(&machine->tape_state)) {
            if (wz_machine_rewind_tape(machine) != WZ_RESULT_OK) return 0;
        }
        if (wz_machine_set_tape_motor(machine, true) != WZ_RESULT_OK) return 0;
    }
    if (wz_machine_set_keyboard_key(machine, (wz_byte_t)row,
                                    (wz_byte_t)column, true) != WZ_RESULT_OK) {
        return 0;
    }
    if ((wz_machine_ula_port_fe_read(machine,
            (wz_word_t)(0xfffeu & ~(1u << (8u + row)))) &
            (wz_byte_t)(1u << column)) != 0u ||
        !run_frames(runner, frame_ticks, 8u)) return 0;
    if (wz_machine_set_keyboard_key(machine, (wz_byte_t)row,
                                    (wz_byte_t)column, false) != WZ_RESULT_OK ||
        !run_frames(runner, frame_ticks, 8u)) return 0;
    return 1;
}

int main(int argc, char** argv)
{
    wz_machine_t machine = {0};
    wz_headless_runner_t runner;
    wz_byte_t* rom = NULL;
    wz_byte_t* tap = NULL;
    wz_tape_segment_t* segments = NULL;
    size_t rom_length = 0u;
    size_t tap_length = 0u;
    size_t segment_count = 0u;
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    wz_master_tick_t frame_ticks;
    wz_word_t basic_address;
    wz_word_t loaded_basic_address;
    wz_word_t edit_line;
    int result = 1;
    int machine_initialized = 0;

    REQUIRE(argc == 3);
    rom = read_file(argv[1], &rom_length);
    tap = read_file(argv[2], &tap_length);
    REQUIRE(rom != NULL && tap != NULL);
    REQUIRE(wz_machine_init(&machine, profile) == WZ_RESULT_OK);
    machine_initialized = 1;
    REQUIRE(wz_machine_load_48k_rom(&machine, rom, rom_length) == WZ_RESULT_OK);
    REQUIRE(wz_tape_parse_standard_tap(tap, tap_length,
        profile->master_ticks_per_cpu_tstate, NULL, 0u, &segment_count) ==
        WZ_RESULT_BUFFER_TOO_SMALL);
    REQUIRE(segment_count != 0u &&
        segment_count <= SIZE_MAX / sizeof(*segments));
    segments = (wz_tape_segment_t*)malloc(segment_count * sizeof(*segments));
    REQUIRE(segments != NULL);
    REQUIRE(wz_tape_parse_standard_tap(tap, tap_length,
        profile->master_ticks_per_cpu_tstate, segments, segment_count,
        &segment_count) == WZ_RESULT_OK);
    REQUIRE(wz_machine_mount_tape(&machine, segments, segment_count) ==
            WZ_RESULT_OK);
    REQUIRE(wz_headless_runner_init(&runner, &machine, NULL) == WZ_RESULT_OK);
    frame_ticks = (wz_master_tick_t)profile->tstates_per_frame *
                  profile->master_ticks_per_cpu_tstate;

    REQUIRE(run_frames(&runner, frame_ticks, 80u));
    REQUIRE(tap_key(&machine, &runner, frame_ticks, WZ_KEY_J));
    edit_line = (wz_word_t)(wz_machine_memory_read(&machine, 0x5c59u) |
        ((wz_word_t)wz_machine_memory_read(&machine, 0x5c5au) << 8u));
    (void)printf("after J: E_LINE=%04x bytes=", edit_line);
    for (size_t index = 0u; index < 8u; ++index) {
        (void)printf("%02x", wz_machine_memory_read(&machine,
            (wz_word_t)(edit_line + index)));
    }
    (void)putchar('\n');
    REQUIRE(wz_machine_set_keyboard_key(&machine, 7u, 1u, true) ==
            WZ_RESULT_OK);
    REQUIRE(run_frames(&runner, frame_ticks, 8u));
    REQUIRE(wz_machine_set_keyboard_key(&machine, 5u, 0u, true) ==
            WZ_RESULT_OK);
    REQUIRE(run_frames(&runner, frame_ticks, 8u));
    REQUIRE(wz_machine_set_keyboard_key(&machine, 5u, 0u, false) ==
            WZ_RESULT_OK);
    REQUIRE(run_frames(&runner, frame_ticks, 8u));
    REQUIRE(wz_machine_set_keyboard_key(&machine, 5u, 0u, true) ==
            WZ_RESULT_OK);
    REQUIRE(run_frames(&runner, frame_ticks, 8u));
    REQUIRE(wz_machine_set_keyboard_key(&machine, 5u, 0u, false) ==
            WZ_RESULT_OK);
    REQUIRE(run_frames(&runner, frame_ticks, 8u));
    REQUIRE(wz_machine_set_keyboard_key(&machine, 7u, 1u, false) ==
            WZ_RESULT_OK);
    REQUIRE(run_frames(&runner, frame_ticks, 8u));
    edit_line = (wz_word_t)(wz_machine_memory_read(&machine, 0x5c59u) |
        ((wz_word_t)wz_machine_memory_read(&machine, 0x5c5au) << 8u));
    (void)printf("before ENTER: E_LINE=%04x bytes=", edit_line);
    for (size_t index = 0u; index < 12u; ++index) {
        (void)printf("%02x", wz_machine_memory_read(&machine,
            (wz_word_t)(edit_line + index)));
    }
    (void)putchar('\n');
    REQUIRE(tap_key(&machine, &runner, frame_ticks, WZ_KEY_ENTER));
    REQUIRE(run_until_tape_stops(&machine, &runner, frame_ticks));
    REQUIRE(machine.tape_state.motor_on == false);
    REQUIRE(!wz_tape_state_at_end(&machine.tape_state));

    basic_address = (wz_word_t)(wz_machine_memory_read(&machine, 0x5c53u) |
        ((wz_word_t)wz_machine_memory_read(&machine, 0x5c54u) << 8u));
    loaded_basic_address = find_basic_payload(&machine);
    (void)printf("after LOAD: PC=%04x PROG=%04x payload=%04x BASIC=%02x%02x%02x%02x tape-segment=%lu end=%u\n",
        machine.cpu.program_counter, basic_address, loaded_basic_address,
        wz_machine_memory_read(&machine, basic_address),
        wz_machine_memory_read(&machine, (wz_word_t)(basic_address + 1u)),
        wz_machine_memory_read(&machine, (wz_word_t)(basic_address + 2u)),
        wz_machine_memory_read(&machine, (wz_word_t)(basic_address + 3u)),
        (unsigned long)machine.tape_state.segment_index,
        (unsigned)wz_tape_state_at_end(&machine.tape_state));
    REQUIRE(basic_address >= 0x5ccbu && basic_address < 0xfffcu);
    REQUIRE(loaded_basic_address != 0u);
    REQUIRE(loaded_basic_address == basic_address);

    REQUIRE(tap_key(&machine, &runner, frame_ticks, WZ_KEY_R));
    REQUIRE(tap_key(&machine, &runner, frame_ticks, WZ_KEY_U));
    REQUIRE(tap_key(&machine, &runner, frame_ticks, WZ_KEY_N));
    REQUIRE(tap_key(&machine, &runner, frame_ticks, WZ_KEY_ENTER));
    REQUIRE(machine.tape_state.motor_on);
    REQUIRE(run_until_tape_stops(&machine, &runner, frame_ticks));
    REQUIRE(wz_tape_state_at_end(&machine.tape_state));
    (void)printf("after RUN: PC=%04x code=%02x%02x%02x%02x tape-segment=%lu end=%u\n",
        machine.cpu.program_counter, wz_machine_memory_read(&machine, 0x8000u),
        wz_machine_memory_read(&machine, 0x8001u),
        wz_machine_memory_read(&machine, 0x8002u),
        wz_machine_memory_read(&machine, 0x8003u),
        (unsigned long)machine.tape_state.segment_index,
        (unsigned)wz_tape_state_at_end(&machine.tape_state));
    REQUIRE(wz_machine_memory_read(&machine, 0x8000u) == 0xf3u);
    (void)puts("DIZZY4K BASIC and machine-code blocks loaded through the normal ROM path");
    result = 0;

cleanup:
    if (machine_initialized) wz_machine_destroy(&machine);
    free(segments);
    free(tap);
    free(rom);
    return result;
}
