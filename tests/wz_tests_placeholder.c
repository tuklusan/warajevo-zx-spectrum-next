/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/wz_machine.h"
#include "core/wz_microdrive.h"
#include "core/wz_zxnet.h"
#include "core/wz_keyboard_matrix.h"
#include "core/wz_kempston.h"
#include "app/wz_input_timing.h"
#include "app/wz_input_focus.h"
#include "app/wz_kempston_mapping.h"
#include "app/wz_host_media_ownership.h"
#include "app/wz_speed_policy.h"
#include "app/wz_host_pacing.h"
#include "app/wz_host_audio_push.h"
#include "app/wz_host_audio_policy.h"
#include "core/audio/wz_audio_policy.h"
#include "core/audio/wz_audio_evidence.h"
#include "core/audio/wz_audio_mixer.h"
#include "core/audio/wz_ay_mixer_policy.h"
#include "core/wz_bus.h"
#include "core/wz_scheduler.h"
#include "core/wz_state.h"
#include "core/wz_runner.h"
#include "core/wz_tape.h"
#include "core/wz_raster_evidence.h"
#include "core/wz_raster_diagnostic.h"
#include "diagnostics/wz_trace_file.h"

static void test_raster_evidence(void)
{
    wz_byte_t samples[4] = {0u, 1u, 2u, 3u};
    wz_raster_buffer_t buffer = {samples, 2u, 2u};
    wz_trace_event_t event = {0};
    wz_qword_t first_hash = 0u;
    wz_qword_t second_hash = 0u;

    if (wz_raster_buffer_hash(&buffer, &first_hash) != WZ_RESULT_OK ||
        wz_raster_buffer_hash(&buffer, &second_hash) != WZ_RESULT_OK ||
        first_hash != second_hash || first_hash == 0u) {
        fputs("raster evidence hash failed\n", stderr);
        exit(1);
    }
    event.kind = WZ_TRACE_CPU_BUS;
    event.master_tick = 17u;
    event.address = 0x4000u;
    event.value = 0xa5u;
    if (wz_trace_events_hash(&event, 1u, &first_hash) != WZ_RESULT_OK ||
        wz_trace_events_hash(&event, 1u, &second_hash) != WZ_RESULT_OK ||
        first_hash != second_hash || first_hash == 0u) {
        fputs("trace evidence hash failed\n", stderr);
        exit(1);
    }
    event.value = 0xa4u;
    if (wz_trace_events_hash(&event, 1u, &second_hash) != WZ_RESULT_OK ||
        first_hash == second_hash) {
        fputs("trace evidence hash collision fixture failed\n", stderr);
        exit(1);
    }
}

static void test_complete_keyboard_matrix(void)
{
    wz_keyboard_matrix_t matrix;
    size_t row;
    size_t column;

    wz_keyboard_matrix_init(&matrix);
    for (size_t index = 0u; index < WZ_KEYBOARD_MATRIX_KEY_COUNT; ++index) {
        if (!wz_keyboard_matrix_key_position((wz_keyboard_key_t)index, &row, &column) ||
            row != index / WZ_KEYBOARD_MATRIX_KEYS_PER_ROW ||
            column != index % WZ_KEYBOARD_MATRIX_KEYS_PER_ROW ||
            !wz_keyboard_matrix_set(&matrix, (wz_keyboard_key_t)index, true) ||
            wz_keyboard_matrix_scan(&matrix, (unsigned char)~(1u << row)) !=
                (wz_byte_t)(0x1fu & (wz_byte_t)~(1u << column)) ||
            !wz_keyboard_matrix_set(&matrix, (wz_keyboard_key_t)index, false)) {
            fputs("complete keyboard matrix coverage failed\n", stderr);
            exit(1);
        }
    }
    if (wz_keyboard_matrix_scan(&matrix, 0xffu) != 0x1fu ||
        !wz_keyboard_matrix_set(&matrix, WZ_KEY_SHIFT, true) ||
        !wz_keyboard_matrix_set(&matrix, WZ_KEY_SPACE, true) ||
        wz_keyboard_matrix_scan(&matrix, 0x7eu) != 0x1eu ||
        wz_keyboard_matrix_scan(&matrix, 0x3fu) != 0x1eu) {
        fputs("keyboard matrix multi-row scan failed\n", stderr);
        exit(1);
    }
    wz_keyboard_matrix_init(&matrix);
    if (!wz_keyboard_matrix_set(&matrix, WZ_KEY_SHIFT, true) ||
        !wz_keyboard_matrix_set(&matrix, WZ_KEY_Z, true) ||
        !wz_keyboard_matrix_set(&matrix, WZ_KEY_A, true) ||
        wz_keyboard_matrix_scan(&matrix, 0xfdu) != 0x1cu) {
        fputs("keyboard matrix ghosting policy failed\n", stderr);
        exit(1);
    }
}

static void test_input_timestamp_assignment(void)
{
    wz_input_timestamp_assigner_t assigner;
    wz_input_event_t event = {0u, 3u, 1u};
    wz_timed_input_event_t first;
    wz_timed_input_event_t second;

    wz_input_timestamp_assigner_init(&assigner);
    if (!wz_input_timestamp_assign(&assigner, &event, 100u, &first) ||
        !wz_input_timestamp_assign(&assigner, &event, 90u, &second) ||
        first.master_tick != 100u || second.master_tick != 100u ||
        first.sequence != 0u || second.sequence != 1u ||
        !wz_input_timestamp_assign(&assigner, &event, UINT64_MAX, &second) ||
        second.master_tick != UINT64_MAX || second.sequence != 2u) {
        fputs("input timestamp assignment failed\n", stderr);
        exit(1);
    }
    event.pressed = 0u;
    if (!wz_input_timestamp_assign(&assigner, &event, UINT64_MAX, &second) ||
        second.event.pressed != 0u || second.master_tick != UINT64_MAX ||
        second.sequence != 3u) {
        fputs("input release timestamp assignment failed\n", stderr);
        exit(1);
    }
    assigner.next_sequence = UINT64_MAX;
    if (wz_input_timestamp_assign(&assigner, &event, UINT64_MAX, &second)) {
        fputs("input timestamp sequence exhaustion failed\n", stderr);
        exit(1);
    }
}

static void test_input_focus_loss(void)
{
    wz_input_arbiter_t arbiter;
    wz_input_focus_controller_t controller;

    wz_input_arbiter_init(&arbiter);
    wz_input_focus_init(&controller, &arbiter);
    if (!wz_input_arbiter_set(&arbiter, WZ_INPUT_SOURCE_LOCAL, 2u, true) ||
        !wz_input_arbiter_set(&arbiter, WZ_INPUT_SOURCE_TELNET, 2u, true) ||
        !wz_input_focus_lost(&controller) || wz_input_focus_is_focused(&controller) ||
        !wz_input_arbiter_key_down(&arbiter, 2u) ||
        !wz_input_focus_lost(&controller) || wz_input_focus_is_focused(&controller) ||
        !wz_input_focus_gained(&controller) || !wz_input_focus_is_focused(&controller) ||
        wz_input_arbiter_key_down(&arbiter, 2u) != true) {
        fputs("input focus-loss isolation failed\n", stderr);
        exit(1);
    }
    if (!wz_input_arbiter_set(&arbiter, WZ_INPUT_SOURCE_LOCAL, 2u, true) ||
        !wz_input_focus_lost(&controller) || !wz_input_arbiter_key_down(&arbiter, 2u)) {
        fputs("input focus regain behavior failed\n", stderr);
        exit(1);
    }
}

static void test_kempston_decode(void)
{
    wz_kempston_t joystick;

    wz_kempston_init(&joystick);
    if (!wz_kempston_port_selected(0x001fu) || !wz_kempston_port_selected(0x201fu) ||
        wz_kempston_read(&joystick, 0x1fu) != 0u ||
        wz_kempston_read(&joystick, 0x1eu) != 0u) {
        fputs("Kempston port selection failed\n", stderr);
        exit(1);
    }
    for (size_t index = 0u; index < WZ_KEMPSTON_CONTROL_COUNT; ++index) {
        if (!wz_kempston_set(&joystick, (wz_kempston_control_t)index, true) ||
            (wz_kempston_read(&joystick, 0x1fu) &
             (wz_byte_t[]){0x10u, 0x08u, 0x02u, 0x01u, 0x04u}[index]) == 0u ||
            !wz_kempston_set(&joystick, (wz_kempston_control_t)index, false)) {
            fputs("Kempston bit decode failed\n", stderr);
            exit(1);
        }
    }
    if (!wz_kempston_set(&joystick, WZ_KEMPSTON_RIGHT, true) ||
        !wz_kempston_set(&joystick, WZ_KEMPSTON_DOWN, true) ||
        wz_kempston_read(&joystick, 0x1fu) != 0x12u) {
        fputs("Kempston combination decode failed\n", stderr);
        exit(1);
    }
}

static void test_kempston_mapping(void)
{
    static const unsigned defaults[WZ_KEMPSTON_MAPPING_CONTROL_COUNT] =
        {100u, 101u, 102u, 103u, 104u};
    wz_kempston_mapping_t mapping;
    wz_kempston_t joystick;

    wz_kempston_mapping_init(&mapping, defaults);
    wz_kempston_init(&joystick);
    if (!wz_kempston_mapping_apply(&mapping, &joystick, 100u, true) ||
        wz_kempston_read(&joystick, WZ_KEMPSTON_PORT) != 0x10u ||
        !wz_kempston_mapping_bind(&mapping, WZ_KEMPSTON_FIRE, 100u) ||
        !wz_kempston_mapping_apply(&mapping, &joystick, 100u, true) ||
        wz_kempston_read(&joystick, WZ_KEMPSTON_PORT) != 0x14u ||
        !wz_kempston_mapping_apply(&mapping, &joystick, 100u, false) ||
        wz_kempston_read(&joystick, WZ_KEMPSTON_PORT) != 0u ||
        !wz_kempston_mapping_unbind(&mapping, WZ_KEMPSTON_RIGHT) ||
        !wz_kempston_mapping_unbind(&mapping, WZ_KEMPSTON_FIRE) ||
        wz_kempston_mapping_apply(&mapping, &joystick, 100u, true)) {
        fputs("Kempston host mapping failed\n", stderr);
        exit(1);
    }
}

static void test_speed_policy(void)
{
    static const unsigned expected[WZ_SPEED_COUNT] =
        {25u, 50u, 100u, 200u, 400u, 800u, 0u};

    for (unsigned index = 0u; index < WZ_SPEED_COUNT; ++index) {
        wz_speed_policy_t policy = (wz_speed_policy_t)index;
        if (!wz_speed_policy_valid(policy) ||
            wz_speed_policy_percent(policy) != expected[index] ||
            wz_speed_policy_is_unlimited(policy) !=
                (policy == WZ_SPEED_UNLIMITED)) {
            fputs("speed policy value contract failed\n", stderr);
            exit(1);
        }
    }
    if (wz_speed_policy_valid((wz_speed_policy_t)WZ_SPEED_COUNT) ||
        wz_speed_policy_percent((wz_speed_policy_t)WZ_SPEED_COUNT) != 0u ||
        wz_speed_policy_is_unlimited((wz_speed_policy_t)WZ_SPEED_COUNT)) {
        fputs("speed policy invalid-value contract failed\n", stderr);
        exit(1);
    }
}

typedef struct {
    wz_qword_t nanoseconds;
    unsigned calls;
} pacing_sleep_record_t;

static bool record_pacing_sleep(wz_qword_t nanoseconds, void* context)
{
    pacing_sleep_record_t* recorded = (pacing_sleep_record_t*)context;
    recorded->nanoseconds = nanoseconds;
    recorded->calls += 1u;
    return true;
}

static void test_host_pacing(void)
{
    wz_host_pacing_t pacing;
    pacing_sleep_record_t sleep = {0u, 0u};
    wz_qword_t requested = 0u;

    if (!wz_host_pacing_init(&pacing, 1000u, WZ_SPEED_100, 1000000000u, 0u) ||
        !wz_host_pacing_wait(&pacing, 1000000000u, 1000u,
                             record_pacing_sleep, &sleep, &requested) ||
        requested != 1000000000u || sleep.nanoseconds != requested || sleep.calls != 1u ||
        !wz_host_pacing_wait(&pacing, 2000000000u, 1000u,
                             record_pacing_sleep, &sleep, &requested) ||
        requested != 0u ||
        !wz_host_pacing_set_speed(&pacing, WZ_SPEED_200) ||
        !wz_host_pacing_wait(&pacing, 1000000000u, 1000u,
                             record_pacing_sleep, &sleep, &requested) ||
        requested != 0u ||
        !wz_host_pacing_wait(&pacing, 1000000000u, 2000u,
                             record_pacing_sleep, &sleep, &requested) ||
        requested != 500000000u || sleep.nanoseconds != requested ||
        !wz_host_pacing_wait(&pacing, 1000000000u, 0u,
                             record_pacing_sleep, &sleep, &requested) ||
        requested != 0u ||
        !wz_host_pacing_set_speed(&pacing, WZ_SPEED_UNLIMITED) ||
        !wz_host_pacing_wait(&pacing, 0u, UINT64_MAX,
                             record_pacing_sleep, &sleep, &requested) ||
        requested != 0u || sleep.calls != 2u) {
        fprintf(stderr, "host pacing contract failed: request=%llu sleep=%llu\n",
                (unsigned long long)requested,
                (unsigned long long)sleep.nanoseconds);
        exit(1);
    }
}

static void test_audio_speed_boundary_transitions(void)
{
    wz_host_pacing_t pacing;
    wz_machine_t machine;
    wz_headless_runner_t runner;
    wz_trace_sink_t trace;
    wz_qword_t requested = 0u;
    wz_master_tick_t before_tick;

    wz_trace_sink_init(&trace, 0, 0);
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_headless_runner_init(&runner, &machine, &trace) != WZ_RESULT_OK ||
        !wz_host_pacing_init(&pacing, 1000u, WZ_SPEED_200, 0u, 0u) ||
        !wz_host_pacing_wait(&pacing, 0u, 1000u, 0, 0, &requested) ||
        !wz_host_audio_enabled(WZ_SPEED_200) || requested != 500000000u) {
        fputs("audio speed-boundary setup failed\n", stderr);
        exit(1);
    }
    before_tick = machine.master_tick;
    if (wz_headless_runner_advance(&runner, 8u) != WZ_RESULT_OK ||
        machine.master_tick <= before_tick ||
        !wz_host_pacing_set_speed(&pacing, WZ_SPEED_400) ||
        !wz_host_pacing_wait(&pacing, 1000000u, machine.master_tick,
                             0, 0, &requested) ||
        wz_host_audio_enabled(WZ_SPEED_400) || requested != 0u ||
        !wz_host_pacing_set_speed(&pacing, WZ_SPEED_50) ||
        !wz_host_pacing_wait(&pacing, 2000000u, machine.master_tick,
                             0, 0, &requested) ||
        !wz_host_audio_enabled(WZ_SPEED_50) || requested != 0u) {
        fputs("audio speed-boundary transition failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_tape_object_and_state(void)
{
    const wz_tape_segment_t segments[2u] = {{3u, 0u}, {5u, 1u}};
    const wz_tape_segment_t invalid[1u] = {{0u, 1u}};
    wz_tape_t tape;
    wz_tape_state_t state;

    if (wz_tape_mount(&tape, segments, 2u) != WZ_RESULT_OK ||
        wz_tape_mount(&tape, invalid, 1u) != WZ_RESULT_INVALID_ARGUMENT ||
        wz_tape_state_init(&state, &tape) != WZ_RESULT_OK ||
        wz_tape_state_ear_level(&state) != 0u ||
        wz_tape_state_set_motor(&state, true) != WZ_RESULT_OK ||
        wz_tape_state_advance(&state, 2u) != WZ_RESULT_OK ||
        wz_tape_state_ear_level(&state) != 0u ||
        wz_tape_state_advance(&state, 1u) != WZ_RESULT_OK ||
        wz_tape_state_ear_level(&state) != 1u ||
        wz_tape_state_set_motor(&state, false) != WZ_RESULT_OK ||
        wz_tape_state_advance(&state, 8u) != WZ_RESULT_OK ||
        wz_tape_state_at_end(&state) ||
        wz_tape_state_rewind(&state) != WZ_RESULT_OK ||
        wz_tape_state_set_motor(&state, true) != WZ_RESULT_OK ||
        wz_tape_state_advance(&state, 8u) != WZ_RESULT_OK ||
        !wz_tape_state_at_end(&state) || wz_tape_state_ear_level(&state) != 1u ||
        wz_tape_state_set_motor(&state, true) != WZ_RESULT_OK) {
        fputs("tape object/state contract failed\n", stderr);
        exit(1);
    }
}

static void test_machine_tape_playback(void)
{
    const wz_tape_segment_t segments[2u] = {{2u, 0u}, {3u, 1u}};
    wz_machine_t machine;
    wz_headless_runner_t runner;
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK ||
        wz_machine_set_tape_motor(&machine, true) != WZ_RESULT_INVALID_STATE ||
        wz_machine_mount_tape(&machine, segments, 2u) != WZ_RESULT_OK ||
        wz_machine_tape_ear_level(&machine) != 0u ||
        (wz_machine_ula_port_fe_read(&machine, 0xfeu) & 0x40u) != 0u ||
        wz_machine_set_tape_motor(&machine, true) != WZ_RESULT_OK ||
        wz_headless_runner_init(&runner, &machine, 0) != WZ_RESULT_OK ||
        wz_headless_runner_advance(&runner, 1u) != WZ_RESULT_OK ||
        wz_machine_tape_ear_level(&machine) != 0u ||
        wz_headless_runner_advance(&runner, 1u) != WZ_RESULT_OK ||
        wz_machine_tape_ear_level(&machine) != 1u ||
        (wz_machine_ula_port_fe_read(&machine, 0xfeu) & 0x40u) == 0u ||
        wz_machine_rewind_tape(&machine) != WZ_RESULT_OK ||
        wz_machine_tape_ear_level(&machine) != 0u ||
        wz_machine_unmount_tape(&machine) != WZ_RESULT_OK ||
        wz_machine_tape_ear_level(&machine) != 0u) {
        fputs("machine tape playback integration failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_tape_loading_mode_state(void)
{
    wz_machine_t machine;
    wz_tape_loading_mode_t mode;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_machine_tape_loading_mode(&machine) != WZ_TAPE_LOADING_NORMAL ||
        wz_machine_set_tape_loading_mode(&machine, WZ_TAPE_LOADING_INSTANT_TRAP) !=
            WZ_RESULT_OK ||
        wz_machine_tape_loading_mode(&machine) != WZ_TAPE_LOADING_INSTANT_TRAP) {
        fputs("tape loading mode transition failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    mode = wz_machine_tape_loading_mode(&machine);
    if (wz_machine_set_tape_loading_mode(&machine,
            (wz_tape_loading_mode_t)2u) != WZ_RESULT_INVALID_ARGUMENT ||
        wz_machine_tape_loading_mode(&machine) != mode ||
        wz_machine_tape_loading_mode(0) != WZ_TAPE_LOADING_NORMAL) {
        fputs("tape loading mode validation failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_networking_mode_state(void)
{
    static wz_machine_t machine;
    wz_networking_mode_t mode;
    static wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE];

    for (size_t index = 0u; index < sizeof(interface1_rom); ++index) {
        interface1_rom[index] = (wz_byte_t)(index ^ 0x5au);
    }

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_machine_networking_mode(&machine) != WZ_NETWORKING_NONE ||
        wz_machine_set_networking_mode(&machine, WZ_NETWORKING_INTERFACE1) !=
            WZ_RESULT_INVALID_STATE ||
        wz_machine_load_interface1_rom(&machine, interface1_rom,
            sizeof(interface1_rom), WZ_INTERFACE1_ROM_OLD) != WZ_RESULT_OK ||
        wz_machine_set_networking_mode(&machine, WZ_NETWORKING_INTERFACE1) !=
            WZ_RESULT_OK ||
        wz_machine_networking_mode(&machine) != WZ_NETWORKING_INTERFACE1) {
        fputs("networking mode transition failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    mode = wz_machine_networking_mode(&machine);
    if (wz_machine_set_networking_mode(&machine,
            (wz_networking_mode_t)3u) != WZ_RESULT_INVALID_ARGUMENT ||
        wz_machine_networking_mode(&machine) != mode ||
        wz_machine_networking_mode(0) != WZ_NETWORKING_NONE) {
        fputs("networking mode validation failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    if (wz_machine_set_networking_mode(&machine, WZ_NETWORKING_EAR_MIC) !=
            WZ_RESULT_UNSUPPORTED_OPERATION ||
        wz_machine_networking_mode(&machine) != mode) {
        fputs("reserved EAR_MIC mode was activated or mutated state\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

typedef struct {
    unsigned calls;
    wz_word_t block_id;
    size_t length;
    wz_byte_t first_byte;
    wz_result_t result;
} zxnet_write_capture_t;

static wz_result_t record_zxnet_write(wz_word_t block_id,
                                      const wz_byte_t* data,
                                      size_t length,
                                      void* context)
{
    zxnet_write_capture_t* capture = (zxnet_write_capture_t*)context;
    capture->calls += 1u;
    capture->block_id = block_id;
    capture->length = length;
    capture->first_byte = length == 0u ? 0u : data[0];
    return capture->result;
}

static void test_zxnet_state_machine(void)
{
    wz_zxnet_t network;
    wz_zxnet_snapshot_t snapshot;
    zxnet_write_capture_t capture = {0u, 0u, 0u, 0u, WZ_RESULT_OK};
    wz_byte_t value = 0u;
    wz_byte_t block[WZ_ZXNET_DATA_CAPACITY];

    for (size_t index = 0u; index < sizeof(block); ++index) {
        block[index] = (wz_byte_t)(index ^ 0x5au);
    }
    wz_zxnet_init(&network);
    if (wz_zxnet_read(&network, &value) != WZ_RESULT_OK || value != 30u ||
        wz_zxnet_begin_claim(&network, 0x42u) != WZ_RESULT_OK ||
        wz_zxnet_read(&network, &value) != WZ_RESULT_OK || value != 0x42u ||
        wz_zxnet_begin_claim(&network, 0x17u) != WZ_RESULT_OK ||
        wz_zxnet_accept_claim(&network, false) != WZ_RESULT_OK) {
        fputs("ZX Net claim state failed\n", stderr);
        return;
    }
    for (size_t count = 0u; count < WZ_ZXNET_DEFAULT_FREE_LENGTH; ++count) {
        if (wz_zxnet_read(&network, &value) != WZ_RESULT_OK || value != 30u) {
            fputs("ZX Net FREE count failed\n", stderr);
            return;
        }
    }
    if (wz_zxnet_begin_claim(&network, 0x21u) != WZ_RESULT_OK ||
        wz_zxnet_accept_claim(&network, true) != WZ_RESULT_OK ||
        wz_zxnet_snapshot(&network, &snapshot) != WZ_RESULT_OK ||
        snapshot.state != WZ_ZXNET_COLLWRITE) {
        fputs("ZX Net write collection state failed\n", stderr);
        return;
    }
    wz_zxnet_set_write_callback(&network, record_zxnet_write, &capture);
    for (size_t count = 0u; count < 9u; ++count) {
        if (wz_zxnet_write_bit(&network, (count & 1u) != 0u) != WZ_RESULT_OK) {
            fputs("ZX Net write bit collection failed\n", stderr);
            return;
        }
    }
    if (wz_zxnet_write_bit(&network, false) != WZ_RESULT_OK ||
        wz_zxnet_finish_write(&network) != WZ_RESULT_OK || capture.calls != 1u ||
        capture.block_id != 1u || capture.length != 1u ||
        wz_zxnet_snapshot(&network, &snapshot) != WZ_RESULT_OK ||
        snapshot.state != WZ_ZXNET_IDLE) {
        fputs("ZX Net write flush failed\n", stderr);
        return;
    }
    if (wz_zxnet_feed_read_block(&network, 2u, block, sizeof(block) - 1u) !=
            WZ_RESULT_INVALID_ARGUMENT ||
        wz_zxnet_feed_read_block(&network, 2u, block, sizeof(block)) != WZ_RESULT_OK ||
        wz_zxnet_read(&network, &value) != WZ_RESULT_OK || value != 30u ||
        wz_zxnet_snapshot(&network, &snapshot) != WZ_RESULT_OK ||
        snapshot.state != WZ_ZXNET_BUSY ||
        wz_zxnet_start_read_collection(&network) != WZ_RESULT_OK) {
        fputs("ZX Net read collection setup failed\n", stderr);
        return;
    }
    if (wz_zxnet_read(&network, &value) != WZ_RESULT_OK || value != block[0] ||
        wz_zxnet_snapshot(&network, &snapshot) != WZ_RESULT_OK ||
        snapshot.state != WZ_ZXNET_COLLREAD) {
        fputs("ZX Net collision read failed\n", stderr);
        return;
    }
    if (wz_zxnet_begin_claim(&network, 0x11u) != WZ_RESULT_INVALID_STATE) {
        fputs("ZX Net collection isolation failed\n", stderr);
    }
}

static void test_networking_mode_cold_reconfiguration(void)
{
    const wz_machine_profile_t* profiles[2u] = {
        wz_machine_profile_48k_pal(), wz_machine_profile_128k_pal()
    };

    for (size_t index = 0u; index < 2u; ++index) {
        static wz_machine_t machine;
        const wz_machine_profile_t* profile = profiles[index];
        static wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE];
        for (size_t rom_index = 0u; rom_index < sizeof(interface1_rom); ++rom_index) {
            interface1_rom[rom_index] = (wz_byte_t)(rom_index ^ 0xa5u);
        }
        wz_machine_init(&machine, profile);
        wz_machine_load_interface1_rom(&machine, interface1_rom,
            sizeof(interface1_rom), WZ_INTERFACE1_ROM_NEW);
        wz_machine_memory_write(&machine, 0x8000u, 0xa5u);
        machine.master_tick = 1234u;
        machine.paging_7ffd = 0x1fu;
        machine.networking_mode = WZ_NETWORKING_NONE;
        if (wz_machine_reconfigure_networking_mode(&machine,
                WZ_NETWORKING_INTERFACE1) != WZ_RESULT_OK ||
            machine.profile != profile ||
            wz_machine_networking_mode(&machine) != WZ_NETWORKING_INTERFACE1 ||
            wz_machine_memory_read(&machine, 0x8000u) != 0u ||
            machine.master_tick != 0u || machine.paging_7ffd != 0u) {
            fputs("cold networking reconfiguration did not reset context\n", stderr);
            wz_machine_destroy(&machine);
            exit(1);
        }
        if (wz_machine_reconfigure_networking_mode(&machine,
                WZ_NETWORKING_INTERFACE1) != WZ_RESULT_OK ||
            wz_machine_networking_mode(&machine) != WZ_NETWORKING_INTERFACE1) {
            fputs("same networking mode was not idempotent\n", stderr);
            wz_machine_destroy(&machine);
            exit(1);
        }
        wz_machine_memory_write(&machine, 0x8000u, 0x5au);
        machine.master_tick = 4321u;
        if (wz_machine_reconfigure_networking_mode(&machine,
                WZ_NETWORKING_EAR_MIC) != WZ_RESULT_UNSUPPORTED_OPERATION ||
            wz_machine_networking_mode(&machine) != WZ_NETWORKING_INTERFACE1 ||
            wz_machine_memory_read(&machine, 0x8000u) != 0x5au ||
            machine.master_tick != 4321u) {
            fputs("reserved networking mode mutated cold context\n", stderr);
            wz_machine_destroy(&machine);
            exit(1);
        }
        if (wz_machine_reconfigure_networking_mode(&machine,
                WZ_NETWORKING_NONE) != WZ_RESULT_OK ||
            wz_machine_networking_mode(&machine) != WZ_NETWORKING_NONE) {
            fputs("cold networking mode return transition failed\n", stderr);
            wz_machine_destroy(&machine);
            exit(1);
        }
        wz_machine_memory_write(&machine, 0x8000u, 0x3cu);
        machine.master_tick = 8765u;
        if (wz_machine_reconfigure_networking_mode(&machine,
                WZ_NETWORKING_EAR_MIC) != WZ_RESULT_UNSUPPORTED_OPERATION ||
            wz_machine_networking_mode(&machine) != WZ_NETWORKING_NONE ||
            wz_machine_memory_read(&machine, 0x8000u) != 0x3cu ||
            machine.master_tick != 8765u) {
            fputs("reserved networking mode mutated NONE context\n", stderr);
            wz_machine_destroy(&machine);
            exit(1);
        }
        wz_machine_destroy(&machine);
    }
}

static void test_interface1_rom_paging(void)
{
    static wz_machine_t machine;
    static wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE];

    for (size_t index = 0u; index < sizeof(interface1_rom); ++index) {
        interface1_rom[index] = (wz_byte_t)(index ^ 0x22u);
    }
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_machine_load_interface1_rom(&machine, interface1_rom,
            sizeof(interface1_rom), WZ_INTERFACE1_ROM_NEW) != WZ_RESULT_OK ||
        wz_machine_reconfigure_networking_mode(&machine, WZ_NETWORKING_INTERFACE1) !=
            WZ_RESULT_OK ||
        wz_machine_set_interface1_rom_page(&machine, 1u) != WZ_RESULT_OK ||
        wz_machine_memory_read(&machine, 0x0000u) != interface1_rom[0] ||
        wz_machine_memory_read(&machine, 0x1fffu) != interface1_rom[0x1fffu] ||
        wz_machine_memory_read(&machine, 0x2000u) != 0u) {
        fputs("Interface 1 shadow page mapping failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    if (wz_machine_reconfigure_networking_mode(&machine, WZ_NETWORKING_NONE) !=
            WZ_RESULT_OK ||
        wz_machine_memory_read(&machine, 0x0000u) != 0u ||
        wz_machine_interface1_rom_page(&machine) != 0u) {
        fputs("Interface 1 normal mapping restoration failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_interface1_machine_registers(void)
{
    static wz_machine_t machine;
    static wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE];
    wz_bus_request_t request;

    for (size_t index = 0u; index < sizeof(interface1_rom); ++index) {
        interface1_rom[index] = (wz_byte_t)(index ^ 0x71u);
    }
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_machine_load_interface1_rom(&machine, interface1_rom,
            sizeof(interface1_rom), WZ_INTERFACE1_ROM_NEW) != WZ_RESULT_OK ||
        wz_machine_reconfigure_networking_mode(&machine, WZ_NETWORKING_INTERFACE1) !=
            WZ_RESULT_OK ||
        wz_machine_interface1_control_latch(&machine) !=
            WZ_INTERFACE1_CONTROL_RESET ||
        wz_machine_interface1_previous_control_latch(&machine) !=
            WZ_INTERFACE1_CONTROL_RESET ||
        wz_machine_interface1_motor_shift(&machine) != 0xffu ||
        wz_machine_interface1_active_motor(&machine) != 0xffu ||
        wz_machine_interface1_control_latch_tick(&machine) != 0u ||
        !wz_machine_interface1_port_selected(0x20efu, 0xefu) ||
        wz_machine_interface1_port_selected(0x20eeu, 0xefu)) {
        fputs("Interface 1 register reset or alias state failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 400u, 0x20efu, 0xecu, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        request.source != WZ_BUS_SOURCE_INPUT ||
        wz_machine_interface1_control_latch(&machine) != 0xecu ||
        wz_machine_interface1_previous_control_latch(&machine) != 0xeeu ||
        wz_machine_interface1_motor_shift(&machine) != 0xfeu ||
        wz_machine_interface1_active_motor(&machine) != 0u ||
        wz_machine_interface1_control_latch_tick(&machine) != 400u) {
        fputs("Interface 1 control latch falling-edge state failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    if (wz_machine_interface1_control_write(&machine, 0xefu, 401u) != WZ_RESULT_OK ||
        wz_machine_interface1_control_write(&machine, 0xedu, 402u) != WZ_RESULT_OK ||
        wz_machine_interface1_motor_shift(&machine) != 0xfdu ||
        wz_machine_interface1_active_motor(&machine) != 1u ||
        wz_machine_interface1_previous_control_latch(&machine) != 0xefu) {
        fputs("Interface 1 repeated latch transition state failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

typedef struct {
    wz_result_t result;
    size_t sector;
    size_t length;
    wz_byte_t first_byte;
    unsigned int calls;
} microdrive_flush_test_context_t;

static wz_result_t record_microdrive_flush(size_t sector,
                                            const wz_byte_t* data,
                                            size_t length,
                                            void* context)
{
    microdrive_flush_test_context_t* record =
        (microdrive_flush_test_context_t*)context;
    record->calls++;
    record->sector = sector;
    record->length = length;
    record->first_byte = data[WZ_MDR_HEADER_OFFSET];
    return record->result;
}

static void test_microdrive_image_validation(void)
{
    static wz_byte_t image_data[WZ_MDR_MAX_SECTORS * WZ_MDR_SECTOR_SIZE];
    wz_mdr_image_t image = {0};
    wz_mdr_image_t original;
    wz_mdr_transport_t transport;
    microdrive_flush_test_context_t flush_record = {WZ_RESULT_PARSE_ERROR, 0u,
                                                     0u, 0u, 0u};
    const wz_byte_t* sector_data = 0;
    wz_byte_t value = 0u;

    for (size_t index = 0u; index < sizeof(image_data); ++index) {
        image_data[index] = (wz_byte_t)index;
    }
    if (wz_mdr_image_init(&image, image_data,
            WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE) != WZ_RESULT_OK ||
        image.sector_count != WZ_MDR_MIN_SECTORS ||
        wz_mdr_image_sector(&image, WZ_MDR_MIN_SECTORS - 1u, &sector_data) !=
            WZ_RESULT_OK ||
        sector_data != image_data + (WZ_MDR_MIN_SECTORS - 1u) * WZ_MDR_SECTOR_SIZE ||
        wz_mdr_image_init(&image, image_data,
            WZ_MDR_MAX_SECTORS * WZ_MDR_SECTOR_SIZE) != WZ_RESULT_OK ||
        image.sector_count != WZ_MDR_MAX_SECTORS) {
        fputs("MDR valid geometry or sector addressing failed\n", stderr);
        exit(1);
    }
    original = image;
    if (wz_mdr_image_init(&image, image_data, 8u * WZ_MDR_SECTOR_SIZE) !=
            WZ_RESULT_PARSE_ERROR ||
        memcmp(&image, &original, sizeof(image)) != 0 ||
        wz_mdr_image_init(&image, image_data,
            (WZ_MDR_MAX_SECTORS + 1u) * WZ_MDR_SECTOR_SIZE) !=
            WZ_RESULT_PARSE_ERROR ||
        memcmp(&image, &original, sizeof(image)) != 0 ||
        wz_mdr_image_init(&image, image_data,
            WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE - 1u) !=
            WZ_RESULT_PARSE_ERROR ||
        memcmp(&image, &original, sizeof(image)) != 0 ||
        wz_mdr_image_sector(&image, WZ_MDR_MAX_SECTORS, &sector_data) !=
            WZ_RESULT_INVALID_STATE) {
        fputs("MDR malformed-input or bounds handling failed\n", stderr);
        exit(1);
    }
    image_data[WZ_MDR_HEADER_OFFSET] = 0xa1u;
    image_data[WZ_MDR_DATA_OFFSET] = 0xb1u;
    image_data[WZ_MDR_SECTOR_SIZE + WZ_MDR_HEADER_OFFSET] = 0xa2u;
    if (wz_mdr_image_init(&image, image_data,
            WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE) != WZ_RESULT_OK) {
        fputs("MDR transport image setup failed\n", stderr);
        exit(1);
    }
    wz_mdr_transport_init(&transport);
    if (wz_mdr_transport_mount(&transport, &image) != WZ_RESULT_OK ||
        wz_mdr_transport_select_motor(&transport, 0u) != WZ_RESULT_OK ||
        wz_mdr_transport_read(&transport, &value) != WZ_RESULT_OK ||
        value != 0xa1u) {
        fputs("MDR header visibility failed\n", stderr);
        exit(1);
    }
    for (size_t index = 1u; index < WZ_MDR_HEADER_SIZE; ++index) {
        if (wz_mdr_transport_read(&transport, &value) != WZ_RESULT_OK) {
            fputs("MDR header traversal failed\n", stderr);
            exit(1);
        }
    }
    if (wz_mdr_transport_read(&transport, &value) != WZ_RESULT_OK ||
        value != 0xb1u) {
        fputs("MDR data visibility failed\n", stderr);
        exit(1);
    }
    for (size_t index = 1u; index < WZ_MDR_DATA_SIZE; ++index) {
        if (wz_mdr_transport_read(&transport, &value) != WZ_RESULT_OK) {
            fputs("MDR data traversal failed\n", stderr);
            exit(1);
        }
    }
    if (wz_mdr_transport_read(&transport, &value) != WZ_RESULT_OK ||
        value != 0xa2u) {
        fputs("MDR sector wrap failed\n", stderr);
        exit(1);
    }
    if (wz_mdr_transport_set_write_mode(&transport, 1u) != WZ_RESULT_OK ||
        wz_mdr_transport_write(&transport, 0x55u) != WZ_RESULT_OK ||
        wz_mdr_transport_select_motor(&transport, 0u) != WZ_RESULT_OK ||
        wz_mdr_transport_set_erase(&transport, 1u) != WZ_RESULT_OK ||
        wz_mdr_transport_write(&transport, 0x55u) != WZ_RESULT_INVALID_STATE ||
        wz_mdr_transport_set_erase(&transport, 0u) != WZ_RESULT_OK ||
        wz_mdr_transport_write(&transport, 0x55u) != WZ_RESULT_OK ||
        wz_mdr_transport_is_dirty(&transport) != 1u ||
        image_data[WZ_MDR_SECTOR_SIZE + WZ_MDR_HEADER_OFFSET] != 0xa2u) {
        fputs("MDR protected/buffered write behavior failed\n", stderr);
        exit(1);
    }
    wz_mdr_transport_discard(&transport);
    if (wz_mdr_transport_is_dirty(&transport) != 0u) {
        fputs("MDR write discard failed\n", stderr);
        exit(1);
    }
    if (wz_mdr_transport_set_write_mode(&transport, 1u) != WZ_RESULT_OK ||
        wz_mdr_transport_write(&transport, 0x66u) != WZ_RESULT_OK ||
        wz_mdr_transport_flush(&transport, record_microdrive_flush,
                                &flush_record) != WZ_RESULT_PARSE_ERROR ||
        flush_record.calls != 1u || flush_record.sector != 1u ||
        flush_record.length != WZ_MDR_SECTOR_SIZE ||
        flush_record.first_byte != 0x66u ||
        wz_mdr_transport_is_dirty(&transport) != 1u) {
        fputs("MDR failed flush recovery failed\n", stderr);
        exit(1);
    }
    flush_record.result = WZ_RESULT_OK;
    if (wz_mdr_transport_flush(&transport, record_microdrive_flush,
                               &flush_record) != WZ_RESULT_OK ||
        flush_record.calls != 2u || wz_mdr_transport_is_dirty(&transport) != 0u) {
        fputs("MDR successful flush commit failed\n", stderr);
        exit(1);
    }
}

static void test_historical_state_representability(void)
{
    wz_machine_t machine;
    wz_tape_loading_mode_t tape_mode;
    wz_networking_mode_t networking_mode;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_state_validate_historical_representability(
            &machine, WZ_HISTORICAL_FORMAT_SNA) != WZ_RESULT_OK ||
        wz_state_validate_historical_representability(
            &machine, WZ_HISTORICAL_FORMAT_Z80) != WZ_RESULT_OK ||
        wz_state_validate_historical_representability(
            &machine, (wz_historical_state_format_t)2u) !=
            WZ_RESULT_INVALID_ARGUMENT ||
        wz_state_validate_historical_representability(0,
            WZ_HISTORICAL_FORMAT_SNA) != WZ_RESULT_INVALID_ARGUMENT) {
        fputs("historical state baseline validation failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    tape_mode = machine.tape_loading_mode;
    networking_mode = machine.networking_mode;
    machine.tape_loading_mode = WZ_TAPE_LOADING_INSTANT_TRAP;
    if (wz_state_validate_historical_representability(
            &machine, WZ_HISTORICAL_FORMAT_SNA) !=
            WZ_RESULT_UNSUPPORTED_OPERATION ||
        machine.tape_loading_mode != WZ_TAPE_LOADING_INSTANT_TRAP) {
        fputs("historical tape state was not rejected\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    machine.tape_loading_mode = tape_mode;
    machine.networking_mode = WZ_NETWORKING_INTERFACE1;
    if (wz_state_validate_historical_representability(
            &machine, WZ_HISTORICAL_FORMAT_Z80) !=
            WZ_RESULT_UNSUPPORTED_OPERATION ||
        machine.networking_mode != WZ_NETWORKING_INTERFACE1) {
        fputs("historical networking state was not rejected\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    machine.networking_mode = networking_mode;
    wz_machine_destroy(&machine);
}

static void test_snapshot_state_isolated_validation(void)
{
    static wz_machine_t machine;
    static wz_snapshot_state_t snapshot;
    static wz_snapshot_state_t original;
    static wz_byte_t serialized[WZ_STATE_SNAPSHOT_CAPACITY];
    wz_state_writer_t writer;
    wz_qword_t before_hash;
    wz_qword_t after_hash;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_state_hash_machine(&machine, &before_hash) != WZ_RESULT_OK) {
        fputs("snapshot state setup failed\n", stderr);
        exit(1);
    }
    wz_snapshot_state_init(&snapshot);
    wz_state_writer_init(&writer, serialized, sizeof(serialized));
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK ||
        wz_snapshot_state_load(&snapshot, serialized, writer.length) != WZ_RESULT_OK ||
        wz_snapshot_state_length(&snapshot) != writer.length ||
        wz_snapshot_state_data(&snapshot) == 0) {
        fputs("snapshot state load failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    original = snapshot;
    serialized[0u] = 0u;
    if (wz_snapshot_state_load(&snapshot, serialized, writer.length) !=
            WZ_RESULT_INVALID_STATE ||
        memcmp(&snapshot, &original, sizeof(snapshot)) != 0 ||
        wz_state_hash_machine(&machine, &after_hash) != WZ_RESULT_OK ||
        before_hash != after_hash) {
        fputs("snapshot state validation was not isolated\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    if (wz_snapshot_state_load(&snapshot, 0, writer.length) !=
            WZ_RESULT_INVALID_ARGUMENT ||
        wz_snapshot_state_length(0) != 0u ||
        wz_snapshot_state_data(0) != 0) {
        fputs("snapshot state argument validation failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_sna_48k_loader(void)
{
    static wz_byte_t sna[WZ_SNA_48K_LENGTH];
    static wz_machine_t machine;
    static wz_snapshot_state_t snapshot;
    static wz_snapshot_state_t original;
    wz_qword_t before_hash;
    wz_qword_t after_hash;

    memset(sna, 0, sizeof(sna));
    sna[0u] = 0x3au;
    sna[1u] = 0x12u;
    sna[2u] = 0x34u;
    sna[15u] = 0x78u;
    sna[16u] = 0x56u;
    sna[19u] = 4u;
    sna[20u] = 0x9au;
    sna[21u] = 0x5au;
    sna[22u] = 0xa5u;
    sna[23u] = 0xfeu;
    sna[24u] = 0xffu;
    sna[25u] = 2u;
    sna[26u] = 6u;
    sna[27u + 0xfffeu - 0x4000u] = 0x21u;
    sna[27u + 0xffffu - 0x4000u] = 0x43u;
    sna[27u] = 0x43u;
    sna[28u] = 0x65u;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_state_hash_machine(&machine, &before_hash) != WZ_RESULT_OK) {
        fputs("SNA loader setup failed\n", stderr);
        exit(1);
    }
    wz_snapshot_state_init(&snapshot);
    if (wz_snapshot_state_load_sna_48k(&snapshot, sna, sizeof(sna)) != WZ_RESULT_OK) {
        fputs("SNA 48K valid image rejected\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    if (wz_snapshot_state_length(&snapshot) != WZ_STATE_SNAPSHOT_CAPACITY ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0x4000u] != 0x43u ||
        wz_snapshot_state_data(&snapshot)[31u] != 0x00u ||
        wz_snapshot_state_data(&snapshot)[32u] != 0x00u ||
        wz_snapshot_state_data(&snapshot)[33u] != 0x21u ||
        wz_snapshot_state_data(&snapshot)[34u] != 0x43u ||
        wz_snapshot_state_data(&snapshot)[29u] != 0x78u ||
        wz_snapshot_state_data(&snapshot)[30u] != 0x56u) {
        fputs("SNA 48K register or RAM mapping failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    original = snapshot;
    sna[25u] = 3u;
    if (wz_snapshot_state_load_sna_48k(&snapshot, sna, sizeof(sna)) !=
            WZ_RESULT_INVALID_STATE ||
        memcmp(&snapshot, &original, sizeof(snapshot)) != 0 ||
        wz_state_hash_machine(&machine, &after_hash) != WZ_RESULT_OK ||
        before_hash != after_hash) {
        fputs("SNA 48K malformed image was published\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    if (wz_snapshot_state_load_sna_48k(&snapshot, sna, sizeof(sna) - 1u) !=
            WZ_RESULT_INVALID_STATE ||
        wz_snapshot_state_load_sna_48k(0, sna, sizeof(sna)) !=
            WZ_RESULT_INVALID_ARGUMENT) {
        fputs("SNA 48K argument validation failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_host_media_ownership(void)
{
    const char* path = "wz-host-media-ownership.tmp";
    FILE* file = fopen(path, "wb");
    wz_host_media_claim_t first = {0};
    wz_host_media_claim_t second = {0};
    wz_host_media_claim_t read_only = {0};
    bool success = false;

    if (file == 0) {
        fputs("media ownership fixture creation failed\n", stderr);
        return;
    }
    fclose(file);
    if (!wz_host_media_claim_acquire(path, true, &first) ||
        wz_host_media_claim_acquire(path, true, &second) ||
        !wz_host_media_claim_acquire(path, false, &read_only) ||
        read_only.held || read_only.writable) {
        fputs("media ownership claim policy failed\n", stderr);
        goto cleanup;
    }
    wz_host_media_claim_release(&first);
    if (!wz_host_media_claim_acquire(path, true, &second)) {
        fputs("media ownership release failed\n", stderr);
        goto cleanup;
    }
    success = true;

cleanup:
    wz_host_media_claim_release(&first);
    wz_host_media_claim_release(&second);
    wz_host_media_claim_release(&read_only);
    remove(path);
    if (!success) {
        return;
    }
}

static void test_sna_48k_writer(void)
{
    static wz_byte_t sna[WZ_SNA_48K_LENGTH];
    static wz_byte_t original[WZ_SNA_48K_LENGTH];
    static wz_machine_t machine;
    static wz_snapshot_state_t snapshot;
    wz_qword_t before_hash;
    wz_qword_t after_hash;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        fputs("SNA writer setup failed\n", stderr);
        exit(1);
    }
    machine.cpu.program_counter = 0x4321u;
    machine.cpu.stack_pointer = 0xfffeu;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = WZ_Z80_INTERRUPT_MODE_2;
    machine.border_color = 6u;
    machine.memory[0x4000u] = 0x5au;
    memset(sna, 0xa5, sizeof(sna));
    if (wz_state_hash_machine(&machine, &before_hash) != WZ_RESULT_OK ||
        wz_state_save_sna_48k(&machine, sna, sizeof(sna)) != WZ_RESULT_OK ||
        sna[19u] != 4u || sna[23u] != 0xfcu || sna[24u] != 0xffu ||
        sna[25u] != 2u || sna[26u] != 6u || sna[27u] != 0x5au ||
        sna[27u + 0xbffcu] != 0x21u || sna[27u + 0xbffdu] != 0x43u ||
        wz_state_hash_machine(&machine, &after_hash) != WZ_RESULT_OK ||
        before_hash != after_hash) {
        fputs("SNA writer encoding or immutability failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    memcpy(original, sna, sizeof(sna));
    wz_snapshot_state_init(&snapshot);
    if (wz_snapshot_state_load_sna_48k(&snapshot, sna, sizeof(sna)) != WZ_RESULT_OK ||
        wz_snapshot_state_data(&snapshot)[33u] != 0x21u ||
        wz_snapshot_state_data(&snapshot)[34u] != 0x43u) {
        fputs("SNA writer round-trip failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    machine.networking_mode = WZ_NETWORKING_INTERFACE1;
    if (wz_state_save_sna_48k(&machine, sna, sizeof(sna)) !=
            WZ_RESULT_UNSUPPORTED_OPERATION ||
        memcmp(sna, original, sizeof(sna)) != 0 ||
        wz_state_save_sna_48k(&machine, sna, sizeof(sna) - 1u) !=
            WZ_RESULT_BUFFER_TOO_SMALL) {
        fputs("SNA writer rejection was not atomic\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_sna_128k_image_scaffolding(void)
{
    static wz_sna_128k_image_t image;
    static wz_sna_128k_image_t decoded;
    static wz_sna_128k_image_t original_image;
    static wz_byte_t data[WZ_SNA_128K_LENGTH];
    static wz_byte_t original[WZ_SNA_128K_LENGTH];

    if (wz_sna_128k_image_init(&image) != WZ_RESULT_OK ||
        wz_sna_128k_image_init(&decoded) != WZ_RESULT_OK) {
        fputs("SNA 128K image initialization failed\n", stderr);
        exit(1);
    }
    for (size_t index = 0u; index < sizeof(image.header); ++index) {
        image.header[index] = (wz_byte_t)(index + 1u);
    }
    image.program_counter = 0x4321u;
    image.paging_7ffd = 0x03u;
    image.trdos_active = 1u;
    for (wz_byte_t page = 0u; page < WZ_SNA_128K_PAGE_COUNT; ++page) {
        image.page_present[page] = 1u;
        for (size_t index = 0u; index < WZ_SNA_128K_PAGE_SIZE; ++index) {
            image.pages[page][index] = (wz_byte_t)(page * 17u + index);
        }
    }
    if (wz_sna_128k_image_save(&image, data, sizeof(data)) != WZ_RESULT_OK ||
        wz_sna_128k_image_load(&decoded, data, sizeof(data)) != WZ_RESULT_OK ||
        memcmp(&image, &decoded, sizeof(image)) != 0) {
        fputs("SNA 128K image round-trip failed\n", stderr);
        exit(1);
    }
    original_image = decoded;
    memcpy(original, data, sizeof(data));
    data[49181u] = 0x05u;
    if (wz_sna_128k_image_load(&decoded, data, sizeof(data)) != WZ_RESULT_PARSE_ERROR ||
        memcmp(&decoded, &original_image, sizeof(decoded)) != 0) {
        fputs("SNA 128K malformed image was not rejected\n", stderr);
        exit(1);
    }
    memcpy(data, original, sizeof(data));
    if (wz_sna_128k_image_save(&image, data, WZ_SNA_128K_LENGTH - 1u) !=
            WZ_RESULT_BUFFER_TOO_SMALL) {
        fputs("SNA 128K output capacity was not enforced\n", stderr);
        exit(1);
    }
}

static void test_sna_128k_machine_round_trip(void)
{
    static wz_machine_t source;
    static wz_machine_t target;
    static wz_byte_t sna[WZ_SNA_128K_LENGTH];
    static wz_byte_t invalid[WZ_SNA_128K_LENGTH];

    if (wz_machine_init(&source, wz_machine_profile_128k_pal()) != WZ_RESULT_OK ||
        wz_machine_init(&target, wz_machine_profile_128k_pal()) != WZ_RESULT_OK) {
        fputs("128K SNA machine setup failed\n", stderr);
        exit(1);
    }
    for (wz_byte_t page = 0u; page < WZ_128K_RAM_BANK_COUNT; ++page) {
        for (size_t index = 0u; index < WZ_128K_RAM_BANK_SIZE; ++index) {
            source.ram_128k[page * WZ_128K_RAM_BANK_SIZE + index] =
                (wz_byte_t)(page * 17u + index);
        }
    }
    source.cpu.main.a = 0x42u;
    source.cpu.main.f = 0x24u;
    source.cpu.alternate.a = 0x81u;
    source.cpu.stack_pointer = 0x8123u;
    source.cpu.program_counter = 0x4567u;
    source.cpu.interrupt_mode = WZ_Z80_INTERRUPT_MODE_2;
    source.cpu.iff1 = 1u;
    source.cpu.iff2 = 1u;
    source.border_color = 6u;
    if (wz_machine_128k_paging_write(&source, 0x7ffdu, 0x2bu) != WZ_RESULT_OK ||
        wz_state_save_sna_128k(&source, sna, sizeof(sna)) != WZ_RESULT_OK ||
        wz_state_load_sna_128k(&target, sna, sizeof(sna)) != WZ_RESULT_OK ||
        target.cpu.main.a != source.cpu.main.a ||
        target.cpu.main.f != source.cpu.main.f ||
        target.cpu.alternate.a != source.cpu.alternate.a ||
        target.cpu.stack_pointer != source.cpu.stack_pointer ||
        target.cpu.program_counter != source.cpu.program_counter ||
        target.cpu.interrupt_mode != source.cpu.interrupt_mode ||
        target.cpu.iff1 != source.cpu.iff1 || target.cpu.iff2 != source.cpu.iff2 ||
        target.border_color != source.border_color ||
        target.paging_7ffd != source.paging_7ffd ||
        target.paging_7ffd_locked != source.paging_7ffd_locked) {
        fputs("128K SNA machine state round trip failed\n", stderr);
        wz_machine_destroy(&source);
        wz_machine_destroy(&target);
        exit(1);
    }
    for (wz_byte_t page = 0u; page < WZ_128K_RAM_BANK_COUNT; ++page) {
        if (memcmp(target.ram_128k + page * WZ_128K_RAM_BANK_SIZE,
                   source.ram_128k + page * WZ_128K_RAM_BANK_SIZE,
                   WZ_128K_RAM_BANK_SIZE) != 0) {
            fputs("128K SNA RAM bank round trip failed\n", stderr);
            wz_machine_destroy(&source);
            wz_machine_destroy(&target);
            exit(1);
        }
    }
    if (wz_machine_memory_read(&target, 0xc000u) !=
            wz_machine_memory_read(&source, 0xc000u)) {
        fputs("128K SNA visible bank mapping failed\n", stderr);
        wz_machine_destroy(&source);
        wz_machine_destroy(&target);
        exit(1);
    }
    memcpy(invalid, sna, sizeof(invalid));
    invalid[49181u] = 0x02u;
    target.cpu.main.a = 0x99u;
    if (wz_state_load_sna_128k(&target, invalid, sizeof(invalid)) == WZ_RESULT_OK ||
        target.cpu.main.a != 0x99u || target.paging_7ffd != 0x2bu ||
        wz_state_save_sna_128k(&source, sna, sizeof(sna) - 1u) !=
            WZ_RESULT_BUFFER_TOO_SMALL) {
        fputs("128K SNA rejection atomicity failed\n", stderr);
        wz_machine_destroy(&source);
        wz_machine_destroy(&target);
        exit(1);
    }
    wz_machine_destroy(&source);
    wz_machine_destroy(&target);
}

static void test_z80_128k_machine_round_trip(void)
{
    static wz_machine_t source;
    static wz_machine_t target;
    static wz_byte_t z80[WZ_Z80_128K_V2_LENGTH];
    static wz_byte_t invalid[WZ_Z80_128K_V2_LENGTH];

    if (wz_machine_init(&source, wz_machine_profile_128k_pal()) != WZ_RESULT_OK ||
        wz_machine_init(&target, wz_machine_profile_128k_pal()) != WZ_RESULT_OK) {
        fputs("128K Z80 machine setup failed\n", stderr);
        exit(1);
    }
    for (wz_byte_t page = 0u; page < WZ_128K_RAM_BANK_COUNT; ++page) {
        for (size_t index = 0u; index < WZ_128K_RAM_BANK_SIZE; ++index) {
            source.ram_128k[page * WZ_128K_RAM_BANK_SIZE + index] =
                (wz_byte_t)(page * 29u + index);
        }
    }
    source.cpu.main.a = 0x42u;
    source.cpu.main.f = 0x24u;
    source.cpu.alternate.a = 0x81u;
    source.cpu.stack_pointer = 0x8123u;
    source.cpu.program_counter = 0x4567u;
    source.cpu.interrupt_mode = WZ_Z80_INTERRUPT_MODE_2;
    source.cpu.iff1 = 1u;
    source.cpu.iff2 = 1u;
    source.border_color = 6u;
    source.ay.selected_register = 13u;
    source.ay.registers[13u] = 0x0bu;
    if (wz_machine_128k_paging_write(&source, 0x7ffdu, 0x2bu) != WZ_RESULT_OK ||
        wz_state_save_z80_v2_128k(&source, z80, sizeof(z80)) != WZ_RESULT_OK ||
        wz_state_load_z80_v2_128k(&target, z80, sizeof(z80)) != WZ_RESULT_OK ||
        target.cpu.main.a != source.cpu.main.a ||
        target.cpu.main.f != source.cpu.main.f ||
        target.cpu.alternate.a != source.cpu.alternate.a ||
        target.cpu.stack_pointer != source.cpu.stack_pointer ||
        target.cpu.program_counter != source.cpu.program_counter ||
        target.cpu.interrupt_mode != source.cpu.interrupt_mode ||
        target.cpu.iff1 != source.cpu.iff1 || target.cpu.iff2 != source.cpu.iff2 ||
        target.border_color != source.border_color ||
        target.paging_7ffd != source.paging_7ffd ||
        target.paging_7ffd_locked != source.paging_7ffd_locked ||
        target.ay.selected_register != source.ay.selected_register ||
        target.ay.registers[13u] != source.ay.registers[13u]) {
        fputs("128K Z80 machine state round trip failed\n", stderr);
        wz_machine_destroy(&source);
        wz_machine_destroy(&target);
        exit(1);
    }
    for (wz_byte_t page = 0u; page < WZ_128K_RAM_BANK_COUNT; ++page) {
        if (memcmp(target.ram_128k + page * WZ_128K_RAM_BANK_SIZE,
                   source.ram_128k + page * WZ_128K_RAM_BANK_SIZE,
                   WZ_128K_RAM_BANK_SIZE) != 0) {
            fputs("128K Z80 RAM bank round trip failed\n", stderr);
            wz_machine_destroy(&source);
            wz_machine_destroy(&target);
            exit(1);
        }
    }
    memcpy(invalid, z80, sizeof(invalid));
    invalid[WZ_Z80_V2_HEADER_LENGTH + 2u] = 10u;
    target.cpu.main.a = 0x99u;
    if (wz_state_load_z80_v2_128k(&target, invalid, sizeof(invalid)) == WZ_RESULT_OK ||
        target.cpu.main.a != 0x99u ||
        wz_state_save_z80_v2_128k(&source, z80, sizeof(z80) - 1u) !=
            WZ_RESULT_BUFFER_TOO_SMALL) {
        fputs("128K Z80 rejection atomicity failed\n", stderr);
        wz_machine_destroy(&source);
        wz_machine_destroy(&target);
        exit(1);
    }
    wz_machine_destroy(&source);
    wz_machine_destroy(&target);
}

static void test_z80_v1_loader(void)
{
    static wz_byte_t z80[WZ_Z80_V1_HEADER_LENGTH + WZ_Z80_V1_MEMORY_LENGTH + 4u];
    static wz_snapshot_state_t snapshot;
    static wz_snapshot_state_t original;

    memset(z80, 0, sizeof(z80));
    z80[0u] = 0x12u;
    z80[1u] = 0x34u;
    z80[6u] = 0x21u;
    z80[7u] = 0x43u;
    z80[8u] = 0x78u;
    z80[9u] = 0x56u;
    z80[10u] = 0x9au;
    z80[11u] = 0x55u;
    z80[12u] = 0x25u;
    z80[23u] = 0x34u;
    z80[24u] = 0x12u;
    z80[25u] = 0x78u;
    z80[26u] = 0x56u;
    z80[27u] = 1u;
    z80[28u] = 1u;
    z80[29u] = 2u;
    for (size_t index = WZ_Z80_V1_HEADER_LENGTH;
         index < WZ_Z80_V1_HEADER_LENGTH + WZ_Z80_V1_MEMORY_LENGTH; ++index) {
        z80[index] = (wz_byte_t)(index * 3u);
    }
    z80[12u] |= 0x20u;
    z80[WZ_Z80_V1_HEADER_LENGTH + WZ_Z80_V1_MEMORY_LENGTH] = 0u;
    z80[WZ_Z80_V1_HEADER_LENGTH + WZ_Z80_V1_MEMORY_LENGTH + 1u] = 0xedu;
    z80[WZ_Z80_V1_HEADER_LENGTH + WZ_Z80_V1_MEMORY_LENGTH + 2u] = 0xedu;
    z80[WZ_Z80_V1_HEADER_LENGTH + WZ_Z80_V1_MEMORY_LENGTH + 3u] = 0u;
    wz_snapshot_state_init(&snapshot);
    if (wz_snapshot_state_load_z80_v1(&snapshot, z80, sizeof(z80)) != WZ_RESULT_OK ||
        wz_snapshot_state_data(&snapshot)[33u] != 0x21u ||
        wz_snapshot_state_data(&snapshot)[34u] != 0x43u ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0x4000u] !=
            (wz_byte_t)(WZ_Z80_V1_HEADER_LENGTH * 3u)) {
        fputs("Z80 v1 compressed load failed\n", stderr);
        exit(1);
    }
    original = snapshot;
    z80[sizeof(z80) - 1u] = 1u;
    if (wz_snapshot_state_load_z80_v1(&snapshot, z80, sizeof(z80)) !=
            WZ_RESULT_PARSE_ERROR ||
        memcmp(&snapshot, &original, sizeof(snapshot)) != 0) {
        fputs("Z80 v1 missing end marker was published\n", stderr);
        exit(1);
    }
}

static void test_z80_v2_loader_and_writer(void)
{
    static wz_byte_t z80[WZ_Z80_V2_LENGTH];
    static wz_byte_t output[WZ_Z80_V2_LENGTH];
    static wz_byte_t original_output[WZ_Z80_V2_LENGTH];
    static wz_snapshot_state_t snapshot;
    static wz_snapshot_state_t original_snapshot;
    wz_machine_t machine;
    size_t offset;

    memset(z80, 0, sizeof(z80));
    z80[8u] = 0x78u;
    z80[9u] = 0x56u;
    z80[27u] = 1u;
    z80[28u] = 1u;
    z80[29u] = 2u;
    z80[30u] = 23u;
    z80[32u] = 0x21u;
    z80[33u] = 0x43u;
    wz_write_le16(z80 + WZ_Z80_V2_HEADER_LENGTH, 260u);
    z80[WZ_Z80_V2_HEADER_LENGTH + 2u] = 8u;
    offset = WZ_Z80_V2_HEADER_LENGTH + 3u;
    for (size_t run = 0u; run < 64u; ++run) {
        z80[offset++] = 0xedu;
        z80[offset++] = 0xedu;
        z80[offset++] = 0xffu;
        z80[offset++] = 0xa5u;
    }
    z80[offset++] = 0xedu;
    z80[offset++] = 0xedu;
    z80[offset++] = 64u;
    z80[offset++] = 0xa5u;
    {
        size_t page4 = WZ_Z80_V2_HEADER_LENGTH + 263u;
        wz_write_le16(z80 + page4, WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH);
        z80[page4 + 2u] = 4u;
        for (size_t index = 0u; index < WZ_Z80_V2_PAGE_SIZE; ++index) {
            z80[page4 + 3u + index] = (wz_byte_t)index;
        }
        page4 += 3u + WZ_Z80_V2_PAGE_SIZE;
        wz_write_le16(z80 + page4, WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH);
        z80[page4 + 2u] = 5u;
        for (size_t index = 0u; index < WZ_Z80_V2_PAGE_SIZE; ++index) {
            z80[page4 + 3u + index] = (wz_byte_t)(index ^ 0x5au);
        }
        offset = page4 + 3u + WZ_Z80_V2_PAGE_SIZE;
    }
    wz_snapshot_state_init(&snapshot);
    if (wz_snapshot_state_load_z80_v2(&snapshot, z80, offset) != WZ_RESULT_OK ||
        wz_snapshot_state_data(&snapshot)[33u] != 0x21u ||
        wz_snapshot_state_data(&snapshot)[34u] != 0x43u ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0x4000u] != 0xa5u ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0x8000u] != 0u ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0xc000u] != 0x5au) {
        fputs("Z80 v2 load or page mapping failed\n", stderr);
        exit(1);
    }
    original_snapshot = snapshot;
    z80[WZ_Z80_V2_HEADER_LENGTH + 2u] = 4u;
    if (wz_snapshot_state_load_z80_v2(&snapshot, z80, offset) !=
            WZ_RESULT_PARSE_ERROR ||
        memcmp(&snapshot, &original_snapshot, sizeof(snapshot)) != 0) {
        fputs("Z80 v2 duplicate page was published\n", stderr);
        exit(1);
    }

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        fputs("Z80 v2 writer setup failed\n", stderr);
        exit(1);
    }
    machine.cpu.program_counter = 0x4321u;
    machine.cpu.stack_pointer = 0x8765u;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = WZ_Z80_INTERRUPT_MODE_1;
    machine.border_color = 6u;
    machine.memory[0x4000u] = 0x11u;
    machine.memory[0x8000u] = 0x22u;
    machine.memory[0xc000u] = 0x33u;
    memset(output, 0x5au, sizeof(output));
    if (wz_state_save_z80_v2_48k(&machine, output, sizeof(output)) != WZ_RESULT_OK ||
        output[6u] != 0u || output[7u] != 0u || output[30u] != 23u ||
        output[32u] != 0x21u || output[33u] != 0x43u || output[57u] != 8u ||
        wz_snapshot_state_load_z80_v2(&snapshot, output, sizeof(output)) !=
            WZ_RESULT_OK || wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0x4000u] != 0x11u ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0x8000u] != 0x22u ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0xc000u] != 0x33u) {
        fputs("Z80 v2 canonical writer failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    memcpy(original_output, output, sizeof(output));
    machine.networking_mode = WZ_NETWORKING_EAR_MIC;
    if (wz_state_save_z80_v2_48k(&machine, output, sizeof(output)) !=
            WZ_RESULT_UNSUPPORTED_OPERATION ||
        memcmp(output, original_output, sizeof(output)) != 0 ||
        wz_state_save_z80_v2_48k(&machine, output, sizeof(output) - 1u) !=
            WZ_RESULT_BUFFER_TOO_SMALL) {
        fputs("Z80 v2 writer rejection was not atomic\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_z80_v3_loader(void)
{
    static wz_byte_t z80[WZ_Z80_V3_HEADER_LENGTH +
                         3u * (3u + WZ_Z80_V2_PAGE_SIZE) + 1u];
    static wz_snapshot_state_t snapshot;
    static wz_snapshot_state_t original_snapshot;
    size_t offset = WZ_Z80_V3_HEADER_LENGTH;
    size_t length;

    memset(z80, 0, sizeof(z80));
    z80[8u] = 0x78u;
    z80[9u] = 0x56u;
    z80[27u] = 1u;
    z80[28u] = 1u;
    z80[29u] = 2u;
    wz_write_le16(z80 + 30u, 54u);
    z80[32u] = 0x21u;
    z80[33u] = 0x43u;
    for (size_t page = 0u; page < WZ_Z80_V2_PAGE_COUNT; ++page) {
        const wz_byte_t page_numbers[WZ_Z80_V2_PAGE_COUNT] = {8u, 4u, 5u};
        wz_write_le16(z80 + offset, WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH);
        z80[offset + 2u] = page_numbers[page];
        for (size_t index = 0u; index < WZ_Z80_V2_PAGE_SIZE; ++index) {
            z80[offset + 3u + index] = (wz_byte_t)(page * 0x40u + index);
        }
        offset += 3u + WZ_Z80_V2_PAGE_SIZE;
    }
    length = offset;
    wz_snapshot_state_init(&snapshot);
    if (wz_snapshot_state_load_z80_v3(&snapshot, z80, length) != WZ_RESULT_OK ||
        wz_snapshot_state_data(&snapshot)[33u] != 0x21u ||
        wz_snapshot_state_data(&snapshot)[34u] != 0x43u ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0x4000u] != 0u ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0x8000u] != 0x40u ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0xc000u] != 0x80u) {
        fputs("Z80 v3 load or page mapping failed\n", stderr);
        exit(1);
    }
    original_snapshot = snapshot;
    z80[37u] = 1u;
    if (wz_snapshot_state_load_z80_v3(&snapshot, z80, length) !=
            WZ_RESULT_PARSE_ERROR ||
        memcmp(&snapshot, &original_snapshot, sizeof(snapshot)) != 0) {
        fputs("Z80 v3 device state was published\n", stderr);
        exit(1);
    }
    z80[37u] = 0u;
    memmove(z80 + WZ_Z80_V3_HEADER_LENGTH_EXTENDED,
            z80 + WZ_Z80_V3_HEADER_LENGTH,
            length - WZ_Z80_V3_HEADER_LENGTH);
    memset(z80 + WZ_Z80_V3_HEADER_LENGTH, 0, 1u);
    wz_write_le16(z80 + 30u, 55u);
    if (wz_snapshot_state_load_z80_v3(&snapshot, z80,
                                      length + 1u) != WZ_RESULT_OK) {
        fputs("Z80 v3 extended header load failed\n", stderr);
        exit(1);
    }
}

static void test_snapshot_parser_fuzz_matrix(void)
{
    static wz_byte_t random_data[2048u];
    static wz_byte_t z80_v2[WZ_Z80_V2_LENGTH + 1u];
    static wz_byte_t z80_v3[WZ_Z80_V3_HEADER_LENGTH +
                            3u * (3u + WZ_Z80_V2_PAGE_SIZE)];
    static wz_snapshot_state_t snapshot;
    static wz_snapshot_state_t original_snapshot;
    static wz_sna_128k_image_t image;
    wz_dword_t seed = 0x6d2b79f5u;

    memset(z80_v2, 0, sizeof(z80_v2));
    z80_v2[27u] = 1u;
    z80_v2[28u] = 1u;
    z80_v2[29u] = 2u;
    z80_v2[30u] = 23u;
    z80_v2[32u] = 1u;
    for (size_t page = 0u; page < WZ_Z80_V2_PAGE_COUNT; ++page) {
        const wz_byte_t page_numbers[WZ_Z80_V2_PAGE_COUNT] = {8u, 4u, 5u};
        size_t offset = WZ_Z80_V2_HEADER_LENGTH +
                        page * (3u + WZ_Z80_V2_PAGE_SIZE);
        wz_write_le16(z80_v2 + offset, WZ_Z80_V2_UNCOMPRESSED_PAGE_LENGTH);
        z80_v2[offset + 2u] = page_numbers[page];
    }
    memcpy(z80_v3, z80_v2, WZ_Z80_V2_HEADER_LENGTH);
    memmove(z80_v3 + WZ_Z80_V3_HEADER_LENGTH,
            z80_v2 + WZ_Z80_V2_HEADER_LENGTH,
            WZ_Z80_V2_LENGTH - WZ_Z80_V2_HEADER_LENGTH);
    memset(z80_v3 + WZ_Z80_V2_HEADER_LENGTH,
           0, WZ_Z80_V3_HEADER_LENGTH - WZ_Z80_V2_HEADER_LENGTH);
    wz_write_le16(z80_v3 + 30u, 54u);

    wz_snapshot_state_init(&snapshot);
    memset(snapshot.data, 0x5au, sizeof(snapshot.data));
    snapshot.length = sizeof(snapshot.data);
    for (size_t iteration = 0u; iteration < 1024u; ++iteration) {
        size_t length;
        seed = seed * 1664525u + 1013904223u;
        for (size_t index = 0u; index < sizeof(random_data); ++index) {
            seed = seed * 1664525u + 1013904223u;
            random_data[index] = (wz_byte_t)(seed >> 24u);
        }
        length = (size_t)(seed % (sizeof(random_data) + 1u));
        original_snapshot = snapshot;
        if (wz_snapshot_state_load_sna_48k(&snapshot, random_data, length) !=
                WZ_RESULT_OK && memcmp(&snapshot, &original_snapshot,
                                       sizeof(snapshot)) != 0) {
            fputs("SNA parser published malformed fuzz input\n", stderr);
            exit(1);
        }
        original_snapshot = snapshot;
        if (wz_snapshot_state_load_z80_v1(&snapshot, random_data, length) !=
                WZ_RESULT_OK && memcmp(&snapshot, &original_snapshot,
                                       sizeof(snapshot)) != 0) {
            fputs("Z80 v1 parser published malformed fuzz input\n", stderr);
            exit(1);
        }
        original_snapshot = snapshot;
        if (wz_snapshot_state_load_z80_v2(&snapshot, random_data, length) !=
                WZ_RESULT_OK && memcmp(&snapshot, &original_snapshot,
                                       sizeof(snapshot)) != 0) {
            fputs("Z80 v2 parser published malformed fuzz input\n", stderr);
            exit(1);
        }
        original_snapshot = snapshot;
        if (wz_snapshot_state_load_z80_v3(&snapshot, random_data, length) !=
                WZ_RESULT_OK && memcmp(&snapshot, &original_snapshot,
                                       sizeof(snapshot)) != 0) {
            fputs("Z80 v3 parser published malformed fuzz input\n", stderr);
            exit(1);
        }
    }
    if (wz_sna_128k_image_init(&image) != WZ_RESULT_OK) {
        fputs("SNA 128K fuzz setup failed\n", stderr);
        exit(1);
    }
    for (size_t iteration = 0u; iteration < 1024u; ++iteration) {
        seed = seed * 1664525u + 1013904223u;
        for (size_t index = 0u; index < sizeof(random_data); ++index) {
            seed = seed * 1664525u + 1013904223u;
            random_data[index] = (wz_byte_t)(seed >> 24u);
        }
        (void)wz_sna_128k_image_load(&image, random_data,
                                     (size_t)(seed % (sizeof(random_data) + 1u)));
    }

    wz_write_le16(z80_v2 + WZ_Z80_V2_HEADER_LENGTH, 0u);
    wz_snapshot_state_init(&snapshot);
    if (wz_snapshot_state_load_z80_v2(&snapshot, z80_v2,
                                      WZ_Z80_V2_LENGTH) != WZ_RESULT_PARSE_ERROR) {
        fputs("Z80 v2 zero-length fuzz case was accepted\n", stderr);
        exit(1);
    }
    memset(z80_v2, 0, sizeof(z80_v2));
    z80_v2[27u] = 1u;
    z80_v2[28u] = 1u;
    z80_v2[29u] = 2u;
    z80_v2[30u] = 23u;
    wz_write_le16(z80_v2 + WZ_Z80_V2_HEADER_LENGTH, 4u);
    z80_v2[WZ_Z80_V2_HEADER_LENGTH + 2u] = 8u;
    z80_v2[WZ_Z80_V2_HEADER_LENGTH + 3u] = 0xedu;
    z80_v2[WZ_Z80_V2_HEADER_LENGTH + 4u] = 0xedu;
    if (wz_snapshot_state_load_z80_v2(&snapshot, z80_v2,
                                      WZ_Z80_V2_HEADER_LENGTH + 7u) !=
            WZ_RESULT_PARSE_ERROR) {
        fputs("Z80 v2 truncated run fuzz case was accepted\n", stderr);
        exit(1);
    }
    z80_v3[34u] = 1u;
    if (wz_snapshot_state_load_z80_v3(&snapshot, z80_v3,
                                      sizeof(z80_v3)) != WZ_RESULT_PARSE_ERROR) {
        fputs("Z80 v3 unsupported hardware fuzz case was accepted\n", stderr);
        exit(1);
    }
}

static void test_snapshot_cross_host_round_trips(void)
{
    static wz_byte_t sna[WZ_SNA_48K_LENGTH];
    static wz_byte_t z80[WZ_Z80_V2_LENGTH];
    static wz_byte_t output[WZ_Z80_V2_LENGTH];
    static wz_byte_t original_output[WZ_Z80_V2_LENGTH];
    static wz_snapshot_state_t snapshot;
    static wz_snapshot_state_t original_snapshot;
    static wz_machine_t machine;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        fputs("cross-host snapshot setup failed\n", stderr);
        exit(1);
    }
    machine.cpu.program_counter = 0x4321u;
    machine.cpu.stack_pointer = 0x8765u;
    machine.cpu.main.a = 0x12u;
    machine.cpu.main.f = 0x34u;
    machine.cpu.ix = 0xabcdU;
    machine.cpu.iy = 0x2345u;
    machine.border_color = 6u;
    machine.memory[0x4000u] = 0x11u;
    machine.memory[0x8000u] = 0x22u;
    machine.memory[0xc000u] = 0x33u;

    memset(sna, 0x5au, sizeof(sna));
    if (wz_state_save_sna_48k(&machine, sna, sizeof(sna)) != WZ_RESULT_OK ||
        sna[19u] != 0u || sna[23u] != 0x63u || sna[24u] != 0x87u ||
        sna[26u] != 6u || sna[27u + 0x0000u] != 0x11u) {
        fputs("portable SNA golden boundary failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_snapshot_state_init(&snapshot);
    if (wz_snapshot_state_load_sna_48k(&snapshot, sna, sizeof(sna)) !=
            WZ_RESULT_OK ||
        wz_snapshot_state_data(&snapshot)[33u] != 0x21u ||
        wz_snapshot_state_data(&snapshot)[34u] != 0x43u ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0x4000u] != 0x11u) {
        fputs("cross-host SNA round-trip failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    original_snapshot = snapshot;
    if (wz_snapshot_state_load_sna_48k(&snapshot, sna, sizeof(sna) - 1u) !=
            WZ_RESULT_INVALID_STATE ||
        memcmp(&snapshot, &original_snapshot, sizeof(snapshot)) != 0 ||
        wz_snapshot_state_load_sna_48k(&snapshot, z80, sizeof(z80)) !=
            WZ_RESULT_INVALID_STATE ||
        memcmp(&snapshot, &original_snapshot, sizeof(snapshot)) != 0) {
        fputs("SNA truncation or wrong-format failure was not atomic\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }

    memset(z80, 0xa5, sizeof(z80));
    if (wz_state_save_z80_v2_48k(&machine, z80, sizeof(z80)) != WZ_RESULT_OK ||
        z80[6u] != 0u || z80[7u] != 0u || z80[30u] != 23u ||
        z80[32u] != 0x21u || z80[33u] != 0x43u || z80[57u] != 8u) {
        fputs("portable Z80 golden boundary failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    if (wz_snapshot_state_load_z80_v2(&snapshot, z80, sizeof(z80)) !=
            WZ_RESULT_OK ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0x4000u] != 0x11u ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0x8000u] != 0x22u ||
        wz_snapshot_state_data(&snapshot)[WZ_STATE_MACHINE_MEMORY_OFFSET + 0xc000u] != 0x33u) {
        fputs("cross-host Z80 round-trip failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    original_snapshot = snapshot;
    z80[34u] = 1u;
    if (wz_snapshot_state_load_z80_v2(&snapshot, z80, sizeof(z80)) !=
            WZ_RESULT_PARSE_ERROR ||
        memcmp(&snapshot, &original_snapshot, sizeof(snapshot)) != 0) {
        fputs("Z80 wrong-model failure was not atomic\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    z80[34u] = 0u;
    if (wz_snapshot_state_load_z80_v2(&snapshot, z80, sizeof(z80) - 1u) !=
            WZ_RESULT_PARSE_ERROR ||
        memcmp(&snapshot, &original_snapshot, sizeof(snapshot)) != 0) {
        fputs("Z80 truncation failure was not atomic\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }

    machine.networking_mode = WZ_NETWORKING_INTERFACE1;
    memset(output, 0x5au, sizeof(output));
    memcpy(original_output, output, sizeof(original_output));
    if (wz_state_save_z80_v2_48k(&machine, output, sizeof(output)) !=
            WZ_RESULT_UNSUPPORTED_OPERATION ||
        memcmp(output, original_output, sizeof(output)) != 0) {
        fputs("Z80 unsupported-state save was accepted\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    memset(output, 0x5au, sizeof(output));
    if (wz_state_save_z80_v2_48k(&machine, output, sizeof(output) - 1u) !=
            WZ_RESULT_BUFFER_TOO_SMALL ||
        output[0u] != 0x5au || output[sizeof(output) - 1u] != 0x5au ||
        memcmp(output, original_output, sizeof(output)) != 0) {
        fputs("Z80 output failure was not atomic\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_128k_banked_memory_and_paging(void)
{
    static wz_machine_t machine;
    wz_bus_request_t request;

    if (wz_machine_init(&machine, wz_machine_profile_128k_pal()) != WZ_RESULT_OK) {
        fputs("128K paging setup failed\n", stderr);
        exit(1);
    }
    wz_machine_memory_write(&machine, 0x4000u, 0x15u);
    wz_machine_memory_write(&machine, 0x8000u, 0x22u);
    wz_machine_memory_write(&machine, 0xc000u, 0x00u);
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffdu, 0x03u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        wz_machine_128k_paging_value(&machine) != 0x03u ||
        wz_machine_memory_read(&machine, 0x4000u) != 0x15u ||
        wz_machine_memory_read(&machine, 0x8000u) != 0x22u) {
        fputs("128K fixed bank mapping failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_memory_write(&machine, 0xc000u, 0x33u);
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffdu, 0x04u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        wz_machine_memory_read(&machine, 0xc000u) != 0u) {
        fputs("128K selected bank isolation failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_memory_write(&machine, 0xc000u, 0x44u);
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x1ffdu, 0x04u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        wz_machine_128k_paging_value(&machine) != 0x04u) {
        fputs("128K paging alias decode failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffbu, 0x07u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        wz_machine_128k_paging_value(&machine) != 0x04u) {
        fputs("128K paging A1 rejection failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0xffffu, 0x07u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        wz_machine_128k_paging_value(&machine) != 0x04u) {
        fputs("128K paging A15 rejection failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_bus_request_init(&request, WZ_BUS_IO_READ, 0u, 0x7ffdu, 0u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        wz_machine_128k_paging_value(&machine) != 0x04u) {
        fputs("128K paging read interference failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    if (wz_machine_128k_screen_bank(&machine) != 5u ||
        wz_machine_128k_rom_bank(&machine) != 0u) {
        fputs("128K default screen or ROM selection failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffdu, 0x1cu, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        wz_machine_128k_paging_value(&machine) != 0x1cu ||
        wz_machine_128k_screen_bank(&machine) != 7u ||
        wz_machine_128k_rom_bank(&machine) != 1u) {
        fputs("128K paging latch fields failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffdu, 0x24u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        wz_machine_128k_paging_value(&machine) != 0x24u) {
        fputs("128K paging lock setup failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffdu, 0x01u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        wz_machine_128k_paging_value(&machine) != 0x24u ||
        wz_machine_memory_read(&machine, 0xc000u) != 0x44u) {
        fputs("128K paging lock was not enforced\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0xfffdu, 0x02u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        wz_machine_128k_paging_value(&machine) != 0x24u) {
        fputs("128K paging partial decode failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_128k_paging_all_bank_fixture(void)
{
    static wz_machine_t machine;
    wz_bus_request_t request;

    if (wz_machine_init(&machine, wz_machine_profile_128k_pal()) != WZ_RESULT_OK) {
        fputs("128K all-bank fixture setup failed\n", stderr);
        exit(1);
    }
    for (wz_byte_t bank = 0u; bank < WZ_128K_RAM_BANK_COUNT; ++bank) {
        wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffdu, bank, 4u);
        if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK) {
            fputs("128K all-bank select failed\n", stderr);
            wz_machine_destroy(&machine);
            exit(1);
        }
        wz_machine_memory_write(&machine, 0xc000u, (wz_byte_t)(0xa0u + bank));
    }
    for (wz_byte_t bank = 0u; bank < WZ_128K_RAM_BANK_COUNT; ++bank) {
        wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffdu, bank, 4u);
        if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
            wz_machine_memory_read(&machine, 0xc000u) !=
                (wz_byte_t)(0xa0u + bank)) {
            fputs("128K all-bank isolation fixture failed\n", stderr);
            wz_machine_destroy(&machine);
            exit(1);
        }
    }
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffdu, 0x1fu, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        wz_machine_128k_screen_bank(&machine) != 7u ||
        wz_machine_128k_rom_bank(&machine) != 1u) {
        fputs("128K screen and ROM fixture failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffdu, 0x20u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK) {
        fputs("128K lock fixture setup failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffdu, 0x01u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        wz_machine_128k_paging_value(&machine) != 0x20u) {
        fputs("128K lock fixture failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_128k_ula_profile(void)
{
    wz_machine_t machine;
    wz_bus_request_t request;
    wz_ula_fetch_event_t events[2u];
    size_t count = 0u;

    if (wz_machine_init(&machine, wz_machine_profile_128k_pal()) != WZ_RESULT_OK ||
        machine.profile->tstates_per_line != 228u ||
        machine.profile->lines_per_frame != 311u ||
        machine.profile->tstates_per_frame != 70908u ||
        machine.profile->raster_clocks_per_line != 456u ||
        machine.profile->ula_fetch_start_tstate != 14361u ||
        machine.profile->ula_fetch_line_count != 192u ||
        machine.profile->ula_fetches_per_line != 32u ||
        machine.profile->ula_fetch_interval_tstates != 4u) {
        fputs("128K ULA profile geometry failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffdu, 0x0fu, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK) {
        fputs("128K ULA screen-bank setup failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_memory_write(&machine, 0xc000u, 0xa5u);
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffdu, 0x08u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        wz_machine_ula_fetches_at_tick(&machine, 14361u * 2u, events,
                                       sizeof(events) / sizeof(events[0u]),
                                       &count) != WZ_RESULT_OK || count != 2u ||
        events[0u].address != 0x4000u || events[0u].value != 0xa5u ||
        events[0u].master_tick != 14361u * 2u ||
        events[1u].address != 0x5800u ||
        events[1u].master_tick != 14361u * 2u + 2u) {
        fputs("128K ULA screen-bank fetch failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 0u, 0x7ffdu, 0x00u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK) {
        fputs("128K ULA fixed-screen setup failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_memory_write(&machine, 0x4000u, 0x5au);
    if (wz_machine_ula_fetches_at_tick(&machine, 14361u * 2u, events,
                                       sizeof(events) / sizeof(events[0u]),
                                       &count) != WZ_RESULT_OK ||
        events[0u].value != 0x5au) {
        fputs("128K ULA fixed-screen fetch failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_128k_timing_relationships(void)
{
    const wz_machine_profile_t* profile = wz_machine_profile_128k_pal();
    const wz_machine_profile_t* profile_48k = wz_machine_profile_48k_pal();
    wz_machine_t machine;
    wz_raster_position_t position;
    const wz_qword_t line_ticks = 456u;
    const wz_qword_t frame_ticks = 141816u;

    if (profile->master_ticks_per_cpu_tstate != 2u ||
        profile->tstates_per_line * profile->master_ticks_per_cpu_tstate !=
            profile->raster_clocks_per_line ||
        profile->tstates_per_frame * profile->master_ticks_per_cpu_tstate !=
            frame_ticks ||
        profile->raster_clocks_per_line * profile->lines_per_frame !=
            frame_ticks ||
        wz_profile_cpu_tstate(0u, profile) != 0u ||
        wz_profile_cpu_tstate(1u, profile) != 0u ||
        wz_profile_cpu_tstate(2u, profile) != 1u ||
        wz_profile_cpu_tstate(3u, profile) != 1u ||
        wz_profile_cpu_phase(0u, profile) != 0u ||
        wz_profile_cpu_phase(1u, profile) != 1u ||
        wz_profile_cpu_phase(2u, profile) != 0u ||
        wz_profile_cpu_phase(3u, profile) != 1u ||
        profile_48k->tstates_per_line *
            profile_48k->master_ticks_per_cpu_tstate !=
            profile_48k->raster_clocks_per_line ||
        profile_48k->tstates_per_frame *
            profile_48k->master_ticks_per_cpu_tstate !=
            profile_48k->raster_clocks_per_line * profile_48k->lines_per_frame) {
        fputs("128K master-tick relationship failed\n", stderr);
        exit(1);
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("128K raster relationship setup failed\n", stderr);
        exit(1);
    }
    machine.master_tick = line_ticks - 1u;
    if (wz_machine_raster_position(&machine, &position) != WZ_RESULT_OK ||
        position.frame_number != 0u || position.frame_raster_clock != 455u ||
        position.line != 0u || position.raster_clock != 455u) {
        fputs("128K final raster clock relationship failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    machine.master_tick = line_ticks;
    if (wz_machine_raster_position(&machine, &position) != WZ_RESULT_OK ||
        position.frame_number != 0u || position.frame_raster_clock != 456u ||
        position.line != 1u || position.raster_clock != 0u) {
        fputs("128K line boundary relationship failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    machine.master_tick = frame_ticks - 1u;
    if (wz_machine_raster_position(&machine, &position) != WZ_RESULT_OK ||
        position.frame_number != 0u || position.frame_raster_clock != 141815u ||
        position.line != 310u || position.raster_clock != 455u) {
        fputs("128K final frame clock relationship failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    machine.master_tick = frame_ticks;
    if (wz_machine_raster_position(&machine, &position) != WZ_RESULT_OK ||
        position.frame_number != 1u || position.frame_raster_clock != 0u ||
        position.line != 0u || position.raster_clock != 0u) {
        fputs("128K frame wrap relationship failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_128k_ay_register_timing(void)
{
    static wz_machine_t machine;
    static wz_machine_t machine_48k;
    wz_bus_request_t select_request;
    wz_bus_request_t write_request;
    wz_ay_event_t events[2u];

    if (wz_machine_init(&machine, wz_machine_profile_128k_pal()) != WZ_RESULT_OK) {
        fputs("AY timing setup failed\n", stderr);
        exit(1);
    }
    wz_bus_request_init(&select_request, WZ_BUS_IO_WRITE, 100u, 0xfffdu,
                        0x1fu, 4u);
    if (wz_machine_bus_request(&machine, &select_request) != WZ_RESULT_OK ||
        select_request.source != WZ_BUS_SOURCE_AY ||
        wz_machine_ay_selected_register(&machine) != 0x0fu) {
        fputs("AY register-select port decode failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_bus_request_init(&write_request, WZ_BUS_IO_WRITE, 102u, 0xbffdu,
                        0xa5u, 4u);
    if (wz_machine_bus_request(&machine, &write_request) != WZ_RESULT_OK ||
        write_request.source != WZ_BUS_SOURCE_AY ||
        wz_machine_ay_register_value(&machine, 0x0fu) != 0xa5u ||
        wz_machine_ay_register_value(&machine, 0x00u) != 0u ||
        wz_machine_ay_events(&machine, events, 2u) != 2u ||
        events[0u].kind != WZ_AY_EVENT_REGISTER_SELECT ||
        events[0u].master_tick != 100u || events[0u].register_index != 0x0fu ||
        events[0u].value != 0x1fu ||
        events[1u].kind != WZ_AY_EVENT_REGISTER_WRITE ||
        events[1u].master_tick != 102u || events[1u].register_index != 0x0fu ||
        events[1u].value != 0xa5u) {
        fputs("AY register-write timing failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
    if (wz_machine_init(&machine_48k, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        fputs("48K AY isolation setup failed\n", stderr);
        exit(1);
    }
    wz_bus_request_init(&write_request, WZ_BUS_IO_WRITE, 100u, 0xbffdu,
                        0x5au, 4u);
    if (wz_machine_bus_request(&machine_48k, &write_request) != WZ_RESULT_OK ||
        write_request.source == WZ_BUS_SOURCE_AY ||
        wz_machine_ay_events(&machine_48k, events, 2u) != 0u) {
        fputs("48K AY isolation failed\n", stderr);
        wz_machine_destroy(&machine_48k);
        exit(1);
    }
    wz_machine_destroy(&machine_48k);
}

static void test_ay_tone_generators(void)
{
    wz_ay_t ay;

    wz_ay_init(&ay);
    if (wz_ay_tone_period(&ay, 0u) != 1u ||
        wz_ay_tone_level(&ay, 0u) != 0u ||
        wz_ay_tone_period(&ay, 3u) != 0u ||
        wz_ay_advance_master_ticks(0, 1u) != WZ_RESULT_INVALID_ARGUMENT) {
        fputs("AY tone reset contract failed\n", stderr);
        exit(1);
    }
    if (wz_ay_select_register(&ay, 0u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 1u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 1u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0u, 0u) != WZ_RESULT_OK ||
        wz_ay_tone_period(&ay, 0u) != 1u ||
        wz_ay_advance_master_ticks(&ay, WZ_AY_MASTER_TICKS_PER_CLOCK - 1u) !=
            WZ_RESULT_OK || wz_ay_tone_level(&ay, 0u) != 0u ||
        wz_ay_advance_master_ticks(&ay, 1u) != WZ_RESULT_OK ||
        wz_ay_tone_level(&ay, 0u) != 1u ||
        wz_ay_advance_master_ticks(&ay, WZ_AY_MASTER_TICKS_PER_CLOCK) !=
            WZ_RESULT_OK || wz_ay_tone_level(&ay, 0u) != 0u) {
        fputs("AY tone period-one divider failed\n", stderr);
        exit(1);
    }
    if (wz_ay_select_register(&ay, 2u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x34u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 3u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x02u, 0u) != WZ_RESULT_OK ||
        wz_ay_tone_period(&ay, 1u) != 0x234u ||
        wz_ay_tone_level(&ay, 1u) != 0u ||
        wz_ay_advance_master_ticks(&ay, 4u * 0x234u) != WZ_RESULT_OK ||
        wz_ay_tone_level(&ay, 1u) != 1u) {
        fputs("AY tone 12-bit period or channel isolation failed\n", stderr);
        exit(1);
    }
}

static void test_ay_noise_generator(void)
{
    wz_ay_t ay;

    wz_ay_init(&ay);
    if (wz_ay_noise_period(&ay) != 1u || wz_ay_noise_level(&ay) != 1u ||
        wz_ay_advance_master_ticks(0, 1u) != WZ_RESULT_INVALID_ARGUMENT) {
        fputs("AY noise reset contract failed\n", stderr);
        exit(1);
    }
    if (wz_ay_select_register(&ay, 6u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x20u, 0u) != WZ_RESULT_OK ||
        wz_ay_noise_period(&ay) != 1u ||
        wz_ay_advance_master_ticks(&ay, 4u * 17u) != WZ_RESULT_OK ||
        wz_ay_noise_level(&ay) != 0u ||
        wz_ay_advance_master_ticks(&ay, 4u) != WZ_RESULT_OK ||
        wz_ay_noise_level(&ay) != 0u) {
        fputs("AY noise period, masking, or LFSR progression failed\n", stderr);
        exit(1);
    }
}

static void test_ay_envelope_generator(void)
{
    wz_ay_t ay;

    wz_ay_init(&ay);
    if (wz_ay_envelope_period(&ay) != 1u ||
        wz_ay_envelope_level(&ay) != 0u ||
        wz_ay_envelope_period(0) != 0u) {
        fputs("AY envelope reset contract failed\n", stderr);
        exit(1);
    }
    if (wz_ay_select_register(&ay, 11u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 2u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 12u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 1u, 0u) != WZ_RESULT_OK ||
        wz_ay_envelope_period(&ay) != 0x0102u ||
        wz_ay_select_register(&ay, 13u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x1cu, 0u) != WZ_RESULT_OK ||
        wz_ay_register_value(&ay, 13u) != 0x0cu ||
        wz_ay_envelope_level(&ay) != 0u) {
        fputs("AY envelope period or restart failed\n", stderr);
        exit(1);
    }
    if (wz_ay_select_register(&ay, 11u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 1u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 12u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0u, 0u) != WZ_RESULT_OK ||
        wz_ay_advance_master_ticks(&ay, 4u * 15u) != WZ_RESULT_OK ||
        wz_ay_envelope_level(&ay) != 15u) {
        fputs("AY envelope attack progression failed\n", stderr);
        exit(1);
    }
    if (wz_ay_advance_master_ticks(&ay, 4u) != WZ_RESULT_OK ||
        wz_ay_envelope_level(&ay) != 0u) {
        fputs("AY envelope cycle transition failed\n", stderr);
        exit(1);
    }
    if (wz_ay_select_register(&ay, 13u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x09u, 0u) != WZ_RESULT_OK ||
        wz_ay_advance_master_ticks(&ay, 4u * 16u) != WZ_RESULT_OK ||
        wz_ay_envelope_level(&ay) != 0u ||
        wz_ay_advance_master_ticks(&ay, 4u) != WZ_RESULT_OK ||
        wz_ay_envelope_level(&ay) != 0u) {
        fputs("AY envelope hold behavior failed\n", stderr);
        exit(1);
    }
}

static void test_ay_audio_mixer(void)
{
    wz_ay_t ay;

    wz_ay_init(&ay);
    if (wz_audio_mixer_ay_sample(0) != 0 ||
        wz_audio_mixer_sample(0, 0) != 0) {
        fputs("AY mixer null-state contract failed\n", stderr);
        exit(1);
    }
    for (wz_byte_t channel = 0u; channel < WZ_AY_CHANNEL_COUNT; ++channel) {
        if (wz_ay_select_register(&ay, (wz_byte_t)(8u + channel), 0u) !=
                WZ_RESULT_OK ||
            wz_ay_write_data(&ay, 15u, 0u) != WZ_RESULT_OK) {
            fputs("AY mixer volume setup failed\n", stderr);
            exit(1);
        }
    }
    if (wz_audio_mixer_ay_sample(&ay) !=
            -(wz_audio_sample_t)(3u * 65536u)) {
        fputs("AY mixer low-level summation failed\n", stderr);
        exit(1);
    }
    if (wz_ay_select_register(&ay, 7u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 1u, 0u) != WZ_RESULT_OK ||
        wz_audio_mixer_ay_sample(&ay) != -(wz_audio_sample_t)65536) {
        fputs("AY mixer tone-enable inversion failed\n", stderr);
        exit(1);
    }
    if (wz_ay_select_register(&ay, 7u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x08u, 0u) != WZ_RESULT_OK ||
        wz_ay_advance_master_ticks(&ay, WZ_AY_MASTER_TICKS_PER_CLOCK) !=
            WZ_RESULT_OK ||
        wz_audio_mixer_ay_sample(&ay) != (wz_audio_sample_t)(3u * 65536u)) {
        fputs("AY mixer oscillator sampling failed\n", stderr);
        exit(1);
    }
    if (wz_ay_select_register(&ay, 8u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x10u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 9u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 10u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 13u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x0cu, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 7u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0x09u, 0u) != WZ_RESULT_OK ||
        wz_ay_advance_master_ticks(&ay, 4u * 12u) != WZ_RESULT_OK ||
        wz_audio_mixer_ay_sample(&ay) != (wz_audio_sample_t)65536) {
        fputs("AY mixer envelope-volume selection failed\n", stderr);
        exit(1);
    }
    if (wz_audio_mixer_sample(WZ_AUDIO_MIXER_MAX, &ay) !=
            WZ_AUDIO_MIXER_MAX ||
        wz_audio_mixer_sample(12345, &ay) !=
            (wz_audio_sample_t)(12345 + 65536)) {
        fputs("AY mixer saturation or beeper composition failed\n", stderr);
        exit(1);
    }
    if (wz_ay_select_register(&ay, 8u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 15u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 9u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 15u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 10u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 15u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&ay, 7u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&ay, 0u, 0u) != WZ_RESULT_OK ||
        wz_ay_advance_master_ticks(&ay, WZ_AY_MASTER_TICKS_PER_CLOCK) !=
            WZ_RESULT_OK ||
        wz_audio_mixer_sample(WZ_AUDIO_MIXER_MIN, &ay) !=
            WZ_AUDIO_MIXER_MIN) {
        fputs("AY mixer negative saturation failed\n", stderr);
        exit(1);
    }
}

static void test_ay_state_round_trip(void)
{
    static wz_machine_t first;
    static wz_machine_t second;
    wz_state_writer_t writer;
    wz_state_writer_t writer_again;
    static wz_byte_t data[WZ_STATE_SNAPSHOT_CAPACITY];
    static wz_byte_t data_again[WZ_STATE_SNAPSHOT_CAPACITY];
    static wz_byte_t invalid[WZ_STATE_SNAPSHOT_CAPACITY];
    wz_qword_t first_hash = 0u;
    wz_qword_t second_hash = 0u;
    wz_audio_sample_t first_sample;
    wz_audio_sample_t second_sample;
    size_t length;

    if (wz_machine_init(&first, wz_machine_profile_128k_pal()) != WZ_RESULT_OK ||
        wz_machine_init(&second, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        fputs("AY state round-trip setup failed\n", stderr);
        exit(1);
    }
    first.ay.selected_register = 13u;
    first.ay.registers[7u] = 0x09u;
    first.ay.registers[8u] = 0x10u;
    first.ay.registers[9u] = 7u;
    first.ay.registers[10u] = 3u;
    first.ay.registers[11u] = 0x34u;
    first.ay.registers[12u] = 0x12u;
    first.ay.registers[13u] = 0x0eu;
    first.ay.tone_counters[0u] = 9u;
    first.ay.tone_counters[1u] = 10u;
    first.ay.tone_counters[2u] = 11u;
    first.ay.tone_levels[0u] = 1u;
    first.ay.tone_levels[1u] = 0u;
    first.ay.tone_levels[2u] = 1u;
    first.ay.noise_counter = 4u;
    first.ay.noise_lfsr = 0x13579u;
    first.ay.noise_level = 1u;
    first.ay.envelope_counter = 5u;
    first.ay.envelope_period = 0x1234u;
    first.ay.envelope_level = 6u;
    first.ay.envelope_attack = 1u;
    first.ay.envelope_holding = 0u;
    first.ay.tone_master_tick_phase = 2u;
    wz_state_writer_init(&writer, data, sizeof(data));
    if (wz_state_serialize_machine(&first, &writer) != WZ_RESULT_OK ||
        writer.length != WZ_STATE_SNAPSHOT_CAPACITY ||
        wz_state_hash_machine(&first, &first_hash) != WZ_RESULT_OK) {
        fputs("AY state serialization failed\n", stderr);
        wz_machine_destroy(&first);
        wz_machine_destroy(&second);
        exit(1);
    }
    first_sample = wz_audio_mixer_sample(1234, &first.ay);
    if (wz_state_deserialize_machine(&second, data, writer.length) != WZ_RESULT_OK ||
        wz_state_hash_machine(&second, &second_hash) != WZ_RESULT_OK ||
        first_hash != second_hash) {
        fputs("AY state hash round-trip failed\n", stderr);
        wz_machine_destroy(&first);
        wz_machine_destroy(&second);
        exit(1);
    }
    wz_state_writer_init(&writer_again, data_again, sizeof(data_again));
    second_sample = wz_audio_mixer_sample(1234, &second.ay);
    if (wz_state_serialize_machine(&second, &writer_again) != WZ_RESULT_OK ||
        writer_again.length != writer.length ||
        memcmp(data, data_again, writer.length) != 0 ||
        first_sample != second_sample) {
        fputs("AY state byte or mixer round-trip failed\n", stderr);
        wz_machine_destroy(&first);
        wz_machine_destroy(&second);
        exit(1);
    }
    length = writer.length;
    memcpy(invalid, data, length);
    invalid[73u] = WZ_AY_REGISTER_COUNT;
    if (wz_state_deserialize_machine(&second, invalid, length) == WZ_RESULT_OK ||
        wz_state_hash_machine(&second, &second_hash) != WZ_RESULT_OK ||
        second_hash != first_hash) {
        fputs("AY state invalid-input atomicity failed\n", stderr);
        wz_machine_destroy(&first);
        wz_machine_destroy(&second);
        exit(1);
    }
    wz_machine_destroy(&first);
    wz_machine_destroy(&second);
}

static void test_tape_trap_eligibility(void)
{
    const wz_tape_segment_t segment = {1u, 0u};
    wz_machine_t machine;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_machine_tape_trap_reason(&machine) != WZ_TAPE_TRAP_REASON_NORMAL_MODE ||
        wz_machine_set_tape_loading_mode(&machine, WZ_TAPE_LOADING_INSTANT_TRAP) !=
            WZ_RESULT_OK ||
        wz_machine_tape_trap_reason(&machine) != WZ_TAPE_TRAP_REASON_NO_TAPE ||
        wz_machine_mount_tape(&machine, &segment, 1u) != WZ_RESULT_OK ||
        wz_machine_tape_trap_reason(&machine) != WZ_TAPE_TRAP_REASON_NO_ROM) {
        fputs("tape trap prerequisite eligibility failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    machine.has_48k_rom = 1u;
    if (wz_machine_tape_trap_reason(&machine) !=
            WZ_TAPE_TRAP_REASON_NO_RECOGNIZED_LOADER) {
        fputs("tape trap false-positive eligibility failed\n", stderr);
        wz_machine_destroy(&machine);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_tape_speed_invariance(void)
{
    const wz_tape_segment_t segments[3u] = {{2u, 0u}, {3u, 1u}, {1u, 0u}};
    wz_byte_t baseline[6u] = {0u};
    bool have_baseline = false;

    for (unsigned speed_index = 0u; speed_index < WZ_SPEED_COUNT; ++speed_index) {
        wz_machine_t machine;
        wz_headless_runner_t runner;
        wz_host_pacing_t pacing;
        wz_speed_policy_t speed = (wz_speed_policy_t)speed_index;
        wz_byte_t observed[6u] = {0u};

        if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
            wz_machine_mount_tape(&machine, segments, 3u) != WZ_RESULT_OK ||
            wz_machine_set_tape_motor(&machine, true) != WZ_RESULT_OK ||
            wz_headless_runner_init(&runner, &machine, 0) != WZ_RESULT_OK ||
            !wz_host_pacing_init(&pacing, 1000u, speed, 0u, 0u) ||
            !wz_host_pacing_set_speed(&pacing, speed)) {
            fputs("tape speed-invariance setup failed\n", stderr);
            wz_machine_destroy(&machine);
            exit(1);
        }
        for (size_t tick = 0u; tick < 6u; ++tick) {
            if (wz_headless_runner_advance(&runner, 1u) != WZ_RESULT_OK) {
                fputs("tape speed-invariance advancement failed\n", stderr);
                wz_machine_destroy(&machine);
                exit(1);
            }
            observed[tick] = wz_machine_tape_ear_level(&machine);
        }
        if (!have_baseline) {
            memcpy(baseline, observed, sizeof(baseline));
            have_baseline = true;
        } else if (memcmp(baseline, observed, sizeof(baseline)) != 0) {
            fputs("tape timing changed with runtime speed\n", stderr);
            wz_machine_destroy(&machine);
            exit(1);
        }
        wz_machine_destroy(&machine);
    }
}

static void test_standard_tap_parser(void)
{
    const wz_byte_t tap[] = {2u, 0u, 0u, 0u};
    const wz_byte_t bad_checksum[] = {2u, 0u, 0u, 1u};
    wz_tape_segment_t sentinel = {77u, 1u};
    wz_tape_segment_t* segments;
    size_t count = 0u;
    size_t required;

    if (wz_tape_parse_standard_tap(tap, sizeof(tap), 2u, 0, 0u, &required) !=
            WZ_RESULT_BUFFER_TOO_SMALL || required != 8098u) {
        fputs("TAP parser sizing failed\n", stderr);
        exit(1);
    }
    segments = (wz_tape_segment_t*)malloc(required * sizeof(*segments));
    if (segments == 0 ||
        wz_tape_parse_standard_tap(tap, sizeof(tap), 2u, segments, required, &count) !=
            WZ_RESULT_OK || count != required || segments[0].duration != 4336u ||
        segments[0].ear_level != 1u ||
        segments[required - 1u].duration != 7000000u ||
        segments[required - 1u].ear_level != 0u) {
        fputs("TAP parser expansion failed\n", stderr);
        free(segments);
        exit(1);
    }
    free(segments);
    if (wz_tape_parse_standard_tap(bad_checksum, sizeof(bad_checksum), 2u,
                                   &sentinel, 1u, &count) != WZ_RESULT_PARSE_ERROR ||
        sentinel.duration != 77u || sentinel.ear_level != 1u ||
        wz_tape_parse_standard_tap(tap, 3u, 2u, &sentinel, 1u, &count) !=
            WZ_RESULT_PARSE_ERROR || sentinel.duration != 77u) {
        fputs("TAP parser rejection atomicity failed\n", stderr);
        exit(1);
    }
}

static void test_standard_tap_writer(void)
{
    const wz_byte_t first_data[] = {0x00u, 0xaau};
    const wz_byte_t second_data[] = {0xffu};
    const wz_tap_block_t blocks[] = {
        {first_data, sizeof(first_data)},
        {second_data, sizeof(second_data)}
    };
    const wz_byte_t expected[] = {3u, 0u, 0u, 0xaau, 0xaau, 2u, 0u, 0xffu, 0xffu};
    wz_byte_t output[sizeof(expected)];
    wz_byte_t sentinel[sizeof(expected)];
    size_t length = 0u;

    memset(sentinel, 0x5au, sizeof(sentinel));
    if (wz_tape_write_standard_tap(blocks, 2u, 0, 0u, &length) !=
            WZ_RESULT_BUFFER_TOO_SMALL || length != sizeof(expected) ||
        wz_tape_write_standard_tap(blocks, 2u, sentinel, sizeof(sentinel) - 1u,
                                   &length) != WZ_RESULT_BUFFER_TOO_SMALL ||
        memcmp(sentinel, (wz_byte_t[sizeof(sentinel)]){
                   0x5au, 0x5au, 0x5au, 0x5au, 0x5au, 0x5au, 0x5au, 0x5au, 0x5au},
                   sizeof(sentinel)) != 0 ||
        wz_tape_write_standard_tap(blocks, 2u, output, sizeof(output), &length) !=
            WZ_RESULT_OK || length != sizeof(expected) ||
        memcmp(output, expected, sizeof(expected)) != 0) {
        fputs("TAP writer output contract failed\n", stderr);
        exit(1);
    }
    if (wz_tape_parse_standard_tap(output, length, 2u, 0, 0u, &length) !=
            WZ_RESULT_BUFFER_TOO_SMALL) {
        fputs("TAP writer/parser compatibility failed\n", stderr);
        exit(1);
    }
}

static void test_native_tap_parser(void)
{
    wz_byte_t native[32u] = {0xffu, 0xffu, 0xffu, 0xffu,
                             0xffu, 0xffu, 0xffu, 0xffu,
                             0xffu, 0xffu, 0xffu, 0xffu};
    wz_native_tap_record_t record;
    size_t count = 0u;
    wz_result_t sizing_result;

    native[0u] = 12u;
    native[1u] = 0u;
    native[2u] = 0u;
    native[3u] = 0u;
    native[12u] = 0u;
    native[13u] = 0u;
    native[14u] = 0u;
    native[15u] = 0u;
    native[16u] = 0xffu;
    native[17u] = 0xffu;
    native[18u] = 0xffu;
    native[19u] = 0xffu;
    native[20u] = 4u;
    native[21u] = 0u;
    native[22u] = 0xa5u;
    native[23u] = 1u;
    native[24u] = 0x01u;
    native[25u] = 0x02u;
    if (!wz_tape_is_native_tap(native, sizeof(native))) {
        fputs("native TAP detection failed\n", stderr);
        exit(1);
    }
    sizing_result = wz_tape_parse_native_tap(native, sizeof(native), 0, 0u, &count);
    if (sizing_result != WZ_RESULT_BUFFER_TOO_SMALL || count != 1u) {
        fprintf(stderr, "native TAP sizing failed: result=%d count=%zu\n",
                (int)sizing_result, count);
        exit(1);
    }
    if (wz_tape_parse_native_tap(native, sizeof(native), &record, 1u, &count) !=
            WZ_RESULT_OK) {
        fputs("native TAP traversal failed\n", stderr);
        exit(1);
    }
    if (record.offset != 12u || record.previous_offset != 0u ||
        record.next_offset != UINT32_MAX || record.stored_length != 4u ||
        record.flag != 0xa5u || record.record_type != 1u ||
        record.payload_length != 2u || record.payload[0u] != 0x01u ||
        record.payload[1u] != 0x02u) {
        fputs("native TAP parser contract failed\n", stderr);
        exit(1);
    }
}

static void test_native_tap_writer(void)
{
    const wz_byte_t ordinary_payload[2u] = {0x01u, 0x02u};
    const wz_byte_t sample_payload[3u] = {0xa0u, 0xa1u, 0xa2u};
    wz_native_tap_record_t records[2u] = {{0}};
    wz_native_tap_record_t parsed[2u] = {{0}};
    wz_byte_t output[64u];
    wz_byte_t sentinel[64u];
    size_t length = 0u;

    records[0u].stored_length = 4u;
    records[0u].record_type = 1u;
    records[0u].flag = 0xa5u;
    records[0u].payload = ordinary_payload;
    records[0u].payload_length = sizeof(ordinary_payload);
    records[1u].stored_length = 65534u;
    records[1u].record_type = 4u;
    records[1u].flag = 0x5au;
    records[1u].decompressed_length = 123u;
    records[1u].compressed_length = 45u;
    records[1u].signed_length = 67u;
    records[1u].payload = sample_payload;
    records[1u].payload_length = sizeof(sample_payload);
    memset(sentinel, 0x5au, sizeof(sentinel));
    memcpy(output, sentinel, sizeof(output));
    if (wz_tape_write_native_tap(records, 2u, sentinel, 45u,
                                 &length) != WZ_RESULT_BUFFER_TOO_SMALL ||
        length != 46u || memcmp(sentinel, output, sizeof(sentinel)) != 0 ||
        wz_tape_write_native_tap(records, 2u, output, sizeof(output), &length) !=
            WZ_RESULT_OK || length != 46u || !wz_tape_is_native_tap(output, length) ||
        wz_tape_parse_native_tap(output, length, parsed, 2u, &length) != WZ_RESULT_OK ||
        parsed[0u].offset != 12u || parsed[0u].next_offset != 26u ||
        parsed[0u].previous_offset != 0u || parsed[0u].record_type != 1u ||
        parsed[0u].payload_length != 2u || parsed[1u].offset != 26u ||
        parsed[1u].previous_offset != 12u || parsed[1u].next_offset != UINT32_MAX ||
        parsed[1u].record_type != 4u || parsed[1u].decompressed_length != 123u ||
        parsed[1u].compressed_length != 45u || parsed[1u].signed_length != 67u ||
        parsed[1u].payload_length != sizeof(sample_payload) ||
        memcmp(parsed[1u].payload, sample_payload, sizeof(sample_payload)) != 0) {
        fputs("native TAP writer round-trip failed\n", stderr);
        exit(1);
    }
}

static void test_tzx_parser(void)
{
    const wz_byte_t valid[21u] = {
        'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1au, 1u, 20u,
        0x10u, 0x00u, 0x00u, 0x02u, 0x00u, 0x00u, 0xffu,
        0x21u, 0x02u, 'O', 'K'
    };
    const wz_byte_t truncated[15u] = {
        'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1au, 1u, 20u,
        0x10u, 0x00u, 0x00u, 0x02u, 0x00u
    };
    const wz_byte_t unsupported[11u] = {
        'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1au, 1u, 20u, 0x16u
    };
    const wz_byte_t bad_jump[13u] = {
        'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1au, 1u, 20u,
        0x23u, 0xffu, 0x7fu
    };
    const wz_byte_t direct_recording[20u] = {
        'Z', 'X', 'T', 'a', 'p', 'e', '!', 0x1au, 1u, 20u,
        0x15u, 0xe8u, 0x03u, 0x00u, 0x00u, 0x08u, 0x01u, 0x00u, 0x00u,
        0xa5u
    };
    wz_tzx_block_t blocks[2u];
    size_t count = 0u;
    wz_tzx_block_t sentinel[1u] = {{0}};
    sentinel[0u].offset = 99u;

    if (wz_tape_parse_tzx(valid, sizeof(valid), 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 2u ||
        wz_tape_parse_tzx(valid, sizeof(valid), blocks, 2u, &count) !=
            WZ_RESULT_OK || count != 2u || blocks[0u].block_id != 0x10u ||
        blocks[0u].disposition != WZ_TZX_SUPPORTED ||
        blocks[0u].block_length != 7u || blocks[1u].block_id != 0x21u ||
        blocks[1u].disposition != WZ_TZX_IGNORED ||
        blocks[1u].block_length != 4u ||
        wz_tape_parse_tzx(valid, sizeof(valid), sentinel, 1u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || sentinel[0u].offset != 99u ||
        wz_tape_parse_tzx(truncated, sizeof(truncated), blocks, 2u, &count) !=
            WZ_RESULT_PARSE_ERROR ||
        wz_tape_parse_tzx(unsupported, sizeof(unsupported), blocks, 2u, &count) !=
            WZ_RESULT_UNSUPPORTED_OPERATION ||
        wz_tape_parse_tzx(bad_jump, sizeof(bad_jump), blocks, 2u, &count) !=
            WZ_RESULT_PARSE_ERROR ||
        wz_tape_parse_tzx(direct_recording, sizeof(direct_recording), blocks, 1u,
                          &count) != WZ_RESULT_OK || count != 1u ||
        blocks[0u].block_id != 0x15u || blocks[0u].block_length != 10u) {
        fputs("TZX parser contract failed\n", stderr);
        exit(1);
    }
}

static void test_tzx_timing(void)
{
    const wz_byte_t standard_data[6u] = {0x00u, 0x00u, 0x02u, 0x00u,
                                         0x00u, 0xffu};
    const wz_byte_t turbo_data[19u] = {
        0xe8u, 0x03u, 0x9bu, 0x02u, 0xefu, 0x02u, 0x57u, 0x03u,
        0xaeu, 0x06u, 0x02u, 0x00u, 8u, 0x00u, 0x00u, 1u, 0x00u,
        0x00u, 0xa5u
    };
    const wz_byte_t pure_data[11u] = {
        1u, 0u, 2u, 0u, 8u, 0u, 0u, 1u, 0u, 0u, 0xa5u
    };
    const wz_byte_t tone_data[4u] = {0xe8u, 0x03u, 0x02u, 0x00u};
    const wz_byte_t sequence_data[5u] = {2u, 0x01u, 0x00u, 0x02u, 0x00u};
    const wz_byte_t pause_data[2u] = {0x0au, 0x00u};
    const wz_byte_t signal_data[5u] = {1u, 0u, 0u, 0u, 0u};
    const wz_byte_t metadata_data[1u] = {0u};
    const wz_byte_t stop_data[4u] = {0u, 0u, 0u, 0u};
    const wz_byte_t after_stop_data[4u] = {0xe8u, 0x03u, 0x01u, 0x00u};
    const wz_tzx_block_t blocks[4u] = {
        {0u, 5u, 0x12u, WZ_TZX_SUPPORTED, tone_data, sizeof(tone_data)},
        {5u, 6u, 0x13u, WZ_TZX_SUPPORTED, sequence_data, sizeof(sequence_data)},
        {11u, 3u, 0x20u, WZ_TZX_SUPPORTED, pause_data, sizeof(pause_data)},
        {14u, 2u, 0x21u, WZ_TZX_IGNORED, metadata_data, sizeof(metadata_data)}
    };
    wz_tape_segment_t segments[5u];
    wz_tape_segment_t pulse_segments[20u];
    size_t count = 0u;
    const wz_tzx_block_t standard =
        {0u, 11u, 0x10u, WZ_TZX_SUPPORTED, standard_data, sizeof(standard_data)};
    const wz_tzx_block_t turbo =
        {0u, 20u, 0x11u, WZ_TZX_SUPPORTED, turbo_data, sizeof(turbo_data)};
    const wz_tzx_block_t pure =
        {0u, 11u, 0x14u, WZ_TZX_SUPPORTED, pure_data, sizeof(pure_data)};
    const wz_tzx_block_t signal_blocks[2u] = {
        {0u, 6u, 0x2bu, WZ_TZX_SUPPORTED, signal_data, sizeof(signal_data)},
        {6u, 5u, 0x12u, WZ_TZX_SUPPORTED, tone_data, sizeof(tone_data)}
    };
    const wz_tzx_block_t stop_blocks[3u] = {
        {0u, 5u, 0x12u, WZ_TZX_SUPPORTED, tone_data, sizeof(tone_data)},
        {5u, 5u, 0x2au, WZ_TZX_SUPPORTED, stop_data, sizeof(stop_data)},
        {10u, 5u, 0x12u, WZ_TZX_SUPPORTED, after_stop_data, sizeof(after_stop_data)}
    };
    const wz_byte_t jump_data[2u] = {2u, 0u};
    const wz_tzx_block_t jump_blocks[4u] = {
        {0u, 5u, 0x12u, WZ_TZX_SUPPORTED, tone_data, sizeof(tone_data)},
        {5u, 3u, 0x23u, WZ_TZX_SUPPORTED, jump_data, sizeof(jump_data)},
        {8u, 5u, 0x12u, WZ_TZX_SUPPORTED, tone_data, sizeof(tone_data)},
        {13u, 5u, 0x12u, WZ_TZX_SUPPORTED, tone_data, sizeof(tone_data)}
    };
    const wz_byte_t loop_start_data[2u] = {2u, 0u};
    const wz_tzx_block_t loop_blocks[3u] = {
        {0u, 3u, 0x24u, WZ_TZX_SUPPORTED, loop_start_data, sizeof(loop_start_data)},
        {3u, 5u, 0x12u, WZ_TZX_SUPPORTED, tone_data, sizeof(tone_data)},
        {8u, 1u, 0x25u, WZ_TZX_SUPPORTED, 0, 0u}
    };
    const wz_byte_t csw_data[20u] = {
        0x10u, 0x00u, 0x00u, 0x00u, 0x01u, 0x00u,
        0xe0u, 0x67u, 0x35u, 0x01u, 0x02u, 0x00u,
        0x00u, 0x00u, 0x02u, 0x00u, 0x04u, 0x00u,
        0x00u, 0x00u
    };
    const wz_tzx_block_t csw_blocks[1u] = {
        {0u, 21u, 0x18u, WZ_TZX_SUPPORTED, csw_data, sizeof(csw_data)}
    };
    const wz_byte_t csw_zrle_data[28u] = {
        0x18u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xe0u, 0x67u,
        0x35u, 0x02u, 0x02u, 0x00u, 0x00u, 0x00u, 0x78u, 0x9cu,
        0x63u, 0x62u, 0x60u, 0x61u, 0x60u, 0x60u, 0x00u, 0x00u,
        0x00u, 0x22u, 0x00u, 0x07u
    };
    const wz_tzx_block_t csw_zrle_blocks[1u] = {
        {0u, 29u, 0x18u, WZ_TZX_SUPPORTED, csw_zrle_data,
         sizeof(csw_zrle_data)}
    };
    const wz_byte_t generalized_data[25u] = {
        0x15u, 0x00u, 0x00u, 0x00u, 0x01u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
        0x02u, 0x00u, 0x00u, 0x00u, 0x01u, 0x02u,
        0x00u, 0x05u, 0x00u, 0x00u, 0x07u, 0x00u,
        0x40u
    };
    const wz_tzx_block_t generalized_blocks[1u] = {
        {0u, 26u, 0x19u, WZ_TZX_SUPPORTED, generalized_data,
         sizeof(generalized_data)}
    };
    const wz_byte_t generalized_pilot_data[27u] = {
        0x17u, 0x00u, 0x00u, 0x00u, 0x01u, 0x00u,
        0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
        0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
        0x00u, 0x03u, 0x00u, 0x00u, 0x02u, 0x00u,
        0x00u, 0x05u, 0x00u
    };
    const wz_tzx_block_t generalized_pilot_blocks[1u] = {
        {0u, 28u, 0x19u, WZ_TZX_SUPPORTED, generalized_pilot_data,
         sizeof(generalized_pilot_data)}
    };
    const wz_byte_t call_data[4u] = {1u, 0u, 2u, 0u};
    const wz_byte_t call_skip_data[2u] = {3u, 0u};
    const wz_tzx_block_t call_blocks[5u] = {
        {0u, 5u, 0x26u, WZ_TZX_SUPPORTED, call_data, sizeof(call_data)},
        {5u, 3u, 0x23u, WZ_TZX_SUPPORTED, call_skip_data, sizeof(call_skip_data)},
        {8u, 5u, 0x12u, WZ_TZX_SUPPORTED, tone_data, sizeof(tone_data)},
        {13u, 1u, 0x27u, WZ_TZX_SUPPORTED, 0, 0u},
        {14u, 5u, 0x12u, WZ_TZX_SUPPORTED, tone_data, sizeof(tone_data)}
    };
    if (wz_tape_expand_tzx_timing(csw_zrle_blocks, 1u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 2u ||
        wz_tape_expand_tzx_timing(csw_zrle_blocks, 1u, 2u, segments, 2u, &count) !=
            WZ_RESULT_OK || count != 2u || segments[0u].duration != 4u ||
        segments[1u].duration != 8u) {
        fputs("CSW Z-RLE timing contract failed\n", stderr);
        exit(1);
    }
    const wz_byte_t select_data[11u] = {
        0x09u, 0x00u, 0x02u, 0x02u, 0x00u, 0x01u, 'A',
        0x01u, 0x00u, 0x01u, 'B'
    };
    const wz_tzx_block_t select_blocks[3u] = {
        {0u, 12u, 0x28u, WZ_TZX_SUPPORTED, select_data, sizeof(select_data)},
        {12u, 5u, 0x12u, WZ_TZX_SUPPORTED, tone_data, sizeof(tone_data)},
        {17u, 5u, 0x12u, WZ_TZX_SUPPORTED, tone_data, sizeof(tone_data)}
    };

    if (wz_tape_expand_tzx_timing(&standard, 1u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 8097u ||
        wz_tape_expand_tzx_timing(&turbo, 1u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 20u ||
        wz_tape_expand_tzx_timing(&turbo, 1u, 2u, pulse_segments, 20u, &count) !=
            WZ_RESULT_OK || pulse_segments[0u].ear_level != 1u ||
        pulse_segments[1u].ear_level != 0u || pulse_segments[2u].ear_level != 1u ||
        pulse_segments[3u].ear_level != 0u || pulse_segments[4u].ear_level != 1u ||
        pulse_segments[5u].ear_level != 0u ||
        wz_tape_expand_tzx_timing(&pure, 1u, 2u, pulse_segments, 16u, &count) !=
            WZ_RESULT_OK || count != 16u || pulse_segments[0u].ear_level != 1u ||
        pulse_segments[1u].ear_level != 0u || pulse_segments[2u].ear_level != 1u ||
        pulse_segments[3u].ear_level != 0u ||
        wz_tape_expand_tzx_timing(signal_blocks, 2u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 2u ||
        wz_tape_expand_tzx_timing(signal_blocks, 2u, 2u, segments, 2u, &count) !=
            WZ_RESULT_OK || segments[0u].ear_level != 0u ||
        wz_tape_expand_tzx_timing(stop_blocks, 3u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 2u ||
        wz_tape_expand_tzx_timing(stop_blocks, 3u, 2u, segments, 2u, &count) !=
            WZ_RESULT_OK || segments[1u].duration != 2000u ||
        wz_tape_expand_tzx_timing(jump_blocks, 4u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 4u ||
        wz_tape_expand_tzx_timing(jump_blocks, 4u, 2u, segments, 4u, &count) !=
            WZ_RESULT_OK || count != 4u ||
        wz_tape_expand_tzx_timing(loop_blocks, 3u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 4u ||
        wz_tape_expand_tzx_timing(loop_blocks, 3u, 2u, segments, 4u, &count) !=
            WZ_RESULT_OK || count != 4u ||
        wz_tape_expand_tzx_timing(csw_blocks, 1u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 3u ||
        wz_tape_expand_tzx_timing(csw_blocks, 1u, 2u, segments, 5u, &count) !=
            WZ_RESULT_OK || count != 3u || segments[0u].duration != 4u ||
        segments[1u].duration != 8u || segments[2u].duration != 7000u ||
        wz_tape_expand_tzx_timing(csw_zrle_blocks, 1u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 2u ||
        wz_tape_expand_tzx_timing(csw_zrle_blocks, 1u, 2u, segments, 2u, &count) !=
            WZ_RESULT_OK || count != 2u || segments[0u].duration != 4u ||
        segments[1u].duration != 8u ||
        wz_tape_expand_tzx_timing(generalized_blocks, 1u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 3u ||
        wz_tape_expand_tzx_timing(generalized_blocks, 1u, 2u, segments, 5u, &count) !=
            WZ_RESULT_OK || count != 3u || segments[0u].duration != 10u ||
        segments[1u].duration != 14u || segments[2u].duration != 7000u ||
        wz_tape_expand_tzx_timing(generalized_pilot_blocks, 1u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 4u ||
        wz_tape_expand_tzx_timing(generalized_pilot_blocks, 1u, 2u, segments, 4u, &count) !=
            WZ_RESULT_OK || count != 4u || segments[0u].duration != 6u ||
        segments[1u].duration != 6u || segments[2u].duration != 10u ||
        segments[3u].duration != 7000u ||
        wz_tape_expand_tzx_timing(call_blocks, 5u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 4u ||
        wz_tape_expand_tzx_timing(call_blocks, 5u, 2u, segments, 5u, &count) !=
            WZ_RESULT_OK || count != 4u || segments[0u].duration != 2000u ||
        segments[2u].duration != 2000u ||
        wz_tape_expand_tzx_timing(select_blocks, 3u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 2u ||
        wz_tape_expand_tzx_timing(select_blocks, 3u, 2u, segments, 2u, &count) !=
            WZ_RESULT_OK || count != 2u || segments[0u].duration != 2000u ||
        segments[1u].duration != 2000u ||
        wz_tape_expand_tzx_timing(blocks, 4u, 2u, 0, 0u, &count) !=
            WZ_RESULT_BUFFER_TOO_SMALL || count != 5u ||
        wz_tape_expand_tzx_timing(blocks, 4u, 2u, segments, 5u, &count) !=
            WZ_RESULT_OK || segments[0u].duration != 2000u ||
        segments[0u].ear_level != 1u || segments[1u].ear_level != 0u ||
        segments[2u].duration != 2u || segments[3u].duration != 4u ||
        segments[4u].duration != 70000u || segments[4u].ear_level != 0u) {
        fputs("TZX timing contract failed\n", stderr);
        exit(1);
    }
}

static void test_beeper_port_fe_timeline(void)
{
    wz_machine_t machine;
    wz_beeper_event_t events[4u];
    size_t count;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        fputs("beeper machine setup failed\n", stderr);
        exit(1);
    }
    wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x00u, 100u);
    wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x18u, 200u);
    wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x00u, 300u);
    count = wz_machine_beeper_events(&machine, events, 4u);
    if (count != 2u || events[0].master_tick != 200u || events[0].level != 1u ||
        events[1].master_tick != 300u || events[1].level != 0u ||
        wz_machine_beeper_level(&machine) != 0u ||
        wz_machine_mic_level(&machine) != 0u) {
        fputs("beeper timestamp or MIC separation failed\n", stderr);
        exit(1);
    }
    wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x10u, 400u);
    if (wz_machine_mic_level(&machine) != 1u ||
        wz_machine_beeper_events(&machine, events, 4u) != 2u) {
        fputs("MIC-only port-FE transition failed\n", stderr);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_mic_capture_timeline(void)
{
    wz_machine_t machine;
    wz_mic_event_t events[2u];

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_machine_mic_capture_begin(&machine) != WZ_RESULT_OK) {
        fputs("MIC capture setup failed\n", stderr);
        exit(1);
    }
    wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x10u, 100u);
    wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x10u, 110u);
    wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x00u, 120u);
    wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x01u, 130u);
    if (wz_machine_mic_events(&machine, events, 2u) != 2u ||
        events[0].master_tick != 100u || events[0].level != 1u ||
        events[1].master_tick != 120u || events[1].level != 0u ||
        wz_machine_beeper_events(&machine, 0, 0u) != 0u ||
        wz_machine_mic_capture_end(&machine) != WZ_RESULT_OK) {
        fputs("MIC capture edge ordering failed\n", stderr);
        exit(1);
    }
    wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x00u, 140u);
    if (wz_machine_mic_events(&machine, events, 2u) != 2u ||
        wz_machine_mic_capture_overflowed(&machine)) {
        fputs("MIC capture stop behavior failed\n", stderr);
        exit(1);
    }
    if (wz_machine_mic_capture_begin(&machine) != WZ_RESULT_OK) {
        fputs("MIC capture reset failed\n", stderr);
        exit(1);
    }
    for (size_t index = 0u; index <= WZ_MIC_EVENT_CAPACITY; ++index) {
        wz_machine_ula_port_fe_write(&machine, 0x00feu,
                                     (index & 1u) == 0u ? 0x10u : 0x00u,
                                     (wz_master_tick_t)index);
    }
    if (!wz_machine_mic_capture_overflowed(&machine) ||
        wz_machine_mic_events(&machine, events, 2u) != 2u) {
        fputs("MIC capture overflow contract failed\n", stderr);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_beeper_pcm_render(void)
{
    const wz_beeper_event_t events[2u] = {{3u, 1u}, {7u, 0u}};
    wz_audio_sample_t samples[2u] = {0, 0};

    if (!wz_beeper_render_pcm(events, 2u, 0u, 0u, 10u, 2u,
                              samples, 2u) ||
        samples[0] != -13107 || samples[1] != -13107 ||
        !wz_beeper_render_pcm(events, 2u, 0u, 0u, 10u, 2u,
                               samples, 2u)) {
        fputs("beeper PCM integration contract failed\n", stderr);
        exit(1);
    }
}

static void test_audio_pcm_hash(void)
{
    const wz_beeper_event_t events[2u] = {{3u, 1u}, {7u, 0u}};
    wz_audio_sample_t first[4u] = {0, 0, 0, 0};
    wz_audio_sample_t second[4u] = {0, 0, 0, 0};
    wz_qword_t first_hash;
    wz_qword_t second_hash;

    if (!wz_beeper_render_pcm(events, 2u, 0u, 0u, 10u, 4u,
                              first, 4u) ||
        !wz_beeper_render_pcm(events, 2u, 0u, 0u, 10u, 4u,
                               second, 4u) ||
        wz_audio_samples_hash(first, 4u, &first_hash) != WZ_RESULT_OK ||
        wz_audio_samples_hash(second, 4u, &second_hash) != WZ_RESULT_OK ||
        first_hash != second_hash || first_hash == 0u) {
        fputs("canonical audio hash repeatability failed\n", stderr);
        exit(1);
    }
    second[3] += 1;
    if (wz_audio_samples_hash(second, 4u, &second_hash) != WZ_RESULT_OK ||
        first_hash == second_hash ||
        wz_audio_samples_hash(0, 1u, &second_hash) != WZ_RESULT_INVALID_ARGUMENT) {
        fputs("canonical audio hash sensitivity failed\n", stderr);
        exit(1);
    }
}

static void test_ay_pcm_hash(void)
{
    static wz_audio_sample_t first[64u];
    static wz_audio_sample_t second[64u];
    wz_ay_t first_ay;
    wz_ay_t second_ay;
    wz_qword_t first_hash;
    wz_qword_t second_hash;

    wz_ay_init(&first_ay);
    wz_ay_init(&second_ay);
    if (wz_ay_select_register(&first_ay, 0u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&first_ay, 1u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&first_ay, 7u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&first_ay, 0u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&first_ay, 8u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&first_ay, 15u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&second_ay, 0u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&second_ay, 1u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&second_ay, 7u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&second_ay, 0u, 0u) != WZ_RESULT_OK ||
        wz_ay_select_register(&second_ay, 8u, 0u) != WZ_RESULT_OK ||
        wz_ay_write_data(&second_ay, 15u, 0u) != WZ_RESULT_OK) {
        fputs("AY PCM fixture register writes failed\n", stderr);
        exit(1);
    }
    for (size_t index = 0u; index < 64u; ++index) {
        first[index] = wz_audio_mixer_ay_sample(&first_ay);
        second[index] = wz_audio_mixer_ay_sample(&second_ay);
        if (wz_ay_advance_master_ticks(&first_ay, WZ_AY_MASTER_TICKS_PER_CLOCK) !=
                WZ_RESULT_OK ||
            wz_ay_advance_master_ticks(&second_ay, WZ_AY_MASTER_TICKS_PER_CLOCK) !=
                WZ_RESULT_OK) {
            fputs("AY PCM fixture advancement failed\n", stderr);
            exit(1);
        }
    }
    if (wz_audio_samples_hash(first, 64u, &first_hash) != WZ_RESULT_OK ||
        wz_audio_samples_hash(second, 64u, &second_hash) != WZ_RESULT_OK ||
        first_hash == 0u || first_hash != second_hash) {
        fputs("AY PCM hash repeatability failed\n", stderr);
        exit(1);
    }
    second[63u] += 1;
    if (wz_audio_samples_hash(second, 64u, &second_hash) != WZ_RESULT_OK ||
        first_hash == second_hash) {
        fputs("AY PCM hash sensitivity failed\n", stderr);
        exit(1);
    }
}

static void test_host_audio_push_queue(void)
{
    wz_host_audio_push_queue_t queue;
    wz_audio_sample_t input[3u] = {1, 2, 3};
    static const wz_audio_sample_t fill[WZ_HOST_AUDIO_QUEUE_CAPACITY] = {0};
    wz_audio_sample_t output[3u] = {0, 0, 0};

    wz_host_audio_push_init(&queue);
    if (wz_host_audio_push(&queue, input, 3u) != 3u ||
        wz_host_audio_queued(&queue) != 3u ||
        wz_host_audio_pop(&queue, output, 1u) != 1u ||
        output[0] != 1 ||
        wz_host_audio_push(&queue, fill, WZ_HOST_AUDIO_QUEUE_CAPACITY) !=
            WZ_HOST_AUDIO_QUEUE_CAPACITY - 2u ||
        wz_host_audio_dropped(&queue) != 2u) {
        fputs("host audio push queue contract failed\n", stderr);
        exit(1);
    }
}

static void test_canonical_audio_policy(void)
{
    if (WZ_CANONICAL_AUDIO_SAMPLE_RATE != 44100u ||
        WZ_AUDIO_MIXER_FRACTION_BITS != 16u ||
        sizeof(wz_audio_sample_t) != sizeof(int32_t) ||
        sizeof(wz_audio_accumulator_t) != sizeof(int64_t) ||
        WZ_AUDIO_MIXER_ONE != 65536) {
        fputs("canonical audio sample-rate policy failed\n", stderr);
        exit(1);
    }
}

static void test_host_audio_policy(void)
{
    if (wz_host_audio_enabled(WZ_SPEED_25) ||
        !wz_host_audio_enabled(WZ_SPEED_50) ||
        !wz_host_audio_enabled(WZ_SPEED_100) ||
        !wz_host_audio_enabled(WZ_SPEED_200) ||
        wz_host_audio_enabled(WZ_SPEED_400) ||
        wz_host_audio_enabled(WZ_SPEED_800) ||
        wz_host_audio_enabled(WZ_SPEED_UNLIMITED) ||
        wz_host_audio_enabled((wz_speed_policy_t)WZ_SPEED_COUNT)) {
        fputs("host audio policy contract failed\n", stderr);
        exit(1);
    }
}

static void test_host_audio_backpressure_isolation(void)
{
    wz_machine_t machine;
    wz_host_audio_push_queue_t queue;
    wz_audio_sample_t input[WZ_HOST_AUDIO_QUEUE_CAPACITY] = {0};
    wz_audio_sample_t output[WZ_HOST_AUDIO_QUEUE_CAPACITY] = {0};
    wz_qword_t before_hash;
    wz_qword_t after_hash;

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK ||
        wz_state_hash_machine(&machine, &before_hash) != WZ_RESULT_OK) {
        fputs("host audio backpressure setup failed\n", stderr);
        exit(1);
    }
    wz_host_audio_push_init(&queue);
    if (wz_host_audio_push(&queue, input, WZ_HOST_AUDIO_QUEUE_CAPACITY) !=
            WZ_HOST_AUDIO_QUEUE_CAPACITY ||
        wz_host_audio_push(&queue, input, 1u) != 0u ||
        wz_host_audio_dropped(&queue) != 1u ||
        wz_host_audio_pop(&queue, output, WZ_HOST_AUDIO_QUEUE_CAPACITY) !=
            WZ_HOST_AUDIO_QUEUE_CAPACITY ||
        wz_host_audio_pop(&queue, output, 1u) != 0u ||
        wz_state_hash_machine(&machine, &after_hash) != WZ_RESULT_OK ||
        before_hash != after_hash) {
        fputs("host audio backpressure isolation failed\n", stderr);
        exit(1);
    }
    wz_machine_destroy(&machine);
}

static void test_ay_mixer_policy(void)
{
    if (WZ_AY_CHANNEL_COUNT != 3u || WZ_AY_VOLUME_LEVEL_COUNT != 16u ||
        WZ_AY_VOLUME_GAIN_Q16_16[0] != 0u ||
        WZ_AY_VOLUME_GAIN_Q16_16[12u] != 65536u ||
        WZ_AY_VOLUME_GAIN_Q16_16[15u] != 65536u) {
        fputs("AY mixer policy contract failed\n", stderr);
        exit(1);
    }
}

static void test_raster_diagnostic(void)
{
    wz_byte_t expected_samples[6] = {0u, 1u, 2u, 3u, 4u, 5u};
    wz_byte_t actual_samples[6] = {0u, 1u, 9u, 3u, 4u, 5u};
    wz_raster_buffer_t expected = {expected_samples, 3u, 2u};
    wz_raster_buffer_t actual = {actual_samples, 3u, 2u};
    wz_raster_mismatch_t mismatch = {0};

    if (wz_raster_compare(&expected, &actual, 100u, 2u, &mismatch) != WZ_RESULT_OK ||
        mismatch.equal || mismatch.sample_index != 2u || mismatch.x != 2u ||
        mismatch.y != 0u || mismatch.expected != 2u || mismatch.actual != 9u ||
        mismatch.master_tick != 104u || actual_samples[2] != 9u) {
        fputs("raster diagnostic mismatch failed\n", stderr);
        exit(1);
    }
    actual_samples[2] = 2u;
    if (wz_raster_compare(&expected, &actual, 100u, 2u, &mismatch) != WZ_RESULT_OK ||
        !mismatch.equal || mismatch.sample_index != 6u) {
        fputs("raster diagnostic equality failed\n", stderr);
        exit(1);
    }
}

static void test_raster_invalid_state(void)
{
    wz_byte_t sample = 0xa5u;
    wz_raster_buffer_t overflow = {&sample, SIZE_MAX, 2u};
    wz_raster_buffer_t zero = {&sample, 0u, 1u};

    if (wz_raster_buffer_clear(&overflow, WZ_PALETTE_BLACK) != WZ_RESULT_INVALID_ARGUMENT ||
        wz_raster_buffer_write(&overflow, 0u, 0u, WZ_PALETTE_BLACK) != WZ_RESULT_INVALID_ARGUMENT ||
        wz_raster_buffer_read(&overflow, 0u, 0u, &sample) != WZ_RESULT_INVALID_ARGUMENT ||
        wz_raster_buffer_clear(&zero, WZ_PALETTE_BLACK) != WZ_RESULT_INVALID_ARGUMENT ||
        sample != 0xa5u) {
        fputs("raster invalid-state validation failed\n", stderr);
        exit(1);
    }
}

static void record_event(void* context)
{
    unsigned* value = (unsigned*)context;
    *value += 1u;
}

static void record_trace(const wz_trace_event_t* event, void* context)
{
    unsigned* count = (unsigned*)context;
    if (event->kind == WZ_TRACE_MASTER_TICK_ADVANCED) {
        *count += 1u;
    }
}

typedef struct {
    wz_trace_event_t events[16];
    size_t count;
} timing_trace_log_t;

static void record_timing_trace(const wz_trace_event_t* event, void* context)
{
    timing_trace_log_t* log = (timing_trace_log_t*)context;
    if (log->count < (sizeof(log->events) / sizeof(log->events[0]))) {
        log->events[log->count] = *event;
    }
    log->count += 1u;
}

static bool recover_trace(const wz_trace_event_t* event, void* context)
{
    wz_qword_t* last_sequence = (wz_qword_t*)context;
    *last_sequence = event->sequence;
    return true;
}

static bool recover_timing_trace(const wz_trace_event_t* event, void* context)
{
    record_timing_trace(event, context);
    return true;
}

typedef struct {
    wz_qword_t first_master_tick;
    wz_qword_t last_master_tick;
    size_t count;
    bool saw_bus;
    bool saw_instruction;
} retention_trace_log_t;

static bool recover_retention_trace(const wz_trace_event_t* event, void* context)
{
    retention_trace_log_t* log = (retention_trace_log_t*)context;
    if (log->count == 0u) {
        log->first_master_tick = event->master_tick;
    }
    log->last_master_tick = event->master_tick;
    log->saw_bus = log->saw_bus || event->kind == WZ_TRACE_CPU_BUS;
    log->saw_instruction = log->saw_instruction || event->kind == WZ_TRACE_CPU_INSTRUCTION;
    log->count += 1u;
    return true;
}

typedef struct {
    wz_bus_request_t requests[8];
    size_t count;
} bus_log_t;

static void record_bus_request(const wz_bus_request_t* request, void* context)
{
    bus_log_t* log = (bus_log_t*)context;
    if (log->count < (sizeof(log->requests) / sizeof(log->requests[0]))) {
        log->requests[log->count] = *request;
    }
    log->count += 1u;
}

static wz_byte_t* test_primary_register(wz_z80_state_t* state, wz_byte_t code)
{
    switch (code) {
    case 0u: return &state->main.b;
    case 1u: return &state->main.c;
    case 2u: return &state->main.d;
    case 3u: return &state->main.e;
    case 4u: return &state->main.h;
    case 5u: return &state->main.l;
    case 7u: return &state->main.a;
    default: return 0;
    }
}

static wz_byte_t read_bus_input(wz_bus_cycle_t cycle,
                                wz_word_t address,
                                void* context)
{
    const wz_byte_t* interrupt_value = (const wz_byte_t*)context;
    if (cycle == WZ_BUS_IO_READ) {
        return (wz_byte_t)(address >> 8u);
    }
    return *interrupt_value;
}

typedef struct {
    unsigned calls;
    wz_master_tick_t seen_tick;
    wz_byte_t seen_delay;
} data_source_fixture_t;

static bool claim_timed_data(const wz_bus_request_t* request,
                             wz_byte_t* value,
                             void* context)
{
    data_source_fixture_t* fixture = (data_source_fixture_t*)context;
    if (request->cycle != WZ_BUS_MEMORY_READ || request->address != 0x4000u) {
        return false;
    }
    fixture->calls += 1u;
    fixture->seen_tick = request->master_tick;
    fixture->seen_delay = request->contention_delay;
    *value = 0xa5u;
    return true;
}

typedef struct {
    wz_qword_t hash;
    wz_master_tick_t master_tick;
    wz_word_t program_counter;
    wz_byte_t accumulator;
    wz_byte_t flags;
    wz_byte_t memory_4000;
} differential_result_t;

static bool run_differential_program(wz_machine_t* machine,
                                     const wz_machine_profile_t* profile,
                                     const wz_byte_t* program,
                                     size_t program_length,
                                     wz_word_t load_address,
                                     unsigned steps,
                                     differential_result_t* result)
{
    if (wz_machine_init(machine, profile) != WZ_RESULT_OK ||
        result == 0 || program == 0 || program_length > sizeof(machine->memory) ||
        (size_t)load_address + program_length > sizeof(machine->memory)) {
        return false;
    }
    memcpy(&machine->memory[load_address], program, program_length);
    machine->cpu.program_counter = load_address;
    for (unsigned step = 0u; step < steps; ++step) {
        if (wz_z80_step(machine) != WZ_RESULT_OK) {
            return false;
        }
    }
    if (wz_state_hash_machine(machine, &result->hash) != WZ_RESULT_OK) {
        return false;
    }
    result->master_tick = machine->master_tick;
    result->program_counter = machine->cpu.program_counter;
    result->accumulator = machine->cpu.main.a;
    result->flags = machine->cpu.main.f;
    result->memory_4000 = machine->memory[0x4000u];
    return true;
}

static bool differential_results_equal(const differential_result_t* first,
                                       const differential_result_t* second)
{
    return first->hash == second->hash &&
           first->master_tick == second->master_tick &&
           first->program_counter == second->program_counter &&
           first->accumulator == second->accumulator &&
           first->flags == second->flags &&
           first->memory_4000 == second->memory_4000;
}

static wz_byte_t read_cpu_fixture_input(wz_bus_cycle_t cycle,
                                        wz_word_t address,
                                        void* context)
{
    const wz_byte_t* interrupt_value = (const wz_byte_t*)context;
    (void)address;
    return cycle == WZ_BUS_IO_READ ? 0xffu : *interrupt_value;
}

static wz_result_t set_fixture_bus_input(wz_machine_t* machine,
                                         wz_bus_input_t* input)
{
    if (wz_machine_set_hardware_io_decode(machine, false) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    return wz_machine_set_bus_input(machine, input);
}

static int test_wav_pcm_decoder(void)
{
    static const wz_byte_t wav[] = {
        'R','I','F','F', 40u,0u,0u,0u, 'W','A','V','E',
        'f','m','t',' ', 16u,0u,0u,0u, 1u,0u,1u,0u,
        0xe8u,0x03u,0u,0u, 0xe8u,0x03u,0u,0u, 1u,0u,8u,0u,
        'd','a','t','a', 4u,0u,0u,0u, 0u,255u,0u,255u
    };
    wz_tape_segment_t segments[4];
    size_t count = 0u;
    if (wz_tape_parse_wav_pcm(wav, sizeof(wav), 1000u, 128u, 0u,
                              0, 0u, &count) != WZ_RESULT_BUFFER_TOO_SMALL || count != 4u ||
        wz_tape_parse_wav_pcm(wav, sizeof(wav), 1000u, 128u, 0u,
                              segments, 4u, &count) != WZ_RESULT_OK ||
        count != 4u || segments[0].duration != 1u || segments[0].ear_level != 0u ||
        segments[1].duration != 1u || segments[1].ear_level != 1u ||
        segments[2].duration != 1u || segments[2].ear_level != 0u ||
        segments[3].duration != 1u || segments[3].ear_level != 1u) {
        fputs("WAV PCM edge decoding failed\n", stderr);
        return 1;
    }
    if (wz_tape_parse_wav_pcm(wav, sizeof(wav) - 1u, 1000u, 128u, 0u,
                              segments, 4u, &count) != WZ_RESULT_PARSE_ERROR) {
        fputs("WAV truncation rejection failed\n", stderr);
        return 1;
    }

    static const wz_byte_t three_channel_wav[] = {
        'R','I','F','F', 42u,0u,0u,0u, 'W','A','V','E',
        'f','m','t',' ', 16u,0u,0u,0u, 1u,0u,3u,0u,
        0xe8u,0x03u,0u,0u, 0xb8u,0x0bu,0u,0u, 3u,0u,8u,0u,
        'd','a','t','a', 6u,0u,0u,0u, 0u,0u,0u,255u,255u,255u
    };
    if (wz_tape_parse_wav_pcm(three_channel_wav, sizeof(three_channel_wav),
                              1000u, 128u, 0u, segments, 4u, &count) != WZ_RESULT_OK ||
        count != 2u || segments[0].duration != 1u || segments[0].ear_level != 0u ||
        segments[1].duration != 1u || segments[1].ear_level != 1u) {
        fputs("WAV multi-channel conversion failed\n", stderr);
        return 1;
    }

    wz_byte_t malformed_format[sizeof(wav)];
    memcpy(malformed_format, wav, sizeof(wav));
    malformed_format[28u] = 0u;
    if (wz_tape_parse_wav_pcm(malformed_format, sizeof(malformed_format),
                              1000u, 128u, 0u, segments, 4u, &count) !=
        WZ_RESULT_UNSUPPORTED_OPERATION) {
        fputs("WAV format metadata validation failed\n", stderr);
        return 1;
    }
    return 0;
}

int main(void)
{
    if (test_wav_pcm_decoder() != 0) {
        return 1;
    }
    test_raster_evidence();
    test_complete_keyboard_matrix();
    test_input_timestamp_assignment();
    test_input_focus_loss();
    test_kempston_decode();
    test_kempston_mapping();
    test_speed_policy();
    test_host_pacing();
    test_host_media_ownership();
    test_audio_speed_boundary_transitions();
    test_tape_object_and_state();
    test_machine_tape_playback();
    test_tape_loading_mode_state();
    test_networking_mode_state();
    test_zxnet_state_machine();
    test_networking_mode_cold_reconfiguration();
    test_interface1_rom_paging();
    test_interface1_machine_registers();
    test_microdrive_image_validation();
    test_historical_state_representability();
    test_snapshot_state_isolated_validation();
    test_sna_48k_loader();
    test_sna_48k_writer();
    test_sna_128k_image_scaffolding();
    test_sna_128k_machine_round_trip();
    test_z80_128k_machine_round_trip();
    test_z80_v1_loader();
    test_z80_v2_loader_and_writer();
    test_z80_v3_loader();
    test_snapshot_parser_fuzz_matrix();
    test_snapshot_cross_host_round_trips();
    test_128k_banked_memory_and_paging();
    test_128k_paging_all_bank_fixture();
    test_128k_ula_profile();
    test_128k_timing_relationships();
    test_128k_ay_register_timing();
    test_ay_tone_generators();
    test_ay_noise_generator();
    test_ay_envelope_generator();
    test_ay_audio_mixer();
    test_ay_state_round_trip();
    test_tape_trap_eligibility();
    test_tape_speed_invariance();
    test_standard_tap_parser();
    test_standard_tap_writer();
    test_native_tap_parser();
    test_native_tap_writer();
    test_tzx_parser();
    test_tzx_timing();
    test_beeper_port_fe_timeline();
    test_mic_capture_timeline();
    test_beeper_pcm_render();
    test_audio_pcm_hash();
    test_ay_pcm_hash();
    test_host_audio_push_queue();
    test_canonical_audio_policy();
    test_host_audio_policy();
    test_host_audio_backpressure_isolation();
    test_ay_mixer_policy();
    test_raster_diagnostic();
    test_raster_invalid_state();
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    wz_machine_t machine;
    wz_machine_t restored;
    wz_scheduler_t scheduler;
    wz_byte_t serialized[WZ_STATE_SNAPSHOT_CAPACITY];
    wz_machine_profile_t certified_profile;
    wz_state_writer_t writer;
    wz_qword_t first_hash;
    wz_qword_t second_hash;
    wz_trace_sink_t trace_sink;
    wz_headless_runner_t runner;
    unsigned trace_count = 0u;
    unsigned dispatched = 0u;
    wz_trace_file_t trace_file;
    wz_trace_file_t duplicate;
    wz_bus_observer_t bus_observer;
    wz_bus_input_t bus_input;
    wz_bus_data_source_t bus_data_source;
    wz_bus_request_t bus_request;
    bus_log_t bus_log;
    data_source_fixture_t data_source_fixture;
    timing_trace_log_t timing_trace_log;
    retention_trace_log_t retention_trace_log;
    wz_trace_cpu_state_sync_t recovered_cpu_sync;
    wz_qword_t recovered_last = 0u;
    size_t recovered_count = 0u;
    size_t opcode_index;
    size_t documented_unimplemented = 0u;
    size_t implemented = 0u;
    size_t prefix = 0u;
    size_t undocumented = 0u;
    size_t illegal = 0u;
    size_t cb_documented_unimplemented = 0u;
    size_t cb_implemented = 0u;
    size_t cb_undocumented = 0u;
    size_t ed_documented_unimplemented = 0u;
    size_t ed_implemented = 0u;
    size_t ed_undocumented = 0u;
    wz_byte_t interrupt_value = 0x5au;
    FILE* trace_stream;
    const char* trace_path = "wz-trace-regression.bin";
    const char* failing_trace_path = "wz-trace-failing-opcode.bin";
    const char* state_trace_path = "wz-trace-state-regression.bin";
    const char* timing_full_trace_path = "wz-trace-timing-full-retention.bin";

    {
        static const wz_byte_t expected[] = {
            WZ_PALETTE_BLACK, WZ_PALETTE_BLUE, WZ_PALETTE_RED,
            WZ_PALETTE_MAGENTA, WZ_PALETTE_GREEN, WZ_PALETTE_CYAN,
            WZ_PALETTE_YELLOW, WZ_PALETTE_WHITE,
            WZ_PALETTE_BRIGHT_BLACK, WZ_PALETTE_BRIGHT_BLUE,
            WZ_PALETTE_BRIGHT_RED, WZ_PALETTE_BRIGHT_MAGENTA,
            WZ_PALETTE_BRIGHT_GREEN, WZ_PALETTE_BRIGHT_CYAN,
            WZ_PALETTE_BRIGHT_YELLOW, WZ_PALETTE_BRIGHT_WHITE
        };
        wz_byte_t sample = 0u;
        for (wz_byte_t color = 0u; color < 8u; ++color) {
            if (wz_raster_palette_index(color, false, &sample) != WZ_RESULT_OK ||
                sample != expected[color] ||
                wz_raster_palette_index(color, true, &sample) != WZ_RESULT_OK ||
                sample != expected[(size_t)color + 8u]) {
                fputs("canonical palette table contract failed\n", stderr);
                return 1;
            }
        }
        if (wz_raster_palette_index(8u, false, &sample) != WZ_RESULT_INVALID_ARGUMENT ||
            wz_raster_palette_index(0u, false, 0) != WZ_RESULT_INVALID_ARGUMENT) {
            fputs("canonical palette invalid-input contract failed\n", stderr);
            return 1;
        }
    }

    {
        static const wz_byte_t load_store_program[] = {
            0x3eu, 0x42u, 0x32u, 0x00u, 0x40u, 0x3au, 0x00u, 0x40u
        };
        static const wz_byte_t immediate_add_program[] = {
            0x3eu, 0x3cu, 0xc6u, 0x42u
        };
        differential_result_t first_result;
        differential_result_t second_result;
        if (!run_differential_program(&machine, profile, load_store_program,
                                      sizeof(load_store_program), 0x2000u, 3u,
                                      &first_result) ||
            !run_differential_program(&machine, profile, load_store_program,
                                      sizeof(load_store_program), 0x2000u, 3u,
                                      &second_result) ||
            !differential_results_equal(&first_result, &second_result) ||
            first_result.accumulator != 0x42u || first_result.memory_4000 != 0x42u) {
            fputs("load/store differential scenario failed\n", stderr);
            return 1;
        }
        if (!run_differential_program(&machine, profile, immediate_add_program,
                                      sizeof(immediate_add_program), 0x2000u, 2u,
                                      &first_result) ||
            !run_differential_program(&machine, profile, immediate_add_program,
                                      sizeof(immediate_add_program), 0x2000u, 2u,
                                      &second_result) ||
            !differential_results_equal(&first_result, &second_result) ||
            first_result.accumulator != 0x7eu) {
            fputs("immediate-add differential scenario failed\n", stderr);
            return 1;
        }
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine initialization failed\n", stderr);
        return 1;
    }
    if (wz_machine_set_kempston_control(&machine, WZ_KEMPSTON_RIGHT, true) != WZ_RESULT_OK ||
        wz_machine_set_kempston_control(&machine, WZ_KEMPSTON_FIRE, true) != WZ_RESULT_OK ||
        wz_machine_kempston_read(&machine, 0x1fu) != 0x14u ||
        wz_machine_kempston_read(&machine, 0x1eu) != 0u ||
        wz_machine_kempston_read(&machine, 0x201fu) != 0x14u) {
        fputs("machine Kempston integration failed\n", stderr);
        return 1;
    }
    {
        wz_bus_request_t kempston_request;
        wz_bus_request_init(&kempston_request, WZ_BUS_IO_READ, 0u, 0x001fu,
                            0u, 4u);
        if (wz_machine_bus_request(&machine, &kempston_request) != WZ_RESULT_OK ||
            kempston_request.value != 0x14u ||
            kempston_request.source != WZ_BUS_SOURCE_INPUT) {
            fputs("machine Kempston bus dispatch failed\n", stderr);
            return 1;
        }
        wz_bus_request_init(&kempston_request, WZ_BUS_IO_READ, 0u, 0x201fu,
                            0u, 4u);
        if (wz_machine_bus_request(&machine, &kempston_request) != WZ_RESULT_OK ||
            kempston_request.value != 0x14u ||
            kempston_request.source != WZ_BUS_SOURCE_INPUT) {
            fputs("machine Kempston alias decode failed\n", stderr);
            return 1;
        }
        machine.cpu.program_counter = 0u;
        machine.memory[0u] = 0xdbu;
        machine.memory[1u] = WZ_KEMPSTON_PORT;
        machine.cpu.main.a = 0u;
        if (wz_z80_step(&machine) != WZ_RESULT_OK ||
            machine.cpu.main.a != 0x14u) {
            fputs("machine-code Kempston port read failed\n", stderr);
            return 1;
        }
        if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
            fputs("machine reinitialization after Kempston test failed\n", stderr);
            return 1;
        }
    }
    if (!wz_raster_sample_is_valid(0x00u) ||
        !wz_raster_sample_is_valid(WZ_RASTER_BORDER_MAX) ||
        !wz_raster_sample_is_valid(WZ_RASTER_BLANKING) ||
        wz_raster_sample_is_valid(0x19u) ||
        !wz_raster_sample_is_active(0x0fu) ||
        !wz_raster_sample_is_border(0x10u) ||
        wz_raster_sample_is_border(WZ_RASTER_BLANKING)) {
        fputs("canonical raster sample encoding contract failed\n", stderr);
        return 1;
    }
    {
        wz_byte_t sample = 0u;
        for (unsigned attribute = 0u; attribute <= 0xffu; ++attribute) {
            wz_byte_t expected_ink = (wz_byte_t)((attribute & 0x07u) +
                ((attribute & 0x40u) != 0u ? 8u : 0u));
            wz_byte_t expected_paper = (wz_byte_t)(((attribute >> 3u) & 0x07u) +
                ((attribute & 0x40u) != 0u ? 8u : 0u));
            if (wz_raster_decode_attribute((wz_byte_t)attribute, true,
                                            &sample) != WZ_RESULT_OK ||
                sample != expected_ink ||
                wz_raster_decode_attribute((wz_byte_t)attribute, false,
                                            &sample) != WZ_RESULT_OK ||
                sample != expected_paper) {
                fputs("exhaustive attribute interpretation failed\n", stderr);
                return 1;
            }
        }
        for (wz_byte_t color = 0u; color < 8u; ++color) {
            wz_byte_t attribute = (wz_byte_t)(color | ((7u - color) << 3u));
            if (wz_raster_decode_active_pixel(0x80u, attribute, 0u,
                                              &sample) != WZ_RESULT_OK ||
                sample != color ||
                wz_raster_decode_active_pixel(0x00u, attribute, 0u,
                                              &sample) != WZ_RESULT_OK ||
                sample != (wz_byte_t)(7u - color) ||
                wz_raster_decode_active_pixel(0x01u,
                                              (wz_byte_t)(attribute | 0x40u),
                                              7u, &sample) != WZ_RESULT_OK ||
                sample != (wz_byte_t)(color + 8u)) {
                fputs("active-pixel color table contract failed\n", stderr);
                return 1;
            }
        }
        if (wz_raster_decode_active_pixel(0u, 0u, 8u, &sample) !=
                WZ_RESULT_INVALID_ARGUMENT ||
            wz_raster_decode_active_pixel(0u, 0u, 0u, 0) !=
                WZ_RESULT_INVALID_ARGUMENT) {
            fputs("active-pixel invalid argument contract failed\n", stderr);
            return 1;
        }
        if (wz_machine_flash_phase(&machine, 0u) ||
            !wz_machine_flash_phase(&machine,
                (wz_master_tick_t)profile->tstates_per_frame * 2u * 16u) ||
            wz_raster_decode_attribute_phase(0x87u, true, false, &sample) !=
                WZ_RESULT_OK || sample != 7u ||
            wz_raster_decode_attribute_phase(0x87u, true, true, &sample) !=
                WZ_RESULT_OK || sample != 0u) {
            fputs("deterministic FLASH phase contract failed\n", stderr);
            return 1;
        }
    }
    {
        wz_raster_buffer_t raster_buffer;
        wz_byte_t storage[4u];
        wz_byte_t sample = 0u;

        if (wz_raster_buffer_init(&raster_buffer, 2u, 2u, storage,
                                  sizeof(storage)) != WZ_RESULT_OK ||
            wz_raster_buffer_read(&raster_buffer, 1u, 1u, &sample) !=
                WZ_RESULT_OK || sample != WZ_RASTER_BLANKING ||
            wz_raster_buffer_write(&raster_buffer, 1u, 1u,
                                   WZ_RASTER_BORDER_MIN) != WZ_RESULT_OK ||
            wz_raster_buffer_read(&raster_buffer, 1u, 1u, &sample) !=
                WZ_RESULT_OK || sample != WZ_RASTER_BORDER_MIN ||
            wz_raster_buffer_write(&raster_buffer, 2u, 0u, 0u) !=
                WZ_RESULT_INVALID_ARGUMENT ||
            wz_raster_buffer_write(&raster_buffer, 0u, 0u, 0x19u) !=
                WZ_RESULT_INVALID_ARGUMENT ||
            wz_raster_buffer_clear(&raster_buffer, 0x19u) !=
                WZ_RESULT_INVALID_ARGUMENT) {
            fputs("canonical raster buffer contract failed\n", stderr);
            return 1;
        }
    }
    if (machine.master_tick != 0u || machine.profile != profile) {
        fputs("machine did not initialize deterministic state\n", stderr);
        return 1;
    }
    if (wz_profile_cpu_tstate(5u, profile) != 2u ||
        wz_profile_cpu_phase(5u, profile) != 1u) {
        fputs("master-tick conversion failed\n", stderr);
        return 1;
    }
    {
        wz_raster_position_t raster_position;
        if (wz_machine_raster_position(0, &raster_position) !=
                WZ_RESULT_INVALID_ARGUMENT ||
            wz_machine_raster_position(&machine, 0) !=
                WZ_RESULT_INVALID_ARGUMENT ||
            wz_machine_raster_position(&machine, &raster_position) != WZ_RESULT_OK ||
            raster_position.frame_number != 0u ||
            raster_position.frame_raster_clock != 0u ||
            raster_position.line != 0u || raster_position.raster_clock != 0u) {
            fputs("initial raster coordinate contract failed\n", stderr);
            return 1;
        }
        machine.master_tick = 447u;
        if (wz_machine_raster_position(&machine, &raster_position) != WZ_RESULT_OK ||
            raster_position.line != 0u || raster_position.raster_clock != 447u) {
            fputs("raster line boundary failed\n", stderr);
            return 1;
        }
        machine.master_tick = 448u;
        if (wz_machine_raster_position(&machine, &raster_position) != WZ_RESULT_OK ||
            raster_position.line != 1u || raster_position.raster_clock != 0u ||
            raster_position.frame_raster_clock != 448u) {
            fputs("raster line transition failed\n", stderr);
            return 1;
        }
        machine.master_tick = 139775u;
        if (wz_machine_raster_position(&machine, &raster_position) != WZ_RESULT_OK ||
            raster_position.frame_number != 0u || raster_position.line != 311u ||
            raster_position.raster_clock != 447u) {
            fputs("raster frame boundary failed\n", stderr);
            return 1;
        }
        machine.master_tick = 139776u;
        if (wz_machine_raster_position(&machine, &raster_position) != WZ_RESULT_OK ||
            raster_position.frame_number != 1u || raster_position.frame_raster_clock != 0u ||
            raster_position.line != 0u || raster_position.raster_clock != 0u) {
            fputs("raster frame wrap failed\n", stderr);
            return 1;
        }
        machine.master_tick = 0u;
    }
    {
        wz_raster_position_t before_active;
        wz_raster_position_t active_start;
        wz_border_event_t transitions[2u];
        wz_ula_fetch_event_t fetches[2u];
        size_t fetch_count = 0u;
        size_t transition_count;
        const wz_master_tick_t first_fetch_tick = 14335u * 2u;

        if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
            fputs("overscan transition fixture reset failed\n", stderr);
            return 1;
        }
        machine.master_tick = 447u;
        if (wz_machine_raster_position(&machine, &before_active) != WZ_RESULT_OK ||
            before_active.line != 0u || before_active.raster_clock != 447u) {
            fputs("overscan pre-active edge contract failed\n", stderr);
            return 1;
        }
        machine.master_tick = 448u;
        if (wz_machine_raster_position(&machine, &active_start) != WZ_RESULT_OK ||
            active_start.line != 1u || active_start.raster_clock != 0u ||
            active_start.frame_raster_clock != 448u) {
            fputs("overscan active transition contract failed\n", stderr);
            return 1;
        }
        wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x01u, 447u);
        wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x06u, 448u);
        transition_count = wz_machine_border_events(
            &machine, transitions, sizeof(transitions) / sizeof(transitions[0u]));
        if (transition_count < 2u || transitions[0u].master_tick != 447u ||
            transitions[0u].color != 1u || transitions[1u].master_tick != 448u ||
            transitions[1u].color != 6u ||
            wz_machine_ula_fetches_at_tick(&machine, first_fetch_tick,
                                           fetches, 2u, &fetch_count) != WZ_RESULT_OK ||
            fetch_count != 2u || fetches[0u].master_tick != first_fetch_tick ||
            fetches[1u].master_tick != first_fetch_tick + 2u) {
            fputs("overscan border-to-fetch ordering contract failed\n", stderr);
            return 1;
        }
        if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
            fputs("overscan transition fixture cleanup failed\n", stderr);
            return 1;
        }
    }
    {
        wz_ula_fetch_event_t fetches[2u];
        size_t fetch_count = 0u;
        wz_master_tick_t first_fetch_tick = 14335u * 2u;

        machine.memory[0x4000u] = 0xa5u;
        machine.memory[0x5800u] = 0x1cu;
        if (wz_machine_ula_fetches_at_tick(&machine, first_fetch_tick - 2u,
                                           fetches, 2u, &fetch_count) !=
                WZ_RESULT_OK || fetch_count != 0u ||
            wz_machine_ula_fetches_at_tick(&machine,
                                           (14335u - 1u) * 2u,
                                           fetches, 2u, &fetch_count) !=
                WZ_RESULT_OK || fetch_count != 0u ||
            wz_machine_ula_fetches_at_tick(&machine, first_fetch_tick,
                                           fetches, 2u, &fetch_count) !=
                WZ_RESULT_OK || fetch_count != 2u ||
            fetches[0].kind != WZ_ULA_FETCH_BITMAP ||
            fetches[0].master_tick != first_fetch_tick ||
            fetches[0].address != 0x4000u || fetches[0].value != 0xa5u ||
            fetches[1].kind != WZ_ULA_FETCH_ATTRIBUTE ||
            fetches[1].master_tick != first_fetch_tick + 2u ||
            fetches[1].address != 0x5800u || fetches[1].value != 0x1cu ||
            wz_machine_ula_fetches_at_tick(&machine, first_fetch_tick,
                                           fetches, 1u, &fetch_count) !=
                WZ_RESULT_BUFFER_TOO_SMALL) {
            fputs("first ULA bitmap/attribute fetch contract failed\n", stderr);
            return 1;
        }
        machine.memory[0x57ffu] = 0x3cu;
        machine.memory[0x5affu] = 0x47u;
        if (wz_machine_ula_fetches_at_tick(&machine,
                                           (14335u + 191u * 224u + 31u * 4u) * 2u,
                                           fetches, 2u, &fetch_count) !=
                WZ_RESULT_OK || fetch_count != 2u ||
            fetches[0].address != 0x57ffu || fetches[0].value != 0x3cu ||
            fetches[1].address != 0x5affu || fetches[1].value != 0x47u) {
            fputs("last ULA bitmap/attribute fetch contract failed\n", stderr);
            return 1;
        }
        if (wz_machine_ula_fetches_at_tick(&machine,
                                           (14335u + 32u * 4u) * 2u,
                                           fetches, 2u, &fetch_count) !=
                WZ_RESULT_OK || fetch_count != 0u ||
            wz_machine_ula_fetches_at_tick(&machine,
                                           (14335u + 128u + 224u) * 2u,
                                           fetches, 2u, &fetch_count) !=
                WZ_RESULT_OK || fetch_count != 0u) {
            fputs("ULA blanking/non-display fetch contract failed\n", stderr);
            return 1;
        }
        machine.memory[0x4000u] = 0x11u;
        wz_machine_memory_write_at_tick(&machine, 0x4000u, 0x22u,
                                        first_fetch_tick - 2u);
        if (wz_machine_ula_fetches_at_tick(&machine, first_fetch_tick,
                                           fetches, 2u, &fetch_count) !=
                WZ_RESULT_OK || fetches[0].value != 0x22u) {
            fputs("CPU-before-ULA visibility contract failed\n", stderr);
            return 1;
        }
        machine.memory[0x5800u] = 0x33u;
        wz_machine_memory_write_at_tick(&machine, 0x5800u, 0x44u,
                                        first_fetch_tick);
        if (wz_machine_ula_fetches_at_tick(&machine, first_fetch_tick,
                                           fetches, 2u, &fetch_count) !=
                WZ_RESULT_OK || fetches[1].value != 0x44u) {
            fputs("same-edge CPU-before-ULA visibility contract failed\n", stderr);
            return 1;
        }
        if (wz_machine_floating_bus_value(&machine, first_fetch_tick) != 0x22u ||
            wz_machine_floating_bus_value(&machine, first_fetch_tick + 2u) != 0x44u ||
            wz_machine_floating_bus_value(&machine, first_fetch_tick - 2u) != 0xffu ||
            wz_machine_floating_bus_value(&machine,
                                          (14335u + 32u * 4u) * 2u) != 0xffu) {
            fputs("timed floating-bus contract failed\n", stderr);
            return 1;
        }
        wz_machine_memory_write_at_tick(&machine, 0x5800u, 0x55u,
                                        first_fetch_tick + 2u);
        if (fetches[1].value != 0x44u) {
            fputs("post-fetch visibility contract failed\n", stderr);
            return 1;
        }
        machine.master_tick = 0u;
    }
    {
        wz_border_event_t border_events[4u];
        size_t border_count;

        if (wz_machine_border_color(&machine) != 0u ||
            wz_machine_border_events(&machine, border_events,
                                     sizeof(border_events) /
                                     sizeof(border_events[0u])) != 0u) {
            fputs("initial border state contract failed\n", stderr);
            return 1;
        }
        wz_machine_ula_port_fe_write(&machine, 0x00feu, 0xe5u, 1000u);
        wz_machine_ula_port_fe_write(&machine, 0x00feu, 0x1au, 1004u);
        border_count = wz_machine_border_events(&machine, border_events,
                                                sizeof(border_events) /
                                                sizeof(border_events[0u]));
        if (wz_machine_border_color(&machine) != 2u || border_count != 2u ||
            border_events[0u].master_tick != 1000u ||
            border_events[0u].color != 5u ||
            border_events[1u].master_tick != 1004u ||
            border_events[1u].color != 2u) {
            fputs("timed border transition contract failed\n", stderr);
            return 1;
        }
        if (wz_machine_ula_port_fe_read(&machine, 0x00feu) == 0u) {
            fputs("border state incorrectly replaced ULA input state\n", stderr);
            return 1;
        }
    }
    {
        wz_ula_fetch_event_t fetches[2u];
        size_t fetch_count = 0u;
        const wz_master_tick_t fetch_tick = 14335u * 2u;

        if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
            fputs("multicolor timing fixture reset failed\n", stderr);
            return 1;
        }
        machine.memory[0x4000u] = 0x00u;
        machine.memory[0x5800u] = 0x00u;
        wz_machine_memory_write_at_tick(&machine, 0x4000u, 0xffu,
                                        fetch_tick - 2u);
        wz_machine_memory_write_at_tick(&machine, 0x5800u, 0x47u,
                                        fetch_tick);
        if (wz_machine_ula_fetches_at_tick(&machine, fetch_tick, fetches, 2u,
                                           &fetch_count) != WZ_RESULT_OK ||
            fetch_count != 2u || fetches[0u].value != 0xffu ||
            fetches[1u].value != 0x47u) {
            fputs("multicolor pre-fetch visibility contract failed\n", stderr);
            return 1;
        }
        wz_machine_memory_write_at_tick(&machine, 0x4000u, 0x11u,
                                        fetch_tick + 2u);
        wz_machine_memory_write_at_tick(&machine, 0x5800u, 0x22u,
                                        fetch_tick + 4u);
        if (fetches[0u].value != 0xffu || fetches[1u].value != 0x47u) {
            fputs("multicolor post-fetch non-retroactivity failed\n", stderr);
            return 1;
        }
        machine.master_tick = 0u;
    }
    if (wz_machine_memory_read(0, 0u) != 0xffu ||
        wz_machine_ula_port_fe_read(0, 0u) != 0xffu ||
        wz_machine_bus_request(0, 0) != WZ_RESULT_INVALID_ARGUMENT ||
        wz_machine_bus_request(&machine, 0) != WZ_RESULT_INVALID_ARGUMENT) {
        fputs("null memory or I/O safety contract failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, (wz_bus_cycle_t)255u,
                        UINT64_MAX, 0xffffu, 0xa5u, 0xffu);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_INVALID_ARGUMENT) {
        fputs("invalid bus cycle safety contract failed\n", stderr);
        return 1;
    }
    for (wz_dword_t address = 0u; address <= 0xffffu; ++address) {
        wz_word_t word_address = (wz_word_t)address;
        wz_machine_memory_write(&machine, word_address, (wz_byte_t)address);
        (void)wz_machine_contention_delay(&machine, WZ_BUS_MEMORY_READ,
                                          word_address, UINT64_MAX, 0xffu);
        if (wz_machine_memory_read(&machine, word_address) != (wz_byte_t)address) {
            fputs("full-range memory safety fixture failed\n", stderr);
            return 1;
        }
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset after safety fixture failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK) {
        fputs("bus observer installation failed\n", stderr);
        return 1;
    }
    machine.memory[0x1234u] = 0x9au;
    wz_bus_request_init(&bus_request, WZ_BUS_M1_OPCODE_FETCH, 4u, 0x1234u, 0u, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0x9au) {
        fputs("bus opcode fetch failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_MEMORY_WRITE, 8u, 0x4000u, 0x5cu, 3u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        machine.memory[0x4000u] != 0x5cu) {
        fputs("bus memory write failed\n", stderr);
        return 1;
    }
    memset(&data_source_fixture, 0, sizeof(data_source_fixture));
    wz_bus_data_source_init(&bus_data_source, claim_timed_data,
                            &data_source_fixture);
    if (wz_machine_set_bus_data_source(&machine, &bus_data_source) != WZ_RESULT_OK) {
        fputs("bus data source installation failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_MEMORY_READ, 12u, 0x4000u, 0u, 3u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0xa5u ||
        bus_request.direction != WZ_BUS_DIRECTION_READ ||
        bus_request.source != WZ_BUS_SOURCE_DATA_SOURCE ||
        data_source_fixture.calls != 1u ||
        data_source_fixture.seen_tick != bus_request.master_tick ||
        data_source_fixture.seen_delay != bus_request.contention_delay) {
        fputs("timed bus data source failed\n", stderr);
        return 1;
    }
    if (wz_machine_set_bus_data_source(&machine, 0) != WZ_RESULT_OK) {
        fputs("bus data source removal failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_request_init(&bus_request, WZ_BUS_IO_READ, 12u, 0x00feu, 0u, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0xbfu) {
        fputs("bus I/O read failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_INTERRUPT_ACKNOWLEDGE, 16u, 0xffffu, 0u, 7u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0xffu) {
        fputs("bus interrupt acknowledge failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_INTERNAL, 20u, 0u, 0u, 1u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_log.count != 3u ||
        bus_log.requests[0].cycle != WZ_BUS_IO_READ ||
        bus_log.requests[0].direction != WZ_BUS_DIRECTION_READ ||
        bus_log.requests[0].source != WZ_BUS_SOURCE_ULA ||
        bus_log.requests[1].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].direction != WZ_BUS_DIRECTION_READ ||
        bus_log.requests[1].source != WZ_BUS_SOURCE_FALLBACK ||
        bus_log.requests[2].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[2].direction != WZ_BUS_DIRECTION_NONE ||
        bus_log.requests[2].source != WZ_BUS_SOURCE_NONE) {
        fputs("mock bus did not record exact requests\n", stderr);
        return 1;
    }
    if (wz_machine_set_bus_observer(&machine, 0) != WZ_RESULT_OK) {
        fputs("bus observer removal failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_MEMORY_WRITE, 24u, 0x1234u, 0x5au, 3u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        machine.memory[0x1234u] != 0x5au) {
        fputs("unloaded fixture memory write failed\n", stderr);
        return 1;
    }
    {
        static wz_byte_t rom_image[WZ_48K_ROM_SIZE];

        memset(rom_image, 0, sizeof(rom_image));
        rom_image[0x1234u] = 0x5au;
        if (wz_machine_rom_identity(rom_image, sizeof(rom_image) - 1u) != 0u ||
            wz_machine_load_48k_rom(&machine, rom_image, sizeof(rom_image)) !=
                WZ_RESULT_INVALID_PROFILE) {
            fputs("unconfigured 48K ROM certification failed\n", stderr);
            return 1;
        }
        certified_profile = *profile;
        certified_profile.expected_rom_identity =
            wz_machine_rom_identity(rom_image, sizeof(rom_image));
        if (certified_profile.expected_rom_identity != UINT64_C(0x5bb7495d13c5113f) ||
            wz_machine_init(&machine, &certified_profile) != WZ_RESULT_OK) {
            fputs("48K ROM certification profile failed\n", stderr);
            return 1;
        }
        rom_image[0u] ^= 1u;
        if (wz_machine_load_48k_rom(&machine, rom_image, sizeof(rom_image)) !=
                WZ_RESULT_ROM_IDENTITY_MISMATCH ||
            machine.has_48k_rom != 0u || machine.rom_identity != 0u) {
            fputs("48K ROM mismatch rejection failed\n", stderr);
            return 1;
        }
        rom_image[0u] ^= 1u;
        if (wz_machine_load_48k_rom(&machine, rom_image, sizeof(rom_image)) != WZ_RESULT_OK ||
            machine.rom_identity != certified_profile.expected_rom_identity ||
            wz_machine_load_48k_rom(&machine, rom_image, sizeof(rom_image) - 1u) !=
                WZ_RESULT_INVALID_ARGUMENT) {
            fputs("48K ROM loading contract failed\n", stderr);
            return 1;
        }
        wz_bus_request_init(&bus_request, WZ_BUS_MEMORY_READ, 24u, 0x1234u, 0u, 3u);
        if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
            bus_request.value != 0x5au) {
            fputs("48K ROM bus read failed\n", stderr);
            return 1;
        }
        wz_bus_request_init(&bus_request, WZ_BUS_MEMORY_WRITE, 30u, 0x1234u, 0xa5u, 3u);
        if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
            machine.memory[0x1234u] != 0x5au) {
            fputs("48K ROM write protection failed\n", stderr);
            return 1;
        }
        wz_bus_request_init(&bus_request, WZ_BUS_MEMORY_WRITE, 36u, 0x4000u, 0xa5u, 3u);
        if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
            machine.memory[0x4000u] != 0xa5u) {
            fputs("48K RAM bus write failed\n", stderr);
            return 1;
        }
        wz_state_writer_init(&writer, serialized, sizeof(serialized));
        if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK ||
            wz_state_deserialize_machine(&restored, serialized, writer.length) != WZ_RESULT_OK ||
            restored.has_48k_rom != 1u ||
            restored.rom_identity != certified_profile.expected_rom_identity) {
            fputs("48K ROM certification state round trip failed\n", stderr);
            return 1;
        }
        wz_machine_memory_write(&restored, 0x1234u, 0xa5u);
        if (restored.memory[0x1234u] != 0x5au) {
            fputs("restored 48K ROM write protection failed\n", stderr);
            return 1;
        }
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset after 48K ROM certification test failed\n", stderr);
        return 1;
    }
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    wz_bus_request_init(&bus_request, WZ_BUS_IO_READ, 24u, 0x34ffu, 0u, 4u);
    if (wz_machine_set_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0x34u) {
        fputs("bus input provider failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_IO_READ, 28u, 0x34feu, 0u, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0xbfu) {
        fputs("ULA partial port-FE read decode failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_IO_READ, 29u, 0xfffeu, 0u, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0xbfu) {
        fputs("no-row keyboard read failed\n", stderr);
        return 1;
    }
    if (wz_machine_set_keyboard_key(&machine, 0u, 0u, true) != WZ_RESULT_OK) {
        fputs("keyboard key press failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_IO_READ, 30u, 0xfefeu, 0u, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0xbeu) {
        fputs("keyboard row read failed\n", stderr);
        return 1;
    }
    if (wz_machine_set_keyboard_key(&machine, 3u, 4u, true) != WZ_RESULT_OK) {
        fputs("second keyboard key press failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_IO_READ, 32u, 0xf6feu, 0u, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0xaeu) {
        fputs("simultaneous keyboard row read failed\n", stderr);
        return 1;
    }
    if (wz_machine_set_keyboard_key(&machine, 0u, 0u, false) != WZ_RESULT_OK ||
        wz_machine_set_keyboard_key(&machine, 3u, 4u, false) != WZ_RESULT_OK) {
        fputs("keyboard key release failed\n", stderr);
        return 1;
    }
    if (wz_machine_set_hardware_io_decode(&machine, false) != WZ_RESULT_OK) {
        fputs("fixture I/O mode setup failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_IO_READ, 30u, 0x34feu, 0u, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != 0x34u) {
        fputs("fixture I/O mode failed\n", stderr);
        return 1;
    }
    if (wz_machine_set_hardware_io_decode(&machine, true) != WZ_RESULT_OK) {
        fputs("hardware I/O mode restore failed\n", stderr);
        return 1;
    }
    if (wz_machine_set_bus_input(&machine, 0) != WZ_RESULT_OK) {
        fputs("unsupported I/O fixture setup failed\n", stderr);
        return 1;
    }
    for (wz_dword_t port = 0u; port <= 0xffffu; ++port) {
        wz_word_t address = (wz_word_t)port;
        wz_bus_request_init(&bus_request, WZ_BUS_IO_READ, port, address, 0u, 4u);
        if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
            bus_request.direction != WZ_BUS_DIRECTION_READ ||
            bus_request.source != (address % 2u == 0u
                                       ? WZ_BUS_SOURCE_ULA
                                       : (address & 0xffu) == WZ_KEMPSTON_PORT
                                           ? WZ_BUS_SOURCE_INPUT
                                           : WZ_BUS_SOURCE_FALLBACK) ||
            (address % 2u != 0u && (address & 0xffu) != WZ_KEMPSTON_PORT &&
             bus_request.value != 0xffu) ||
            ((address & 0xffu) == WZ_KEMPSTON_PORT &&
             bus_request.value != wz_machine_kempston_read(&machine, address))) {
            fputs("full-range unsupported I/O read failed\n", stderr);
            return 1;
        }
        wz_bus_request_init(&bus_request, WZ_BUS_IO_WRITE, port, address,
                            (wz_byte_t)port, 4u);
        if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
            bus_request.direction != WZ_BUS_DIRECTION_WRITE ||
            bus_request.source != (address % 2u == 0u
                                       ? WZ_BUS_SOURCE_ULA
                                       : WZ_BUS_SOURCE_FALLBACK)) {
            fputs("full-range unsupported I/O write failed\n", stderr);
            return 1;
        }
    }
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    if (wz_machine_set_bus_input(&machine, &bus_input) != WZ_RESULT_OK) {
        fputs("unsupported I/O fixture restore failed\n", stderr);
        return 1;
    }
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    wz_trace_sink_init(&trace_sink, record_timing_trace, &timing_trace_log);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    wz_bus_request_init(&bus_request, WZ_BUS_IO_WRITE, 32u, 0x12feu, 0xa5u, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK) {
        fputs("ULA partial port-FE write decode failed\n", stderr);
        return 1;
    }
    if (machine.ula_output != 0x05u || machine.ula_output_tick != 32u ||
        timing_trace_log.count != 2u ||
        timing_trace_log.events[0].kind != WZ_TRACE_DEVELOPER_MARKER ||
        timing_trace_log.events[0].master_tick != 32u ||
        timing_trace_log.events[0].address != 0x00feu ||
        timing_trace_log.events[0].value != 0x05u ||
        timing_trace_log.events[0].auxiliary != 0x01u ||
        timing_trace_log.events[1].kind != WZ_TRACE_CPU_BUS ||
        timing_trace_log.events[1].master_tick != 32u ||
        timing_trace_log.events[1].address != 0x12feu) {
        fputs("ULA output latch state failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_IO_WRITE, 36u, 0x12ffu, 0x1fu, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        machine.ula_output != 0x05u || machine.ula_output_tick != 32u) {
        fputs("odd-port ULA output selection failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_IO_WRITE, 40u, 0x12feu, 0x1bu, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        machine.ula_output != 0x1bu || machine.ula_output_tick != 40u) {
        fputs("ULA output latch transition failed\n", stderr);
        return 1;
    }
    wz_machine_set_timing_trace(&machine, 0);
    machine.master_tick = 0u;
    wz_bus_request_init(&bus_request, WZ_BUS_MEMORY_READ, 28670u, 0x4000u, 0u, 3u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.contention_delay != 6u || bus_request.master_tick != 28682u ||
        machine.master_tick != 12u) {
        fputs("48K memory contention boundary failed\n", stderr);
        return 1;
    }
    machine.master_tick = 0u;
    wz_bus_request_init(&bus_request, WZ_BUS_MEMORY_READ, 28926u, 0x4000u, 0u, 3u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.contention_delay != 0u || bus_request.master_tick != 28926u ||
        machine.master_tick != 0u) {
        fputs("48K contention window end failed\n", stderr);
        return 1;
    }
    machine.master_tick = 0u;
    wz_bus_request_init(&bus_request, WZ_BUS_IO_READ, 28670u, 0x12feu, 0u, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.contention_delay != 12u || bus_request.master_tick != 28694u) {
        fputs("48K ULA I/O contention pattern failed\n", stderr);
        return 1;
    }
    machine.master_tick = 0u;
    wz_bus_request_init(&bus_request, WZ_BUS_IO_READ, 28670u, 0x40feu, 0u, 4u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.contention_delay != 18u || bus_request.master_tick != 28706u) {
        fputs("48K contended-port I/O pattern failed\n", stderr);
        return 1;
    }
    machine.master_tick = 0u;
    if (wz_machine_set_hardware_io_decode(&machine, false) != WZ_RESULT_OK) {
        fputs("synthetic CPU fixture I/O setup failed\n", stderr);
        return 1;
    }
    wz_bus_request_init(&bus_request, WZ_BUS_INTERRUPT_ACKNOWLEDGE,
                        28u, 0xffffu, 0u, 7u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.value != interrupt_value ||
        wz_machine_set_bus_input(&machine, 0) != WZ_RESULT_OK) {
        fputs("bus input interrupt or removal failed\n", stderr);
        return 1;
    }
    if (wz_z80_primary_opcode_count() != 256u) {
        fputs("primary opcode table size failed\n", stderr);
        return 1;
    }
    for (opcode_index = 0u; opcode_index < wz_z80_primary_opcode_count(); ++opcode_index) {
        const wz_z80_opcode_decode_t* decode =
            wz_z80_primary_opcode_decode((wz_byte_t)opcode_index);
        if (decode == 0 || decode->opcode != (wz_byte_t)opcode_index) {
            fputs("primary opcode table identity failed\n", stderr);
            return 1;
        }
        if (decode->status == WZ_Z80_OPCODE_DOCUMENTED_UNIMPLEMENTED) {
            documented_unimplemented += 1u;
        } else if (decode->status == WZ_Z80_OPCODE_IMPLEMENTED) {
            implemented += 1u;
        } else if (decode->status == WZ_Z80_OPCODE_PREFIX) {
            prefix += 1u;
        } else if (decode->status == WZ_Z80_OPCODE_UNDOCUMENTED) {
            undocumented += 1u;
        } else if (decode->status == WZ_Z80_OPCODE_ILLEGAL) {
            illegal += 1u;
        } else {
            fputs("primary opcode table classification failed\n", stderr);
            return 1;
        }
    }
    if (implemented != 252u || prefix != 4u || documented_unimplemented != 0u ||
        undocumented != 0u || illegal != 0u ||
        wz_z80_primary_opcode_decode(0x00u)->operation != WZ_Z80_PRIMARY_OP_NOP ||
        wz_z80_primary_opcode_decode(0x32u)->operation != WZ_Z80_PRIMARY_OP_LD_NN_A ||
        wz_z80_primary_opcode_decode(0x01u)->operation != WZ_Z80_PRIMARY_OP_LOAD ||
        wz_z80_primary_opcode_decode(0x40u)->operation != WZ_Z80_PRIMARY_OP_LOAD ||
        wz_z80_primary_opcode_decode(0x3eu)->operation != WZ_Z80_PRIMARY_OP_LD_A_N ||
        wz_z80_primary_opcode_decode(0xd3u)->operation != WZ_Z80_PRIMARY_OP_OUT_N_A ||
        wz_z80_primary_opcode_decode(0xd9u)->operation != WZ_Z80_PRIMARY_OP_EXX ||
        wz_z80_primary_opcode_decode(0xebu)->operation != WZ_Z80_PRIMARY_OP_EX_DE_HL ||
        wz_z80_primary_opcode_decode(0xdbu)->operation != WZ_Z80_PRIMARY_OP_IN_A_N ||
        wz_z80_primary_opcode_decode(0xe3u)->operation != WZ_Z80_PRIMARY_OP_EX_SP_RR ||
        wz_z80_primary_opcode_decode(0xf9u)->operation != WZ_Z80_PRIMARY_OP_LD_SP_RR ||
        wz_z80_primary_opcode_decode(0x80u)->operation != WZ_Z80_PRIMARY_OP_ALU ||
        wz_z80_primary_opcode_decode(0xbfu)->operation != WZ_Z80_PRIMARY_OP_ALU ||
        wz_z80_primary_opcode_decode(0xc6u)->operation != WZ_Z80_PRIMARY_OP_ALU ||
        wz_z80_primary_opcode_decode(0xfeu)->operation != WZ_Z80_PRIMARY_OP_ALU ||
        wz_z80_primary_opcode_decode(0x09u)->operation != WZ_Z80_PRIMARY_OP_ADD_HL_RR ||
        wz_z80_primary_opcode_decode(0x39u)->operation != WZ_Z80_PRIMARY_OP_ADD_HL_RR ||
        wz_z80_primary_opcode_decode(0x07u)->operation != WZ_Z80_PRIMARY_OP_SPECIAL_FLAGS ||
        wz_z80_primary_opcode_decode(0x3fu)->operation != WZ_Z80_PRIMARY_OP_SPECIAL_FLAGS ||
        wz_z80_primary_opcode_decode(0xf1u)->operation != WZ_Z80_PRIMARY_OP_POP ||
        wz_z80_primary_opcode_decode(0xc5u)->operation != WZ_Z80_PRIMARY_OP_PUSH ||
        wz_z80_primary_opcode_decode(0xc9u)->operation != WZ_Z80_PRIMARY_OP_RET ||
        wz_z80_primary_opcode_decode(0xfcu)->operation != WZ_Z80_PRIMARY_OP_CALL ||
        wz_z80_primary_opcode_decode(0xffu)->operation != WZ_Z80_PRIMARY_OP_RST ||
        wz_z80_primary_opcode_decode(0x10u)->operation != WZ_Z80_PRIMARY_OP_BRANCH ||
        wz_z80_primary_opcode_decode(0x38u)->operation != WZ_Z80_PRIMARY_OP_BRANCH ||
        wz_z80_primary_opcode_decode(0xc3u)->operation != WZ_Z80_PRIMARY_OP_BRANCH ||
        wz_z80_primary_opcode_decode(0xe9u)->operation != WZ_Z80_PRIMARY_OP_BRANCH ||
        wz_z80_primary_opcode_decode(0xfau)->operation != WZ_Z80_PRIMARY_OP_BRANCH ||
        wz_z80_primary_opcode_decode(0x04u)->operation != WZ_Z80_PRIMARY_OP_INC_DEC ||
        wz_z80_primary_opcode_decode(0x35u)->operation != WZ_Z80_PRIMARY_OP_INC_DEC ||
        wz_z80_primary_opcode_decode(0x3du)->operation != WZ_Z80_PRIMARY_OP_INC_DEC ||
        wz_z80_primary_opcode_decode(0x76u)->operation != WZ_Z80_PRIMARY_OP_HALT ||
        wz_z80_primary_opcode_decode(0xf3u)->operation != WZ_Z80_PRIMARY_OP_DI ||
        wz_z80_primary_opcode_decode(0xfbu)->operation != WZ_Z80_PRIMARY_OP_EI ||
        wz_z80_primary_opcode_decode(0xcbu)->operation != WZ_Z80_PRIMARY_OP_PREFIX_CB ||
        wz_z80_primary_opcode_decode(0xddu)->operation != WZ_Z80_PRIMARY_OP_PREFIX_DD ||
        wz_z80_primary_opcode_decode(0xedu)->operation != WZ_Z80_PRIMARY_OP_PREFIX_ED ||
        wz_z80_primary_opcode_decode(0xfdu)->operation != WZ_Z80_PRIMARY_OP_PREFIX_FD) {
        fputs("primary opcode table contents failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2000u;
    machine.memory[0x2000u] = 0x01u;
    machine.memory[0x2001u] = 0x34u;
    machine.memory[0x2002u] = 0x12u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.b != 0x12u ||
        machine.cpu.main.c != 0x34u || machine.cpu.program_counter != 0x2003u ||
        machine.master_tick != 20u) {
        fputs("LD BC,nn state or timing failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2050u;
    machine.cpu.main.a = 0x56u;
    machine.cpu.main.b = 0x40u;
    machine.cpu.main.c = 0x01u;
    machine.memory[0x2050u] = 0x02u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.memory[0x4001u] != 0x56u ||
        machine.cpu.memptr != 0x5602u || machine.master_tick != 14u) {
        fputs("LD (BC),A MEMPTR state failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    memset(&bus_log, 0, sizeof(bus_log));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2100u;
    machine.cpu.main.h = 0x12u;
    machine.cpu.main.l = 0x34u;
    machine.memory[0x2100u] = 0x22u;
    machine.memory[0x2101u] = 0x00u;
    machine.memory[0x2102u] = 0x40u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.memory[0x4000u] != 0x34u ||
        machine.memory[0x4001u] != 0x12u || machine.cpu.program_counter != 0x2103u ||
        machine.cpu.memptr != 0x4001u || machine.master_tick != 32u || bus_log.count != 5u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ || bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ || bus_log.requests[2].master_tick != 14u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE || bus_log.requests[3].master_tick != 20u ||
        bus_log.requests[4].cycle != WZ_BUS_MEMORY_WRITE || bus_log.requests[4].master_tick != 26u ||
        wz_machine_set_bus_observer(&machine, 0) != WZ_RESULT_OK) {
        fputs("LD (nn),HL bus sequence failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2200u;
    machine.cpu.main.h = 0x50u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0x2200u] = 0x36u;
    machine.memory[0x2201u] = 0xa5u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.memory[0x5000u] != 0xa5u ||
        machine.cpu.program_counter != 0x2202u || machine.master_tick != 20u) {
        fputs("LD (HL),n state or timing failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2300u;
    machine.cpu.main.a = 0x11u;
    machine.cpu.main.f = 0x22u;
    machine.cpu.alternate.a = 0x33u;
    machine.cpu.alternate.f = 0x44u;
    machine.memory[0x2300u] = 0x08u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.a != 0x33u ||
        machine.cpu.main.f != 0x44u || machine.cpu.alternate.a != 0x11u ||
        machine.cpu.alternate.f != 0x22u || machine.master_tick != 8u) {
        fputs("EX AF,AF' state or timing failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2300u;
    machine.cpu.main.a = 0x11u;
    machine.cpu.main.f = 0x22u;
    machine.cpu.main.b = 0x33u;
    machine.cpu.main.c = 0x44u;
    machine.cpu.main.d = 0x55u;
    machine.cpu.main.e = 0x66u;
    machine.cpu.main.h = 0x77u;
    machine.cpu.main.l = 0x88u;
    machine.cpu.alternate.a = 0x99u;
    machine.cpu.alternate.f = 0xaau;
    machine.cpu.alternate.b = 0xbbu;
    machine.cpu.alternate.c = 0xccu;
    machine.cpu.alternate.d = 0xddu;
    machine.cpu.alternate.e = 0xeeu;
    machine.cpu.alternate.h = 0xf0u;
    machine.cpu.alternate.l = 0x0fu;
    machine.memory[0x2300u] = 0xd9u;
    memset(&bus_log, 0, sizeof(bus_log));
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.program_counter != 0x2301u ||
        machine.cpu.r != 0x01u || machine.cpu.main.a != 0x11u ||
        machine.cpu.main.f != 0x22u || machine.cpu.main.b != 0xbbu ||
        machine.cpu.main.c != 0xccu || machine.cpu.main.d != 0xddu ||
        machine.cpu.main.e != 0xeeu || machine.cpu.main.h != 0xf0u ||
        machine.cpu.main.l != 0x0fu || machine.cpu.alternate.a != 0x99u ||
        machine.cpu.alternate.f != 0xaau || machine.cpu.alternate.b != 0x33u ||
        machine.cpu.alternate.c != 0x44u || machine.cpu.alternate.d != 0x55u ||
        machine.cpu.alternate.e != 0x66u || machine.cpu.alternate.h != 0x77u ||
        machine.cpu.alternate.l != 0x88u || machine.master_tick != 8u ||
        bus_log.count != 1u || bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[0].master_tick != 0u || bus_log.requests[0].address != 0x2300u) {
        fputs("EXX state, fetch, or timing failed\n", stderr);
        return 1;
    }
    machine.memory[0x2301u] = 0xddu;
    machine.memory[0x2302u] = 0xfdu;
    machine.memory[0x2303u] = 0xd9u;
    memset(&bus_log, 0, sizeof(bus_log));
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.program_counter != 0x2304u ||
        machine.cpu.r != 0x04u || machine.cpu.main.b != 0x33u ||
        machine.cpu.main.c != 0x44u || machine.cpu.main.d != 0x55u ||
        machine.cpu.main.e != 0x66u || machine.cpu.main.h != 0x77u ||
        machine.cpu.main.l != 0x88u || machine.cpu.alternate.b != 0xbbu ||
        machine.cpu.alternate.c != 0xccu || machine.cpu.alternate.d != 0xddu ||
        machine.cpu.alternate.e != 0xeeu || machine.cpu.alternate.h != 0xf0u ||
        machine.cpu.alternate.l != 0x0fu || machine.master_tick != 32u ||
        bus_log.count != 3u || bus_log.requests[0].address != 0x2301u ||
        bus_log.requests[1].address != 0x2302u || bus_log.requests[2].address != 0x2303u) {
        fputs("prefixed EXX state, fetch, or timing failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2380u;
    machine.cpu.main.d = 0x12u;
    machine.cpu.main.e = 0x34u;
    machine.cpu.main.h = 0x56u;
    machine.cpu.main.l = 0x78u;
    machine.cpu.main.f = 0xa5u;
    machine.cpu.ix = 0x9abcu;
    machine.cpu.iy = 0xdef0u;
    machine.cpu.memptr = 0x1357u;
    machine.memory[0x2380u] = 0xebu;
    memset(&bus_log, 0, sizeof(bus_log));
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.program_counter != 0x2381u ||
        machine.cpu.r != 1u || machine.cpu.main.d != 0x56u || machine.cpu.main.e != 0x78u ||
        machine.cpu.main.h != 0x12u || machine.cpu.main.l != 0x34u || machine.cpu.main.f != 0xa5u ||
        machine.cpu.ix != 0x9abcu || machine.cpu.iy != 0xdef0u || machine.cpu.memptr != 0x1357u ||
        machine.master_tick != 8u || bus_log.count != 1u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH) {
        fputs("EX DE,HL state, fetch, or timing failed\n", stderr);
        return 1;
    }
    machine.memory[0x2381u] = 0xddu;
    machine.memory[0x2382u] = 0xfdu;
    machine.memory[0x2383u] = 0xebu;
    memset(&bus_log, 0, sizeof(bus_log));
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.program_counter != 0x2384u ||
        machine.cpu.r != 4u || machine.cpu.main.d != 0x12u || machine.cpu.main.e != 0x34u ||
        machine.cpu.main.h != 0x56u || machine.cpu.main.l != 0x78u || machine.cpu.main.f != 0xa5u ||
        machine.cpu.ix != 0x9abcu || machine.cpu.iy != 0xdef0u || machine.cpu.memptr != 0x1357u ||
        machine.master_tick != 32u || bus_log.count != 3u ||
        bus_log.requests[0].address != 0x2381u || bus_log.requests[1].address != 0x2382u ||
        bus_log.requests[2].address != 0x2383u) {
        fputs("prefixed EX DE,HL state, fetch, or timing failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2390u;
    machine.cpu.main.h = 0x12u;
    machine.cpu.main.l = 0x34u;
    machine.cpu.main.f = 0xa5u;
    machine.cpu.ix = 0x5678u;
    machine.cpu.iy = 0x9abcu;
    machine.cpu.memptr = 0xdef0u;
    machine.memory[0x2390u] = 0xf9u;
    memset(&bus_log, 0, sizeof(bus_log));
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.program_counter != 0x2391u ||
        machine.cpu.r != 1u || machine.cpu.stack_pointer != 0x1234u ||
        machine.cpu.main.f != 0xa5u || machine.cpu.ix != 0x5678u ||
        machine.cpu.iy != 0x9abcu || machine.cpu.memptr != 0xdef0u ||
        machine.master_tick != 12u || bus_log.count != 1u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[0].address != 0x2390u) {
        fputs("LD SP,HL state, fetch, or timing failed\n", stderr);
        return 1;
    }
    machine.memory[0x2391u] = 0xddu;
    machine.memory[0x2392u] = 0xf9u;
    memset(&bus_log, 0, sizeof(bus_log));
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.program_counter != 0x2393u ||
        machine.cpu.r != 3u || machine.cpu.stack_pointer != 0x5678u ||
        machine.cpu.main.f != 0xa5u || machine.cpu.memptr != 0xdef0u ||
        machine.master_tick != 32u || bus_log.count != 2u ||
        bus_log.requests[0].address != 0x2391u || bus_log.requests[1].address != 0x2392u) {
        fputs("DD LD SP,IX state, fetch, or timing failed\n", stderr);
        return 1;
    }
    machine.memory[0x2393u] = 0xfdu;
    machine.memory[0x2394u] = 0xf9u;
    memset(&bus_log, 0, sizeof(bus_log));
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.program_counter != 0x2395u ||
        machine.cpu.r != 5u || machine.cpu.stack_pointer != 0x9abcu ||
        machine.cpu.main.f != 0xa5u || machine.cpu.memptr != 0xdef0u ||
        machine.master_tick != 52u || bus_log.count != 2u ||
        bus_log.requests[0].address != 0x2393u || bus_log.requests[1].address != 0x2394u) {
        fputs("FD LD SP,IY state, fetch, or timing failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2400u;
    machine.cpu.stack_pointer = 0xffffu;
    machine.cpu.main.h = 0x12u;
    machine.cpu.main.l = 0x34u;
    machine.cpu.main.f = 0xa5u;
    machine.memory[0x2400u] = 0xe3u;
    machine.memory[0xffffu] = 0x78u;
    machine.memory[0u] = 0x56u;
    memset(&bus_log, 0, sizeof(bus_log));
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.program_counter != 0x2401u ||
        machine.cpu.r != 1u || machine.cpu.stack_pointer != 0xffffu ||
        machine.cpu.main.h != 0x56u || machine.cpu.main.l != 0x78u ||
        machine.cpu.main.f != 0xa5u || machine.cpu.memptr != 0x5678u ||
        machine.memory[0xffffu] != 0x34u || machine.memory[0u] != 0x12u ||
        machine.master_tick != 38u || bus_log.count != 6u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].address != 0xffffu ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].address != 0u ||
        bus_log.requests[3].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[4].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[4].address != 0xffffu || bus_log.requests[4].value != 0x34u ||
        bus_log.requests[5].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[5].address != 0u || bus_log.requests[5].value != 0x12u) {
        fputs("EX (SP),HL state, fixture wrap, or bus timing failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2500u;
    machine.cpu.stack_pointer = 0x4000u;
    machine.cpu.ix = 0x1234u;
    machine.cpu.main.h = 0xaau;
    machine.cpu.main.l = 0xbbu;
    machine.memory[0x2500u] = 0xddu;
    machine.memory[0x2501u] = 0xe3u;
    machine.memory[0x4000u] = 0x78u;
    machine.memory[0x4001u] = 0x56u;
    memset(&bus_log, 0, sizeof(bus_log));
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.program_counter != 0x2502u ||
        machine.cpu.r != 2u || machine.cpu.ix != 0x5678u ||
        machine.cpu.main.h != 0xaau || machine.cpu.main.l != 0xbbu ||
        machine.memory[0x4000u] != 0x34u || machine.memory[0x4001u] != 0x12u ||
        machine.master_tick != 46u || bus_log.count != 7u ||
        bus_log.requests[0].address != 0x2500u || bus_log.requests[1].address != 0x2501u ||
        bus_log.requests[2].address != 0x4000u || bus_log.requests[3].address != 0x4001u) {
        fputs("DD EX (SP),IX state, fetch, or timing failed\n", stderr);
        return 1;
    }
    memset(&machine.cpu, 0, sizeof(machine.cpu));
    machine.master_tick = 0u;
    machine.cpu.program_counter = 0x2600u;
    machine.cpu.stack_pointer = 0x4100u;
    machine.cpu.iy = 0x9abcu;
    machine.memory[0x2600u] = 0xfdu;
    machine.memory[0x2601u] = 0xe3u;
    machine.memory[0x4100u] = 0xefu;
    machine.memory[0x4101u] = 0xbeu;
    memset(&bus_log, 0, sizeof(bus_log));
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.program_counter != 0x2602u ||
        machine.cpu.r != 2u || machine.cpu.iy != 0xbeefu ||
        machine.memory[0x4100u] != 0xbcu || machine.memory[0x4101u] != 0x9au ||
        machine.master_tick != 46u || bus_log.count != 7u ||
        bus_log.requests[0].address != 0x2600u || bus_log.requests[1].address != 0x2601u) {
        fputs("FD EX (SP),IY state, fetch, or timing failed\n", stderr);
        return 1;
    }
    if (wz_z80_cb_opcode_count() != 256u) {
        fputs("CB opcode table size failed\n", stderr);
        return 1;
    }
    for (opcode_index = 0u; opcode_index < wz_z80_cb_opcode_count(); ++opcode_index) {
        const wz_z80_cb_opcode_decode_t* decode =
            wz_z80_cb_opcode_decode((wz_byte_t)opcode_index);
        if (decode == 0 || decode->opcode != (wz_byte_t)opcode_index ||
            decode->target != (wz_byte_t)(opcode_index & 0x07u) || decode->bit > 7u) {
            fputs("CB opcode table identity failed\n", stderr);
            return 1;
        }
        if (decode->status == WZ_Z80_OPCODE_DOCUMENTED_UNIMPLEMENTED) {
            cb_documented_unimplemented += 1u;
        } else if (decode->status == WZ_Z80_OPCODE_IMPLEMENTED) {
            cb_implemented += 1u;
        } else if (decode->status == WZ_Z80_OPCODE_UNDOCUMENTED) {
            cb_undocumented += 1u;
        } else {
            fputs("CB opcode table classification failed\n", stderr);
            return 1;
        }
    }
    if (cb_documented_unimplemented != 0u || cb_implemented != 248u || cb_undocumented != 8u ||
        wz_z80_cb_opcode_decode(0x00u)->operation != WZ_Z80_CB_OP_RLC ||
        wz_z80_cb_opcode_decode(0x30u)->operation != WZ_Z80_CB_OP_SLL ||
        wz_z80_cb_opcode_decode(0x30u)->status != WZ_Z80_OPCODE_UNDOCUMENTED ||
        wz_z80_cb_opcode_decode(0x40u)->operation != WZ_Z80_CB_OP_BIT ||
        wz_z80_cb_opcode_decode(0x40u)->bit != 0u ||
        wz_z80_cb_opcode_decode(0x7fu)->operation != WZ_Z80_CB_OP_BIT ||
        wz_z80_cb_opcode_decode(0x7fu)->bit != 7u ||
        wz_z80_cb_opcode_decode(0x80u)->operation != WZ_Z80_CB_OP_RES ||
        wz_z80_cb_opcode_decode(0xffu)->operation != WZ_Z80_CB_OP_SET ||
        wz_z80_cb_opcode_decode(0xffu)->target != 7u ||
        wz_z80_cb_opcode_decode(0xffu)->bit != 7u) {
        fputs("CB opcode table contents failed\n", stderr);
        return 1;
    }
    if (wz_z80_ed_opcode_count() != 256u) {
        fputs("ED opcode table size failed\n", stderr);
        return 1;
    }
    for (opcode_index = 0u; opcode_index < wz_z80_ed_opcode_count(); ++opcode_index) {
        wz_z80_ed_opcode_decode_t decode =
            wz_z80_ed_opcode_decode((wz_byte_t)opcode_index);
        if (decode.opcode != (wz_byte_t)opcode_index) {
            fputs("ED opcode table identity failed\n", stderr);
            return 1;
        }
        if (decode.status == WZ_Z80_OPCODE_DOCUMENTED_UNIMPLEMENTED) {
            ed_documented_unimplemented += 1u;
        } else if (decode.status == WZ_Z80_OPCODE_IMPLEMENTED) {
            ed_implemented += 1u;
        } else if (decode.status == WZ_Z80_OPCODE_UNDOCUMENTED) {
            ed_undocumented += 1u;
        } else {
            fputs("ED opcode table classification failed\n", stderr);
            return 1;
        }
    }
    if (ed_documented_unimplemented != 0u || ed_implemented == 0u ||
        ed_documented_unimplemented + ed_implemented + ed_undocumented != 256u ||
        wz_z80_ed_opcode_decode(0x44u).operation != WZ_Z80_ED_OP_NEG ||
        wz_z80_ed_opcode_decode(0x44u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0x4cu).operation != WZ_Z80_ED_OP_NEG ||
        wz_z80_ed_opcode_decode(0x4cu).status != WZ_Z80_OPCODE_UNDOCUMENTED ||
        wz_z80_ed_opcode_decode(0x45u).operation != WZ_Z80_ED_OP_RETN ||
        wz_z80_ed_opcode_decode(0x45u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0x4du).operation != WZ_Z80_ED_OP_RETI ||
        wz_z80_ed_opcode_decode(0x4du).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0x70u).operation != WZ_Z80_ED_OP_IN_R_C ||
        wz_z80_ed_opcode_decode(0x70u).status != WZ_Z80_OPCODE_UNDOCUMENTED ||
        wz_z80_ed_opcode_decode(0x43u).operation != WZ_Z80_ED_OP_LD_NN_RR ||
        wz_z80_ed_opcode_decode(0x4bu).operation != WZ_Z80_ED_OP_LD_RR_NN ||
        wz_z80_ed_opcode_decode(0xa0u).operation != WZ_Z80_ED_OP_LDI ||
        wz_z80_ed_opcode_decode(0xa0u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xa8u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xb0u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xb8u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xa1u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xa9u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xb1u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xb9u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xa2u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xa3u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xaau).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xabu).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xb2u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xb3u).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xbau).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xbbu).status != WZ_Z80_OPCODE_IMPLEMENTED ||
        wz_z80_ed_opcode_decode(0xbbu).operation != WZ_Z80_ED_OP_OTDR ||
        wz_z80_ed_opcode_decode(0xffu).operation != WZ_Z80_ED_OP_UNSUPPORTED ||
        wz_z80_ed_opcode_decode(0xffu).status != WZ_Z80_OPCODE_UNDOCUMENTED) {
        fputs("ED opcode table contents failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 skeleton test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.r != 1u ||
        machine.cpu.program_counter != 1u ||
        machine.master_tick != 8u ||
        bus_log.count != 1u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[0].master_tick != 0u ||
        bus_log.requests[0].address != 0u ||
        bus_log.requests[0].value != 0u) {
        fputs("Z80 NOP fetch trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before structured timing trace test failed\n", stderr);
        return 1;
    }
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    wz_trace_sink_init(&trace_sink, record_timing_trace, &timing_trace_log);
    trace_sink.next_sequence = WZ_TRACE_CPU_SYNC_INTERVAL - 2u;
    wz_machine_set_timing_trace(&machine, &trace_sink);
    machine.cpu.main.a = 0x12u;
    machine.cpu.main.f = 0x34u;
    machine.cpu.alternate.a = 0x56u;
    machine.cpu.alternate.f = 0x78u;
    machine.cpu.ix = 0x9abcu;
    machine.cpu.iy = 0xdef0u;
    machine.cpu.stack_pointer = 0x1357u;
    machine.cpu.memptr = 0x2468u;
    machine.cpu.i = 0x9au;
    machine.cpu.r = 0x3cu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_enable_delay = 1u;
    machine.cpu.interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2;
    machine.cpu.halted = 0u;
    machine.memory[0u] = 0x00u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || timing_trace_log.count != 8u ||
        timing_trace_log.events[0].kind != WZ_TRACE_CPU_BUS ||
        timing_trace_log.events[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        timing_trace_log.events[0].address != 0u ||
        timing_trace_log.events[1].kind != WZ_TRACE_CPU_OPCODE_BYTE ||
        timing_trace_log.events[1].master_tick != timing_trace_log.events[0].master_tick ||
        timing_trace_log.events[1].address != 0u || timing_trace_log.events[1].value != 0x00u ||
        timing_trace_log.events[2].kind != WZ_TRACE_CPU_INSTRUCTION ||
        timing_trace_log.events[2].program_counter != 0u ||
        timing_trace_log.events[2].value != 0x00u ||
        timing_trace_log.events[2].sequence != WZ_TRACE_CPU_SYNC_INTERVAL ||
        (timing_trace_log.events[2].register_snapshot & UINT64_C(0xffff)) != UINT64_C(0x3412)) {
        fputs("structured CPU timing trace failed\n", stderr);
        return 1;
    }
    if (timing_trace_log.events[3].kind != WZ_TRACE_CPU_STATE_SYNC ||
        timing_trace_log.events[3].cycle != 0u ||
        timing_trace_log.events[3].register_snapshot != timing_trace_log.events[2].register_snapshot ||
        timing_trace_log.events[4].cycle != 1u ||
        (timing_trace_log.events[4].register_snapshot & UINT64_C(0xffff)) != UINT64_C(0x7856) ||
        timing_trace_log.events[5].cycle != 2u ||
        (timing_trace_log.events[5].register_snapshot & UINT64_C(0xffffffff)) != UINT64_C(0xdef09abc) ||
        timing_trace_log.events[6].cycle != 3u ||
        (timing_trace_log.events[6].register_snapshot & UINT64_C(0xffff)) != UINT64_C(0x2468) ||
        timing_trace_log.events[7].cycle != 4u ||
        timing_trace_log.events[7].register_snapshot != 0u ||
        timing_trace_log.events[3].sequence + 4u != timing_trace_log.events[7].sequence) {
        fputs("complete CPU timing synchronization trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before failing opcode trace test failed\n", stderr);
        return 1;
    }
    remove(failing_trace_path);
    if (wz_trace_file_create(&trace_file, failing_trace_path, 4u,
                             (wz_dword_t)profile->kind, 0x9abcu, UINT32_MAX) != WZ_RESULT_OK) {
        fputs("failing opcode trace creation failed\n", stderr);
        return 1;
    }
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    machine.memory[0u] = 0xf9u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        wz_trace_file_freeze(&trace_file) != WZ_RESULT_OK) {
        wz_trace_file_close(&trace_file);
        fputs("failing opcode trace execution failed\n", stderr);
        return 1;
    }
    wz_trace_file_close(&trace_file);
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    if (wz_trace_file_recover(failing_trace_path, recover_timing_trace, &timing_trace_log,
                              &recovered_count) != WZ_RESULT_OK ||
        recovered_count != 8u || timing_trace_log.count != 8u ||
        timing_trace_log.events[0].kind != WZ_TRACE_CPU_BUS ||
        timing_trace_log.events[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        timing_trace_log.events[0].address != 0u || timing_trace_log.events[0].value != 0xf9u ||
        timing_trace_log.events[1].kind != WZ_TRACE_CPU_OPCODE_BYTE ||
        timing_trace_log.events[1].address != 0u || timing_trace_log.events[1].value != 0xf9u ||
        timing_trace_log.events[2].kind != WZ_TRACE_CPU_INSTRUCTION ||
        timing_trace_log.events[2].program_counter != 0u || timing_trace_log.events[2].value != 0xf9u) {
        remove(failing_trace_path);
        fputs("failing opcode trace recovery failed\n", stderr);
        return 1;
    }
    wz_trace_cpu_state_sync_init(&recovered_cpu_sync);
    {
        bool recovered_state = false;
        for (size_t index = 3u; index < timing_trace_log.count; ++index) {
            recovered_state = wz_trace_cpu_state_sync_apply(&recovered_cpu_sync,
                                                             &timing_trace_log.events[index]);
        }
        if (!recovered_state || !recovered_cpu_sync.has_absolute_state ||
            recovered_cpu_sync.master_tick != 0u ||
            recovered_cpu_sync.state.program_counter != 1u || recovered_cpu_sync.state.r != 1u) {
            remove(failing_trace_path);
            fputs("failing opcode state reconstruction failed\n", stderr);
            return 1;
        }
    }
    remove(failing_trace_path);
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 load test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.memory[0u] = 0x3eu;
    machine.memory[1u] = 0x77u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x77u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 14u ||
        bus_log.count != 2u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].address != 1u ||
        bus_log.requests[1].value != 0x77u) {
        fputs("Z80 immediate load trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 store test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.a = 0x22u;
    machine.memory[0u] = 0x32u;
    machine.memory[1u] = 0x00u;
    machine.memory[2u] = 0x40u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x4000u] != 0x22u ||
        machine.cpu.program_counter != 3u ||
        machine.cpu.memptr != 0x2201u ||
        machine.master_tick != 26u ||
        bus_log.count != 4u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 20u ||
        bus_log.requests[3].address != 0x4000u ||
        bus_log.requests[3].value != 0x22u) {
        fputs("Z80 absolute store trace failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 OUT (n),A test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.a = 0xa2u;
    machine.cpu.main.f = 0x5au;
    machine.memory[0u] = 0xd3u;
    machine.memory[1u] = 0xecu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xa2u || machine.cpu.main.f != 0x5au ||
        machine.cpu.memptr != 0xa2edu || machine.cpu.program_counter != 2u ||
        machine.cpu.r != 1u || machine.master_tick != 22u || bus_log.count != 3u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 8u || bus_log.requests[1].address != 1u ||
        bus_log.requests[1].value != 0xecu ||
        bus_log.requests[2].cycle != WZ_BUS_IO_WRITE ||
        bus_log.requests[2].master_tick != 14u || bus_log.requests[2].address != 0xa2ecu ||
        bus_log.requests[2].value != 0xa2u) {
        fputs("Z80 OUT (n),A trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before OUT (n),A MEMPTR wrap test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0xa2u;
    machine.memory[0u] = 0xd3u;
    machine.memory[1u] = 0xffu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.memptr != 0xa200u ||
        machine.cpu.program_counter != 2u || machine.master_tick != 22u) {
        fputs("Z80 OUT (n),A MEMPTR low-byte wrap failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 unsupported-opcode test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x00u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_UNSUPPORTED_OPERATION ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 0u ||
        bus_log.count != 2u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[0].address != 0u ||
        bus_log.requests[0].value != 0xedu ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].address != 1u ||
        bus_log.requests[1].value != 0x00u) {
        fputs("Z80 unsupported opcode trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ADD HL edge test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.h = 0x0fu;
    machine.cpu.main.l = 0xffu;
    machine.cpu.main.b = 0xf0u;
    machine.cpu.main.c = 0x01u;
    machine.cpu.main.f = 0xc7u;
    machine.memory[0u] = 0x09u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.h != 0x00u || machine.cpu.main.l != 0x00u ||
        machine.cpu.main.f != 0xd5u || machine.cpu.memptr != 0x1000u ||
        machine.cpu.program_counter != 1u || machine.master_tick != 22u) {
        fputs("Z80 ADD HL,BC carry/half-carry flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.h = 0x20u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.stack_pointer = 0x0800u;
    machine.cpu.main.f = 0x45u;
    machine.memory[1u] = 0x39u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.h != 0x28u || machine.cpu.main.l != 0x00u ||
        machine.cpu.main.f != 0x6cu || machine.cpu.memptr != 0x2001u ||
        machine.cpu.program_counter != 2u || machine.master_tick != 44u) {
        fputs("Z80 ADD HL,SP preserved/undocumented flags failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before special flag vectors failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x9au;
    machine.memory[0u] = 0x27u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x00u || machine.cpu.main.f != 0x55u) {
        fputs("Z80 DAA addition correction failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x0fu;
    machine.cpu.main.f = 0x12u;
    machine.memory[1u] = 0x27u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x09u || machine.cpu.main.f != 0x0eu) {
        fputs("Z80 DAA subtraction correction failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x9au;
    machine.cpu.main.f = 0x02u;
    machine.memory[2u] = 0x27u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x34u || machine.cpu.main.f != 0x23u) {
        fputs("Z80 DAA subtraction invalid-BCD correction failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x55u;
    machine.cpu.main.f = 0x45u;
    machine.memory[3u] = 0x2fu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xaau || machine.cpu.main.f != 0x7fu) {
        fputs("Z80 CPL flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x28u;
    machine.cpu.main.f = 0x44u;
    machine.memory[4u] = 0x37u;
    machine.memory[5u] = 0x3fu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.f != 0x6du ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.f != 0x7cu) {
        fputs("Z80 SCF/CCF flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x00u;
    machine.cpu.main.f = 0xffu;
    machine.memory[6u] = 0x37u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.f != 0xedu) {
        fputs("Z80 SCF prior-flag X/Y sourcing failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0xffu;
    machine.cpu.main.f = 0x00u;
    machine.memory[7u] = 0x37u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.f != 0x29u) {
        fputs("Z80 SCF accumulator X/Y sourcing failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before accumulator rotate vectors failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x81u;
    machine.cpu.main.f = 0x44u;
    machine.memory[0u] = 0x07u;
    machine.memory[1u] = 0x1fu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x03u || machine.cpu.main.f != 0x45u ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x81u || machine.cpu.main.f != 0x45u ||
        machine.cpu.program_counter != 2u || machine.master_tick != 16u) {
        fputs("Z80 accumulator rotate flags failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 CB-prefix test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.memory[0u] = 0xcbu;
    machine.memory[1u] = 0x11u;
    machine.cpu.main.c = 0x80u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.c != 0x00u ||
        machine.cpu.main.f != 0x45u ||
        machine.cpu.r != 2u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 16u ||
        bus_log.count != 2u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[0].address != 0u ||
        bus_log.requests[0].value != 0xcbu ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 4u ||
        bus_log.requests[1].address != 1u ||
        bus_log.requests[1].value != 0x11u) {
        fputs("Z80 CB-prefix fetch trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED-prefix test failed\n", stderr);
        return 1;
    }

    machine.memory[0u] = 0xddu;
    machine.memory[1u] = 0xfdu;
    machine.memory[2u] = 0x00u;
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 3u ||
        machine.cpu.r != 3u ||
        machine.master_tick != 24u ||
        bus_log.count != 3u ||
        bus_log.requests[0].value != 0xddu ||
        bus_log.requests[1].value != 0xfdu ||
        bus_log.requests[2].value != 0x00u) {
        fputs("Z80 repeated index-prefix NOP failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before ignored index-prefix ED test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x15u;
    machine.memory[0u] = 0xddu;
    machine.memory[1u] = 0xedu;
    machine.memory[2u] = 0x44u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xebu ||
        machine.cpu.program_counter != 3u ||
        machine.cpu.r != 3u ||
        machine.master_tick != 24u) {
        fputs("Z80 ignored index-prefix ED execution failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before index register load test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.memory[0u] = 0xddu;
    machine.memory[1u] = 0x21u;
    machine.memory[2u] = 0x34u;
    machine.memory[3u] = 0x12u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.ix != 0x1234u || machine.cpu.iy != 0u ||
        machine.cpu.program_counter != 4u || machine.cpu.r != 2u ||
        machine.master_tick != 28u || bus_log.count != 4u ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].address != 1u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 16u ||
        bus_log.requests[3].master_tick != 22u) {
        fputs("Z80 DD LD IX,nn trace failed\n", stderr);
        return 1;
    }

    machine.memory[4u] = 0xfdu;
    machine.memory[5u] = 0x21u;
    machine.memory[6u] = 0x78u;
    machine.memory[7u] = 0x56u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.ix != 0x1234u || machine.cpu.iy != 0x5678u ||
        machine.cpu.program_counter != 8u || machine.cpu.r != 4u ||
        machine.master_tick != 56u) {
        fputs("Z80 FD LD IY,nn failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before index memory transfer test failed\n", stderr);
        return 1;
    }
    machine.cpu.ix = 0xabcdu;
    machine.memory[0u] = 0xddu;
    machine.memory[1u] = 0x22u;
    machine.memory[2u] = 0x00u;
    machine.memory[3u] = 0x40u;
    machine.memory[4u] = 0xfdu;
    machine.memory[5u] = 0x2au;
    machine.memory[6u] = 0x00u;
    machine.memory[7u] = 0x40u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x4000u] != 0xcdu || machine.memory[0x4001u] != 0xabu ||
        machine.cpu.memptr != 0x4001u || machine.master_tick != 40u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.iy != 0xabcdu ||
        machine.cpu.program_counter != 8u || machine.cpu.memptr != 0x4001u ||
        machine.master_tick != 80u) {
        fputs("Z80 DD/FD index memory transfer failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before index arithmetic test failed\n", stderr);
        return 1;
    }
    machine.cpu.ix = 0x0fffu;
    machine.cpu.main.b = 0x00u;
    machine.cpu.main.c = 0x01u;
    machine.cpu.main.f = 0xc5u;
    machine.memory[0u] = 0xddu;
    machine.memory[1u] = 0x09u;
    machine.memory[2u] = 0xddu;
    machine.memory[3u] = 0x23u;
    machine.memory[4u] = 0xddu;
    machine.memory[5u] = 0x2bu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.ix != 0x1000u ||
        machine.cpu.main.f != 0xd4u || machine.cpu.memptr != 0x1000u ||
        machine.master_tick != 30u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.ix != 0x1001u ||
        machine.cpu.main.f != 0xd4u || machine.master_tick != 50u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.ix != 0x1000u ||
        machine.cpu.main.f != 0xd4u || machine.master_tick != 70u) {
        fputs("Z80 index add/inc/dec failed\n", stderr);
        return 1;
    }

    machine.cpu.iy = 0x3456u;
    machine.memory[6u] = 0xfdu;
    machine.memory[7u] = 0xf9u;
    machine.memory[8u] = 0xfdu;
    machine.memory[9u] = 0xe9u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.stack_pointer != 0x3456u || machine.master_tick != 90u ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x3456u || machine.master_tick != 106u) {
        fputs("Z80 index stack-pointer/jump transfer failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before indexed byte operation test failed\n", stderr);
        return 1;
    }
    machine.cpu.ix = 0x7f80u;
    machine.cpu.iy = 0x8000u;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0xddu; machine.memory[1u] = 0x24u;
    machine.memory[2u] = 0xddu; machine.memory[3u] = 0x2du;
    machine.memory[4u] = 0xddu; machine.memory[5u] = 0x26u; machine.memory[6u] = 0x12u;
    machine.memory[7u] = 0xfdu; machine.memory[8u] = 0x2cu;
    machine.memory[9u] = 0xfdu; machine.memory[10u] = 0x25u;
    machine.memory[11u] = 0xfdu; machine.memory[12u] = 0x2eu; machine.memory[13u] = 0x34u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.ix != 0x8080u ||
        machine.cpu.main.f != 0x95u || machine.cpu.program_counter != 2u ||
        machine.cpu.r != 2u || machine.master_tick != 16u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.ix != 0x807fu ||
        machine.cpu.main.f != 0x3fu || machine.master_tick != 32u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.ix != 0x127fu ||
        machine.cpu.main.f != 0x3fu || machine.cpu.program_counter != 7u ||
        machine.cpu.r != 6u || machine.master_tick != 54u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.iy != 0x8001u ||
        machine.cpu.main.f != 0x01u || machine.master_tick != 70u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.iy != 0x7f01u ||
        machine.cpu.main.f != 0x3fu || machine.master_tick != 86u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.iy != 0x7f34u ||
        machine.cpu.main.f != 0x3fu || machine.cpu.program_counter != 14u ||
        machine.cpu.r != 12u || machine.master_tick != 108u) {
        fputs("Z80 DD/FD index-byte INC/DEC/LD behavior failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before indexed-memory operation test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.ix = 0x4002u;
    machine.cpu.iy = 0x4100u;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0xddu; machine.memory[1u] = 0x34u; machine.memory[2u] = 0xfeu;
    machine.memory[3u] = 0xfdu; machine.memory[4u] = 0x35u; machine.memory[5u] = 0x01u;
    machine.memory[6u] = 0xddu; machine.memory[7u] = 0x36u; machine.memory[8u] = 0xffu; machine.memory[9u] = 0xa5u;
    machine.memory[10u] = 0xfdu; machine.memory[11u] = 0x36u; machine.memory[12u] = 0x02u; machine.memory[13u] = 0x5au;
    machine.memory[0x4000u] = 0x7fu;
    machine.memory[0x4101u] = 0x80u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.memory[0x4000u] != 0x80u ||
        machine.cpu.main.f != 0x95u || machine.cpu.memptr != 0x4000u ||
        machine.cpu.program_counter != 3u || machine.cpu.r != 2u ||
        machine.master_tick != 46u || bus_log.count != 7u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ || bus_log.requests[2].master_tick != 16u ||
        bus_log.requests[3].cycle != WZ_BUS_INTERNAL || bus_log.requests[3].master_tick != 22u ||
        bus_log.requests[4].cycle != WZ_BUS_MEMORY_READ || bus_log.requests[4].master_tick != 32u ||
        bus_log.requests[5].cycle != WZ_BUS_INTERNAL || bus_log.requests[5].master_tick != 38u ||
        bus_log.requests[6].cycle != WZ_BUS_MEMORY_WRITE || bus_log.requests[6].master_tick != 40u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.memory[0x4101u] != 0x7fu ||
        machine.cpu.main.f != 0x3fu || machine.cpu.memptr != 0x4101u ||
        machine.master_tick != 92u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.memory[0x4001u] != 0xa5u ||
        machine.cpu.memptr != 0x4001u || machine.cpu.program_counter != 10u ||
        machine.cpu.r != 6u || machine.master_tick != 130u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.memory[0x4102u] != 0x5au ||
        machine.cpu.memptr != 0x4102u || machine.cpu.program_counter != 14u ||
        machine.cpu.r != 8u || machine.master_tick != 168u) {
        fprintf(stderr,
                "Z80 DD/FD indexed-memory INC/DEC/LD failed: ix=%04x iy=%04x f=%02x pc=%04x r=%02x mp=%04x tick=%llu count=%zu\n",
                (unsigned)machine.cpu.ix, (unsigned)machine.cpu.iy,
                (unsigned)machine.cpu.main.f, (unsigned)machine.cpu.program_counter,
                (unsigned)machine.cpu.r, (unsigned)machine.cpu.memptr,
                (unsigned long long)machine.master_tick, bus_log.count);
        return 1;
    }

    for (size_t prefix_index = 0u; prefix_index < 2u; prefix_index += 1u) {
        wz_byte_t prefix_value = prefix_index == 0u ? 0xddu : 0xfdu;
        for (wz_byte_t transfer_opcode = 0x40u; transfer_opcode <= 0x7fu;
             transfer_opcode += 1u) {
            wz_byte_t source_code = (wz_byte_t)(transfer_opcode & 0x07u);
            wz_byte_t target_code = (wz_byte_t)((transfer_opcode >> 3u) & 0x07u);
            wz_word_t expected_index;
            wz_byte_t expected_value;
            wz_byte_t* primary_register;

            if (source_code == 6u || target_code == 6u ||
                (source_code != 4u && source_code != 5u &&
                 target_code != 4u && target_code != 5u)) {
                continue;
            }
            if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
                fputs("machine reset before indexed byte-transfer matrix test failed\n", stderr);
                return 1;
            }
            machine.cpu.main.a = 0x70u;
            machine.cpu.main.b = 0x10u;
            machine.cpu.main.c = 0x20u;
            machine.cpu.main.d = 0x30u;
            machine.cpu.main.e = 0x40u;
            machine.cpu.main.h = 0x50u;
            machine.cpu.main.l = 0x60u;
            machine.cpu.main.f = 0x5au;
            machine.cpu.ix = 0xa1b2u;
            machine.cpu.iy = 0xc3d4u;
            machine.cpu.memptr = 0x2468u;
            machine.memory[0u] = prefix_value;
            machine.memory[1u] = transfer_opcode;
            expected_index = prefix_value == 0xddu ? machine.cpu.ix : machine.cpu.iy;
            if (source_code == 4u) {
                expected_value = (wz_byte_t)(expected_index >> 8u);
            } else if (source_code == 5u) {
                expected_value = (wz_byte_t)(expected_index & 0xffu);
            } else {
                primary_register = test_primary_register(&machine.cpu, source_code);
                expected_value = *primary_register;
            }
            if (target_code == 4u) {
                expected_index = (wz_word_t)((expected_index & 0x00ffu) |
                                              ((wz_word_t)expected_value << 8u));
            } else if (target_code == 5u) {
                expected_index = (wz_word_t)((expected_index & 0xff00u) |
                                              expected_value);
            }
            if (wz_z80_step(&machine) != WZ_RESULT_OK ||
                (prefix_value == 0xddu ? machine.cpu.ix : machine.cpu.iy) != expected_index ||
                (target_code != 4u && target_code != 5u &&
                 *test_primary_register(&machine.cpu, target_code) != expected_value) ||
                machine.cpu.main.f != 0x5au || machine.cpu.memptr != 0x2468u ||
                machine.cpu.program_counter != 2u || machine.cpu.r != 2u ||
                machine.master_tick != 16u) {
                fprintf(stderr, "Z80 indexed byte-transfer matrix failed: prefix=%02x opcode=%02x\n",
                        (unsigned)prefix_value, (unsigned)transfer_opcode);
                return 1;
            }
        }
    }

    {
        static const wz_byte_t indexed_memory_transfers[] = {
            0x46u, 0x4eu, 0x56u, 0x5eu, 0x66u, 0x6eu, 0x7eu,
            0x70u, 0x71u, 0x72u, 0x73u, 0x74u, 0x75u, 0x77u
        };
        for (size_t prefix_index = 0u; prefix_index < 2u; prefix_index += 1u) {
            wz_byte_t prefix_value = prefix_index == 0u ? 0xddu : 0xfdu;
            for (size_t transfer_index = 0u;
                 transfer_index < sizeof(indexed_memory_transfers) / sizeof(indexed_memory_transfers[0]);
                 transfer_index += 1u) {
                wz_byte_t transfer_opcode = indexed_memory_transfers[transfer_index];
                wz_byte_t source_code = (wz_byte_t)(transfer_opcode & 0x07u);
                wz_byte_t target_code = (wz_byte_t)((transfer_opcode >> 3u) & 0x07u);
                wz_word_t indexed_address;
                wz_byte_t expected_value;

                if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
                    fputs("machine reset before indexed-memory transfer matrix test failed\n", stderr);
                    return 1;
                }
                memset(&bus_log, 0, sizeof(bus_log));
                wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
                machine.cpu.main.a = 0x70u;
                machine.cpu.main.b = 0x10u;
                machine.cpu.main.c = 0x20u;
                machine.cpu.main.d = 0x30u;
                machine.cpu.main.e = 0x40u;
                machine.cpu.main.h = 0x50u;
                machine.cpu.main.l = 0x60u;
                machine.cpu.main.f = 0x5au;
                machine.cpu.ix = 0x4002u;
                machine.cpu.iy = 0x4002u;
                machine.memory[0u] = prefix_value;
                machine.memory[1u] = transfer_opcode;
                machine.memory[2u] = 0xfeu;
                indexed_address = 0x4000u;
                machine.memory[indexed_address] = 0xa5u;
                expected_value = source_code == 6u ? machine.memory[indexed_address] :
                    *test_primary_register(&machine.cpu, source_code);
                if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
                    wz_z80_step(&machine) != WZ_RESULT_OK ||
                    (source_code == 6u &&
                     *test_primary_register(&machine.cpu, target_code) != expected_value) ||
                    (target_code == 6u && machine.memory[indexed_address] != expected_value) ||
                    machine.cpu.main.f != 0x5au || machine.cpu.memptr != indexed_address ||
                    machine.cpu.program_counter != 3u || machine.cpu.r != 2u ||
                    machine.master_tick != 38u || bus_log.count != 5u ||
                    bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
                    bus_log.requests[2].master_tick != 16u ||
                    bus_log.requests[3].cycle != WZ_BUS_INTERNAL ||
                    bus_log.requests[3].master_tick != 22u ||
                    bus_log.requests[3].t_states != 5u ||
                    bus_log.requests[4].cycle != (source_code == 6u ?
                                                   WZ_BUS_MEMORY_READ : WZ_BUS_MEMORY_WRITE) ||
                    bus_log.requests[4].master_tick != 32u ||
                    bus_log.requests[4].address != indexed_address) {
                    fprintf(stderr,
                            "Z80 indexed-memory transfer matrix failed: prefix=%02x opcode=%02x\n",
                            (unsigned)prefix_value, (unsigned)transfer_opcode);
                    return 1;
                }
            }
        }
    }

    {
        static const wz_byte_t indexed_alu_source_codes[] = { 4u, 5u, 6u };
        static const wz_byte_t indexed_alu_operands[] = { 0x95u, 0x0fu, 0x42u };
        for (size_t prefix_index = 0u; prefix_index < 2u; prefix_index += 1u) {
            wz_byte_t prefix_value = prefix_index == 0u ? 0xddu : 0xfdu;
            for (wz_byte_t operation = 0u; operation < 8u; operation += 1u) {
                for (size_t source_index = 0u;
                     source_index < sizeof(indexed_alu_source_codes) / sizeof(indexed_alu_source_codes[0]);
                     source_index += 1u) {
                    wz_machine_t expected_machine;
                    wz_byte_t source_code = indexed_alu_source_codes[source_index];
                    wz_byte_t operand = indexed_alu_operands[source_index];
                    wz_byte_t indexed_opcode = (wz_byte_t)(0x80u + operation * 8u + source_code);
                    wz_word_t indexed_address = 0x9521u;

                    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK ||
                        wz_machine_init(&expected_machine, profile) != WZ_RESULT_OK) {
                        fputs("machine reset before indexed ALU matrix test failed\n", stderr);
                        return 1;
                    }
                    memset(&bus_log, 0, sizeof(bus_log));
                    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
                    machine.cpu.main.a = 0x63u;
                    machine.cpu.main.f = 0x01u;
                    machine.cpu.ix = 0x950fu;
                    machine.cpu.iy = 0x950fu;
                    machine.cpu.memptr = 0x2468u;
                    machine.memory[0u] = prefix_value;
                    machine.memory[1u] = indexed_opcode;
                    machine.memory[2u] = 0x12u;
                    machine.memory[indexed_address] = operand;
                    expected_machine.cpu.main.a = 0x63u;
                    expected_machine.cpu.main.b = operand;
                    expected_machine.cpu.main.f = 0x01u;
                    expected_machine.memory[0u] = (wz_byte_t)(0x80u + operation * 8u);
                    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
                        wz_z80_step(&expected_machine) != WZ_RESULT_OK ||
                        wz_z80_step(&machine) != WZ_RESULT_OK ||
                        machine.cpu.main.a != expected_machine.cpu.main.a ||
                        machine.cpu.main.f != expected_machine.cpu.main.f ||
                        machine.cpu.program_counter != (source_code == 6u ? 3u : 2u) ||
                        machine.cpu.r != 2u ||
                        machine.master_tick != (source_code == 6u ? 38u : 16u) ||
                        machine.cpu.memptr != (source_code == 6u ? indexed_address : 0x2468u) ||
                        bus_log.count != (source_code == 6u ? 5u : 2u) ||
                        (source_code == 6u &&
                         (bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
                          bus_log.requests[2].master_tick != 16u ||
                          bus_log.requests[3].cycle != WZ_BUS_INTERNAL ||
                          bus_log.requests[3].master_tick != 22u ||
                          bus_log.requests[3].t_states != 5u ||
                          bus_log.requests[4].cycle != WZ_BUS_MEMORY_READ ||
                          bus_log.requests[4].master_tick != 32u ||
                          bus_log.requests[4].address != indexed_address ||
                          bus_log.requests[4].value != operand))) {
                        fprintf(stderr, "Z80 indexed ALU matrix failed: prefix=%02x opcode=%02x\n",
                                (unsigned)prefix_value, (unsigned)indexed_opcode);
                        return 1;
                    }
                }
            }
        }
    }

    {
        static const wz_byte_t indexed_stack_opcodes[] = { 0xe1u, 0xe5u };
        for (size_t prefix_index = 0u; prefix_index < 2u; prefix_index += 1u) {
            wz_byte_t prefix_value = prefix_index == 0u ? 0xddu : 0xfdu;
            for (size_t opcode_index = 0u;
                 opcode_index < sizeof(indexed_stack_opcodes) / sizeof(indexed_stack_opcodes[0]);
                 opcode_index += 1u) {
                wz_byte_t stack_opcode = indexed_stack_opcodes[opcode_index];
                wz_word_t* selected_index;
                wz_word_t* unselected_index;

                if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
                    fputs("machine reset before indexed stack-pair matrix test failed\n", stderr);
                    return 1;
                }
                memset(&bus_log, 0, sizeof(bus_log));
                wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
                machine.cpu.main.f = 0x5au;
                machine.cpu.memptr = 0x2468u;
                machine.cpu.stack_pointer = 0x8000u;
                machine.memory[0u] = prefix_value;
                machine.memory[1u] = stack_opcode;
                selected_index = prefix_value == 0xddu ? &machine.cpu.ix : &machine.cpu.iy;
                unselected_index = prefix_value == 0xddu ? &machine.cpu.iy : &machine.cpu.ix;
                *selected_index = 0x1234u;
                *unselected_index = 0xbeefu;
                if (stack_opcode == 0xe1u) {
                    *selected_index = 0xdeadu;
                    machine.memory[0x8000u] = 0x34u;
                    machine.memory[0x8001u] = 0x12u;
                }
                if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
                    wz_z80_step(&machine) != WZ_RESULT_OK ||
                    *selected_index != 0x1234u || *unselected_index != 0xbeefu ||
                    machine.cpu.main.f != 0x5au || machine.cpu.memptr != 0x2468u ||
                    machine.cpu.program_counter != 2u || machine.cpu.r != 2u ||
                    machine.cpu.stack_pointer != (stack_opcode == 0xe1u ? 0x8002u : 0x7ffeu) ||
                    machine.master_tick != (stack_opcode == 0xe1u ? 28u : 30u) ||
                    bus_log.count != (stack_opcode == 0xe1u ? 4u : 5u) ||
                    (stack_opcode == 0xe1u &&
                     (bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
                      bus_log.requests[2].master_tick != 16u ||
                      bus_log.requests[2].address != 0x8000u ||
                      bus_log.requests[3].cycle != WZ_BUS_MEMORY_READ ||
                      bus_log.requests[3].master_tick != 22u ||
                      bus_log.requests[3].address != 0x8001u)) ||
                    (stack_opcode == 0xe5u &&
                     (bus_log.requests[2].cycle != WZ_BUS_INTERNAL ||
                      bus_log.requests[2].master_tick != 16u ||
                      bus_log.requests[2].t_states != 1u ||
                      bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
                      bus_log.requests[3].master_tick != 18u ||
                      bus_log.requests[3].address != 0x7fffu ||
                      bus_log.requests[3].value != 0x12u ||
                      bus_log.requests[4].cycle != WZ_BUS_MEMORY_WRITE ||
                      bus_log.requests[4].master_tick != 24u ||
                      bus_log.requests[4].address != 0x7ffeu ||
                      bus_log.requests[4].value != 0x34u))) {
                    fprintf(stderr, "Z80 indexed stack-pair matrix failed: prefix=%02x opcode=%02x\n",
                            (unsigned)prefix_value, (unsigned)stack_opcode);
                    return 1;
                }
                if (stack_opcode == 0xe5u &&
                    machine.memory[0x7ffeu] != 0x34u) {
                    fputs("Z80 indexed stack-pair low write failed\n", stderr);
                    return 1;
                }
            }
        }
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before indexed CB rotate test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.ix = 0x4002u;
    machine.memory[0u] = 0xddu;
    machine.memory[1u] = 0xcbu;
    machine.memory[2u] = 0xfeu;
    machine.memory[3u] = 0x00u;
    machine.memory[0x4000u] = 0x80u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x4000u] != 0x01u || machine.cpu.main.b != 0x01u ||
        machine.cpu.main.f != 0x01u || machine.cpu.memptr != 0x4000u ||
        machine.cpu.program_counter != 4u || machine.cpu.r != 2u ||
        machine.master_tick != 46u || bus_log.count != 7u ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 16u ||
        bus_log.requests[3].master_tick != 22u ||
        bus_log.requests[4].address != 0x4000u ||
        bus_log.requests[5].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[5].master_tick != 34u ||
        bus_log.requests[6].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[6].master_tick != 40u) {
        fputs("Z80 DDCB rotate/writeback trace failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before indexed CB bit test failed\n", stderr);
        return 1;
    }
    machine.cpu.iy = 0x3fffu;
    machine.cpu.main.b = 0x55u;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0xfdu;
    machine.memory[1u] = 0xcbu;
    machine.memory[2u] = 0x01u;
    machine.memory[3u] = 0x78u;
    machine.memory[0x4000u] = 0x80u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x4000u] != 0x80u || machine.cpu.main.b != 0x55u ||
        machine.cpu.main.f != 0x91u || machine.cpu.memptr != 0x4000u ||
        machine.cpu.program_counter != 4u || machine.cpu.r != 2u ||
        machine.master_tick != 40u) {
        fputs("Z80 FDCB BIT memory-only behavior failed\n", stderr);
        return 1;
    }

    machine.cpu.program_counter = 4u;
    machine.memory[4u] = 0xfdu;
    machine.memory[5u] = 0xcbu;
    machine.memory[6u] = 0x01u;
    machine.memory[7u] = 0xdeu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x4000u] != 0x88u || machine.cpu.main.f != 0x91u ||
        machine.cpu.program_counter != 8u || machine.master_tick != 86u) {
        fputs("Z80 FDCB SET memory-only behavior failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before ALU edge vectors failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x7fu;
    machine.memory[0u] = 0xc6u; machine.memory[1u] = 0x01u;
    machine.memory[2u] = 0xceu; machine.memory[3u] = 0x00u;
    machine.memory[4u] = 0xd6u; machine.memory[5u] = 0x01u;
    machine.memory[6u] = 0xdeu; machine.memory[7u] = 0x7fu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x80u || machine.cpu.main.f != 0x94u ||
        machine.cpu.program_counter != 2u || machine.master_tick != 14u) {
        fputs("Z80 ADD immediate flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0xffu; machine.cpu.main.f = 0x01u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x00u || machine.cpu.main.f != 0x51u) {
        fputs("Z80 ADC immediate flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x00u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xffu || machine.cpu.main.f != 0xbbu) {
        fputs("Z80 SUB immediate flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x80u; machine.cpu.main.f = 0x01u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x00u || machine.cpu.main.f != 0x56u) {
        fputs("Z80 SBC immediate flags failed\n", stderr);
        return 1;
    }

    machine.memory[8u] = 0xe6u; machine.memory[9u] = 0x3cu;
    machine.memory[10u] = 0xeeu; machine.memory[11u] = 0xffu;
    machine.memory[12u] = 0xf6u; machine.memory[13u] = 0x08u;
    machine.memory[14u] = 0xfeu; machine.memory[15u] = 0x28u;
    machine.cpu.main.a = 0xf0u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x30u || machine.cpu.main.f != 0x34u ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xcfu || machine.cpu.main.f != 0x8cu) {
        fputs("Z80 AND/XOR immediate flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x80u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x88u || machine.cpu.main.f != 0x8cu) {
        fputs("Z80 OR immediate flags failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x10u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x10u || machine.cpu.main.f != 0xbbu ||
        machine.cpu.program_counter != 16u || machine.master_tick != 112u) {
        fputs("Z80 CP immediate flags/writeback failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before ALU operand-path test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.a = 1u; machine.cpu.main.b = 2u;
    machine.cpu.main.h = 0x40u; machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0x80u; machine.memory[1u] = 0x86u;
    machine.memory[0x4000u] = 3u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.a != 3u ||
        machine.master_tick != 8u ||
        wz_z80_step(&machine) != WZ_RESULT_OK || machine.cpu.main.a != 6u ||
        machine.master_tick != 22u || bus_log.count != 3u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 16u ||
        bus_log.requests[2].address != 0x4000u) {
        fputs("Z80 ALU register/memory operand trace failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before CP operand-flag test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.a = 0xf5u;
    machine.cpu.main.b = 0x0fu;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0xb8u;
    machine.memory[1u] = 0xbeu;
    machine.memory[0x4000u] = 0x0fu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xf5u || machine.cpu.main.f != 0x9au ||
        machine.cpu.program_counter != 1u || machine.master_tick != 8u ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xf5u || machine.cpu.main.f != 0x9au ||
        machine.cpu.program_counter != 2u || machine.master_tick != 22u ||
        bus_log.count != 3u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 16u ||
        bus_log.requests[2].address != 0x4000u || bus_log.requests[2].value != 0x0fu) {
        fputs("Z80 CP operand X/Y flags or memory trace failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before BIT (HL) MEMPTR flag test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.f = 0x01u;
    machine.cpu.main.h = 0x61u;
    machine.cpu.main.l = 0x31u;
    machine.cpu.memptr = 0xff00u;
    machine.memory[0u] = 0xcbu;
    machine.memory[1u] = 0x46u;
    machine.memory[0x6131u] = 0xd5u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.f != 0x39u || machine.cpu.memptr != 0xff00u ||
        machine.cpu.program_counter != 2u || machine.cpu.r != 2u ||
        machine.master_tick != 24u) {
        fputs("Z80 BIT (HL) MEMPTR X/Y flags failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 unsupported ED-prefix test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xffu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_UNSUPPORTED_OPERATION ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 0u ||
        bus_log.count != 2u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[0].address != 0u ||
        bus_log.requests[0].value != 0xedu ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 4u ||
        bus_log.requests[1].address != 1u ||
        bus_log.requests[1].value != 0xffu) {
        fputs("Z80 ED-prefix fetch trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED neg test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x15u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x44u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0xebu ||
        machine.cpu.main.f != 0xbbu ||
        machine.cpu.r != 2u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 16u) {
        fputs("Z80 ED NEG failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED interrupt-mode test failed\n", stderr);
        return 1;
    }
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x5eu;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.interrupt_mode != (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2 ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 16u) {
        fputs("Z80 ED IM failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED I/R transfer test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x28u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x47u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.i != 0x28u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 18u) {
        fputs("Z80 ED LD I,A failed\n", stderr);
        return 1;
    }
    machine.cpu.iff2 = 1u;
    machine.cpu.main.f = 0x01u;
    machine.memory[2u] = 0xedu;
    machine.memory[3u] = 0x57u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x28u ||
        machine.cpu.main.f != 0x2du ||
        machine.cpu.program_counter != 4u ||
        machine.master_tick != 36u) {
        fputs("Z80 ED LD A,I failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED input register test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_cpu_fixture_input, (void*)&interrupt_value);
    machine.cpu.main.b = 0x12u;
    machine.cpu.main.c = 0xfeu;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x40u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        set_fixture_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0xffu ||
        machine.cpu.main.f != 0xadu ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 24u ||
        bus_log.count != 3u ||
        bus_log.requests[2].cycle != WZ_BUS_IO_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0x12feu ||
        bus_log.requests[2].value != 0xffu) {
        fputs("Z80 ED IN r,(C) trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED input flags-only test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.b = 0x12u;
    machine.cpu.main.c = 0xfeu;
    machine.cpu.main.f = 0u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x70u;
    wz_bus_input_init(&bus_input, read_cpu_fixture_input, (void*)&interrupt_value);
    if (set_fixture_bus_input(&machine, &bus_input) != WZ_RESULT_OK) {
        fputs("ED input fixture setup failed\n", stderr);
        return 1;
    }
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0x12u ||
        machine.cpu.main.f != 0xacu ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 24u) {
        fputs("Z80 ED IN (C) flags failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED output register test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 0x12u;
    machine.cpu.main.c = 0xfeu;
    machine.cpu.main.f = 0xa5u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x49u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.f != 0xa5u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 24u ||
        bus_log.count != 3u ||
        bus_log.requests[2].cycle != WZ_BUS_IO_WRITE ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0x12feu ||
        bus_log.requests[2].value != 0xfeu) {
        fputs("Z80 ED OUT (C),r trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED ADC HL,rr test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.h = 0x7fu;
    machine.cpu.main.l = 0xffu;
    machine.cpu.main.d = 0x00u;
    machine.cpu.main.e = 0x01u;
    machine.cpu.main.f = 0u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x5au;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.h != 0x80u ||
        machine.cpu.main.l != 0x00u ||
        machine.cpu.main.f != 0x94u ||
        machine.cpu.memptr != 0x8000u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 30u) {
        fputs("Z80 ED ADC HL,rr failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED SBC HL,rr test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.h = 0x00u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.stack_pointer = 0x0001u;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x72u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.h != 0xffu ||
        machine.cpu.main.l != 0xfeu ||
        machine.cpu.main.f != 0xbbu ||
        machine.cpu.memptr != 0x0001u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 30u) {
        fputs("Z80 ED SBC HL,rr failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED RRD test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.a = 0x84u;
    machine.cpu.main.f = 0x01u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x67u;
    machine.memory[0x4000u] = 0x31u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x81u ||
        machine.cpu.main.f != 0x85u ||
        machine.memory[0x4000u] != 0x43u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 36u ||
        bus_log.count != 4u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0x4000u ||
        bus_log.requests[2].value != 0x31u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 14u ||
        bus_log.requests[3].address != 0x4000u ||
        bus_log.requests[3].value != 0x43u ||
        machine.cpu.memptr != 0x4001u) {
        fputs("Z80 ED RRD trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED RLD test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x25u;
    machine.cpu.main.f = 0x01u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x6fu;
    machine.memory[0x4000u] = 0x96u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x29u ||
        machine.cpu.main.f != 0x29u ||
        machine.memory[0x4000u] != 0x65u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 36u) {
        fputs("Z80 ED RLD failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED LDI test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.a = 0x10u;
    machine.cpu.main.b = 0x00u;
    machine.cpu.main.c = 0x02u;
    machine.cpu.main.d = 0x50u;
    machine.cpu.main.e = 0x00u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.main.f = 0xd3u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xa0u;
    machine.memory[0x4000u] = 0x22u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x5000u] != 0x22u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.c != 0x01u ||
        machine.cpu.main.d != 0x50u ||
        machine.cpu.main.e != 0x01u ||
        machine.cpu.main.h != 0x40u ||
        machine.cpu.main.l != 0x01u ||
        machine.cpu.main.f != 0xe5u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 32u ||
        bus_log.count != 4u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0x4000u ||
        bus_log.requests[2].value != 0x22u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 14u ||
        bus_log.requests[3].address != 0x5000u ||
        bus_log.requests[3].value != 0x22u) {
        fputs("Z80 ED LDI trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED LDDR test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x01u;
    machine.cpu.main.b = 0x00u;
    machine.cpu.main.c = 0x02u;
    machine.cpu.main.d = 0x50u;
    machine.cpu.main.e = 0x00u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.main.f = 0xc1u;
    machine.cpu.memptr = 0x1234u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xb8u;
    machine.memory[0x4000u] = 0x01u;
    machine.memory[0x3fffu] = 0x07u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0u ||
        machine.master_tick != 42u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.c != 0x01u ||
        machine.cpu.main.d != 0x4fu ||
        machine.cpu.main.e != 0xffu ||
        machine.cpu.main.h != 0x3fu ||
        machine.cpu.main.l != 0xffu ||
        machine.cpu.main.f != 0xe5u ||
        machine.memory[0x5000u] != 0x01u ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 74u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.c != 0x00u ||
        machine.cpu.main.d != 0x4fu ||
        machine.cpu.main.e != 0xfeu ||
        machine.cpu.main.h != 0x3fu ||
        machine.cpu.main.l != 0xfeu ||
        machine.cpu.main.f != 0xc9u ||
        machine.cpu.memptr != 0x0001u ||
        machine.memory[0x4fffu] != 0x07u) {
        fputs("Z80 ED LDDR repeat failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED CPI test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.a = 0x10u;
    machine.cpu.main.b = 0x00u;
    machine.cpu.main.c = 0x02u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.main.f = 0xc1u;
    machine.cpu.memptr = 0x1234u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xa1u;
    machine.memory[0x4000u] = 0x01u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x10u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.c != 0x01u ||
        machine.cpu.main.h != 0x40u ||
        machine.cpu.main.l != 0x01u ||
        machine.cpu.main.f != 0x3fu ||
        machine.cpu.memptr != 0x1235u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 32u ||
        bus_log.count != 3u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0x4000u ||
        bus_log.requests[2].value != 0x01u) {
        fputs("Z80 ED CPI trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED CPDR test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x10u;
    machine.cpu.main.b = 0x00u;
    machine.cpu.main.c = 0x02u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.main.f = 0x01u;
    machine.cpu.memptr = 0x1234u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xb9u;
    machine.memory[0x4000u] = 0x01u;
    machine.memory[0x3fffu] = 0x10u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0u ||
        machine.master_tick != 42u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.c != 0x01u ||
        machine.cpu.main.h != 0x3fu ||
        machine.cpu.main.l != 0xffu ||
        machine.cpu.main.f != 0x3fu ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 74u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.c != 0x00u ||
        machine.cpu.main.h != 0x3fu ||
        machine.cpu.main.l != 0xfeu ||
        machine.cpu.main.f != 0x43u ||
        machine.cpu.memptr != 0x0000u) {
        fputs("Z80 ED CPDR repeat failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED block I/O test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_cpu_fixture_input, (void*)&interrupt_value);
    machine.cpu.main.b = 0x02u;
    machine.cpu.main.c = 0x00u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xb2u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        set_fixture_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0u ||
        machine.master_tick != 42u ||
        machine.cpu.main.b != 0x01u ||
        machine.cpu.main.h != 0x40u ||
        machine.cpu.main.l != 0x01u ||
        machine.cpu.main.f != 0x13u ||
        machine.memory[0x4000u] != 0xffu ||
        bus_log.count != 4u ||
        bus_log.requests[2].cycle != WZ_BUS_IO_READ ||
        bus_log.requests[2].address != 0x0200u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].address != 0x4000u ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 74u ||
        machine.cpu.main.b != 0x00u ||
        machine.cpu.main.h != 0x40u ||
        machine.cpu.main.l != 0x02u ||
        machine.cpu.main.f != 0x57u ||
        machine.cpu.memptr != 0x0101u ||
        machine.memory[0x4001u] != 0xffu) {
        fputs("Z80 ED INIR repeat failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED OUTD test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 0x02u;
    machine.cpu.main.c = 0x10u;
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0xabu;
    machine.memory[0x4000u] = 0x80u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0x01u ||
        machine.cpu.main.h != 0x3fu ||
        machine.cpu.main.l != 0xffu ||
        machine.cpu.main.f != 0x17u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 32u ||
        bus_log.count != 4u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].address != 0x4000u ||
        bus_log.requests[3].cycle != WZ_BUS_IO_WRITE ||
        bus_log.requests[3].address != 0x0110u ||
        bus_log.requests[3].value != 0x80u ||
        machine.cpu.memptr != 0x010fu) {
        fputs("Z80 ED OUTD trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED RETN test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.stack_pointer = 0xfffeu;
    machine.cpu.iff1 = 0u;
    machine.cpu.iff2 = 1u;
    machine.cpu.main.f = 0xa5u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x45u;
    machine.memory[0xfffeu] = 0x34u;
    machine.memory[0xffffu] = 0x12u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1234u ||
        machine.cpu.stack_pointer != 0u ||
        machine.cpu.iff1 != 1u ||
        machine.cpu.iff2 != 1u ||
        machine.cpu.main.f != 0xa5u ||
        machine.cpu.memptr != 0x1234u ||
        machine.master_tick != 28u ||
        bus_log.count != 4u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0xfffeu ||
        bus_log.requests[2].value != 0x34u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[3].master_tick != 14u ||
        bus_log.requests[3].address != 0xffffu ||
        bus_log.requests[3].value != 0x12u) {
        fputs("Z80 ED RETN trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED RETI test failed\n", stderr);
        return 1;
    }
    machine.cpu.stack_pointer = 0x4000u;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 0u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x4du;
    machine.memory[0x4000u] = 0x78u;
    machine.memory[0x4001u] = 0x56u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x5678u ||
        machine.cpu.stack_pointer != 0x4002u ||
        machine.cpu.iff1 != 0u ||
        machine.master_tick != 28u) {
        fputs("Z80 ED RETI failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED store pair test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 0x12u;
    machine.cpu.main.c = 0x34u;
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x43u;
    machine.memory[2u] = 0xfeu;
    machine.memory[3u] = 0xffu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0xfffeu] != 0x34u ||
        machine.memory[0xffffu] != 0x12u ||
        machine.cpu.program_counter != 4u ||
        machine.master_tick != 40u ||
        bus_log.count != 6u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 2u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[3].master_tick != 14u ||
        bus_log.requests[3].address != 3u ||
        bus_log.requests[4].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[4].master_tick != 20u ||
        bus_log.requests[4].address != 0xfffeu ||
        bus_log.requests[4].value != 0x34u ||
        bus_log.requests[5].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[5].master_tick != 26u ||
        bus_log.requests[5].address != 0xffffu ||
        bus_log.requests[5].value != 0x12u ||
        machine.cpu.memptr != 0xffffu) {
        fputs("Z80 ED store pair trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 ED load pair test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.memory[0u] = 0xedu;
    machine.memory[1u] = 0x7bu;
    machine.memory[2u] = 0x00u;
    machine.memory[3u] = 0x40u;
    machine.memory[0x4000u] = 0x78u;
    machine.memory[0x4001u] = 0x56u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.stack_pointer != 0x5678u ||
        machine.cpu.program_counter != 4u ||
        machine.master_tick != 40u ||
        bus_log.count != 6u ||
        bus_log.requests[4].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[4].master_tick != 20u ||
        bus_log.requests[4].address != 0x4000u ||
        bus_log.requests[4].value != 0x78u ||
        bus_log.requests[5].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[5].master_tick != 26u ||
        bus_log.requests[5].address != 0x4001u ||
        bus_log.requests[5].value != 0x56u) {
        fputs("Z80 ED load pair trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 CB memory rotate test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.h = 0x40u;
    machine.cpu.main.l = 0x00u;
    machine.memory[0u] = 0xcbu;
    machine.memory[1u] = 0x36u;
    machine.memory[0x4000u] = 0x80u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x4000u] != 0x01u ||
        machine.cpu.main.f != 0x01u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 30u ||
        bus_log.count != 4u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 8u ||
        bus_log.requests[2].address != 0x4000u ||
        bus_log.requests[2].value != 0x80u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 14u ||
        bus_log.requests[3].address != 0x4000u ||
        bus_log.requests[3].value != 0x01u) {
        fputs("Z80 CB memory rotate trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 CB bit test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.h = 0x28u;
    machine.cpu.main.l = 0x00u;
    machine.cpu.main.f = 0x01u;
    machine.cpu.memptr = 0x2800u;
    machine.memory[0u] = 0xcbu;
    machine.memory[1u] = 0x7eu;
    machine.memory[0x2800u] = 0x80u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0x2800u] != 0x80u ||
        machine.cpu.main.f != 0xb9u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 24u ||
        bus_log.count != 3u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].address != 0x2800u ||
        bus_log.requests[2].value != 0x80u) {
        fputs("Z80 CB bit trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 CB set/res test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.b = 0x80u;
    machine.cpu.main.f = 0xa5u;
    machine.memory[0u] = 0xcbu;
    machine.memory[1u] = 0xd8u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0x88u ||
        machine.cpu.main.f != 0xa5u ||
        machine.cpu.program_counter != 2u ||
        machine.master_tick != 16u) {
        fputs("Z80 CB set register failed\n", stderr);
        return 1;
    }
    machine.memory[2u] = 0xcbu;
    machine.memory[3u] = 0x80u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0x88u ||
        machine.cpu.main.f != 0xa5u ||
        machine.cpu.program_counter != 4u ||
        machine.master_tick != 32u) {
        fputs("Z80 CB res register failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 PUSH test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 0x12u;
    machine.cpu.main.c = 0x34u;
    machine.cpu.stack_pointer = 0u;
    machine.memory[0u] = 0xc5u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 1u ||
        machine.cpu.stack_pointer != 0xfffeu ||
        machine.memory[0xffffu] != 0x12u ||
        machine.memory[0xfffeu] != 0x34u ||
        machine.master_tick != 22u ||
        bus_log.count != 4u ||
        bus_log.requests[1].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[2].master_tick != 10u ||
        bus_log.requests[2].address != 0xffffu ||
        bus_log.requests[2].value != 0x12u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 16u ||
        bus_log.requests[3].address != 0xfffeu ||
        bus_log.requests[3].value != 0x34u) {
        fputs("Z80 PUSH wrapping trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 POP test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.stack_pointer = 0xfffeu;
    machine.memory[0u] = 0xf1u;
    machine.memory[0xfffeu] = 0xa5u;
    machine.memory[0xffffu] = 0x5au;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.a != 0x5au ||
        machine.cpu.main.f != 0xa5u ||
        machine.cpu.stack_pointer != 0u ||
        machine.master_tick != 20u ||
        bus_log.count != 3u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].address != 0xfffeu ||
        bus_log.requests[1].value != 0xa5u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 14u ||
        bus_log.requests[2].address != 0xffffu ||
        bus_log.requests[2].value != 0x5au) {
        fputs("Z80 POP AF wrapping trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 CALL test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.stack_pointer = 0u;
    machine.memory[0u] = 0xcdu;
    machine.memory[1u] = 0x78u;
    machine.memory[2u] = 0x56u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x5678u ||
        machine.cpu.memptr != 0x5678u ||
        machine.cpu.stack_pointer != 0xfffeu ||
        machine.memory[0xffffu] != 0u ||
        machine.memory[0xfffeu] != 3u ||
        machine.master_tick != 34u ||
        bus_log.count != 6u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 14u ||
        bus_log.requests[3].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[3].master_tick != 20u ||
        bus_log.requests[4].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[4].master_tick != 22u ||
        bus_log.requests[4].address != 0xffffu ||
        bus_log.requests[4].value != 0u ||
        bus_log.requests[5].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[5].master_tick != 28u ||
        bus_log.requests[5].address != 0xfffeu ||
        bus_log.requests[5].value != 3u) {
        fputs("Z80 CALL trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before untaken conditional CALL failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.f = 0x40u;
    machine.cpu.stack_pointer = 0x4000u;
    machine.memory[0u] = 0xc4u;
    machine.memory[1u] = 0x34u;
    machine.memory[2u] = 0x12u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 3u ||
        machine.cpu.memptr != 0x1234u ||
        machine.cpu.stack_pointer != 0x4000u ||
        machine.master_tick != 20u ||
        bus_log.count != 3u) {
        fputs("Z80 untaken conditional CALL trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before taken conditional CALL failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.stack_pointer = 0x8000u;
    machine.memory[0u] = 0xc4u;
    machine.memory[1u] = 0x34u;
    machine.memory[2u] = 0x12u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1234u ||
        machine.cpu.stack_pointer != 0x7ffeu ||
        machine.memory[0x7fffu] != 0u ||
        machine.memory[0x7ffeu] != 3u ||
        machine.master_tick != 34u ||
        bus_log.count != 6u) {
        fputs("Z80 taken conditional CALL trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 RET test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.stack_pointer = 0xffffu;
    machine.memory[0u] = 0xc9u;
    machine.memory[0xffffu] = 0x34u;
    machine.memory[0u] = 0x12u;
    machine.memory[1u] = 0xc9u;
    machine.cpu.program_counter = 1u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1234u ||
        machine.cpu.memptr != 0x1234u ||
        machine.cpu.stack_pointer != 1u ||
        machine.master_tick != 20u ||
        bus_log.count != 3u ||
        bus_log.requests[1].address != 0xffffu ||
        bus_log.requests[1].value != 0x34u ||
        bus_log.requests[2].address != 0u ||
        bus_log.requests[2].value != 0x12u) {
        fputs("Z80 RET wrapping trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before conditional RET tests failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.f = 0x40u;
    machine.cpu.stack_pointer = 0x4000u;
    machine.memory[0u] = 0xc0u;
    machine.memory[0x4000u] = 0x78u;
    machine.memory[0x4001u] = 0x56u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 1u ||
        machine.cpu.stack_pointer != 0x4000u ||
        machine.master_tick != 10u ||
        bus_log.count != 2u ||
        bus_log.requests[1].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[1].master_tick != 8u) {
        fputs("Z80 untaken conditional RET trace failed\n", stderr);
        return 1;
    }
    machine.cpu.main.f = 0u;
    machine.cpu.program_counter = 0u;
    machine.master_tick = 0u;
    memset(&bus_log, 0, sizeof(bus_log));
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x5678u ||
        machine.cpu.stack_pointer != 0x4002u ||
        machine.master_tick != 22u ||
        bus_log.count != 4u ||
        bus_log.requests[1].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 10u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[3].master_tick != 16u) {
        fputs("Z80 taken conditional RET trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 RST test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.stack_pointer = 1u;
    machine.memory[0x1234u] = 0xffu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x38u ||
        machine.cpu.memptr != 0x38u ||
        machine.cpu.stack_pointer != 0xffffu ||
        machine.memory[0u] != 0x12u ||
        machine.memory[0xffffu] != 0x35u ||
        machine.master_tick != 22u ||
        bus_log.count != 4u ||
        bus_log.requests[1].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[2].master_tick != 10u ||
        bus_log.requests[2].address != 0u ||
        bus_log.requests[2].value != 0x12u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 16u ||
        bus_log.requests[3].address != 0xffffu ||
        bus_log.requests[3].value != 0x35u) {
        fputs("Z80 RST wrapping trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 INC register test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.b = 0x7fu;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0x04u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0x80u ||
        machine.cpu.main.f != 0x95u ||
        machine.cpu.program_counter != 1u ||
        machine.master_tick != 8u) {
        fputs("Z80 INC register overflow flags failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 DEC register test failed\n", stderr);
        return 1;
    }
    machine.cpu.main.c = 0x80u;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0x0du;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.c != 0x7fu ||
        machine.cpu.main.f != 0x3fu ||
        machine.cpu.program_counter != 1u ||
        machine.master_tick != 8u) {
        fputs("Z80 DEC register overflow flags failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 INC memory test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.h = 0xffu;
    machine.cpu.main.l = 0xffu;
    machine.cpu.main.f = 0x01u;
    machine.memory[0u] = 0x34u;
    machine.memory[0xffffu] = 0xffu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.memory[0xffffu] != 0u ||
        machine.cpu.main.f != 0x51u ||
        machine.cpu.program_counter != 1u ||
        machine.master_tick != 22u ||
        bus_log.count != 4u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].address != 0xffffu ||
        bus_log.requests[1].value != 0xffu ||
        bus_log.requests[2].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[2].master_tick != 14u ||
        bus_log.requests[2].address != 0xffffu ||
        bus_log.requests[2].t_states != 1u ||
        bus_log.requests[3].cycle != WZ_BUS_MEMORY_WRITE ||
        bus_log.requests[3].master_tick != 16u ||
        bus_log.requests[3].address != 0xffffu ||
        bus_log.requests[3].value != 0u) {
        fputs("Z80 INC memory trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 JR test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.program_counter = 0xfffeu;
    machine.memory[0xfffeu] = 0x18u;
    machine.memory[0xffffu] = 0xfeu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0xfffeu ||
        machine.cpu.memptr != 0xfffeu ||
        machine.master_tick != 24u ||
        bus_log.count != 3u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].address != 0xffffu ||
        bus_log.requests[1].value != 0xfeu ||
        bus_log.requests[2].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[2].master_tick != 14u ||
        bus_log.requests[2].address != 0xffffu ||
        bus_log.requests[2].t_states != 5u) {
        fputs("Z80 JR signed wrapping trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before conditional JR test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.f = 0x40u;
    machine.cpu.memptr = 0x1234u;
    machine.memory[0u] = 0x20u;
    machine.memory[1u] = 0x40u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 2u ||
        machine.cpu.memptr != 0x1234u ||
        machine.master_tick != 14u ||
        bus_log.count != 2u) {
        fputs("Z80 untaken conditional JR trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 DJNZ taken test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 2u;
    machine.memory[0u] = 0x10u;
    machine.memory[1u] = 0xfeu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 1u ||
        machine.cpu.program_counter != 0u ||
        machine.cpu.memptr != 0u ||
        machine.master_tick != 26u ||
        bus_log.count != 4u ||
        bus_log.requests[1].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].t_states != 1u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 10u ||
        bus_log.requests[2].address != 1u ||
        bus_log.requests[3].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[3].master_tick != 16u ||
        bus_log.requests[3].address != 1u ||
        bus_log.requests[3].t_states != 5u) {
        fputs("Z80 DJNZ taken trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 DJNZ untaken test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.b = 1u;
    machine.cpu.memptr = 0x5678u;
    machine.memory[0u] = 0x10u;
    machine.memory[1u] = 0xfeu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.main.b != 0u ||
        machine.cpu.program_counter != 2u ||
        machine.cpu.memptr != 0x5678u ||
        machine.master_tick != 16u ||
        bus_log.count != 3u) {
        fputs("Z80 DJNZ untaken trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before conditional JP test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.f = 0x40u;
    machine.memory[0u] = 0xc2u;
    machine.memory[1u] = 0x34u;
    machine.memory[2u] = 0x12u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 3u ||
        machine.cpu.memptr != 0x1234u ||
        machine.master_tick != 20u ||
        bus_log.count != 3u ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].address != 1u ||
        bus_log.requests[2].master_tick != 14u ||
        bus_log.requests[2].address != 2u) {
        fputs("Z80 untaken conditional JP trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 JP HL test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.main.h = 0xcau;
    machine.cpu.main.l = 0xfeu;
    machine.cpu.memptr = 0x9abcu;
    machine.memory[0u] = 0xe9u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0xcafeu ||
        machine.cpu.memptr != 0x9abcu ||
        machine.master_tick != 8u ||
        bus_log.count != 1u) {
        fputs("Z80 JP HL trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 HALT test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.r = 0xffu;
    machine.memory[0x1234u] = 0x76u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.halted != 1u ||
        machine.cpu.program_counter != 0x1234u ||
        machine.cpu.r != 0x80u ||
        machine.master_tick != 8u ||
        bus_log.count != 1u ||
        bus_log.requests[0].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[0].address != 0x1234u ||
        bus_log.requests[0].value != 0x76u) {
        fputs("Z80 HALT entry failed\n", stderr);
        return 1;
    }
    machine.memory[0x1234u] = 0u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.halted != 1u ||
        machine.cpu.program_counter != 0x1234u ||
        machine.cpu.r != 0x81u ||
        machine.master_tick != 16u ||
        bus_log.count != 2u ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 8u ||
        bus_log.requests[1].address != 0x1234u ||
        bus_log.requests[1].value != 0u) {
        fputs("Z80 repeated halted M1 failed\n", stderr);
        return 1;
    }
    wz_z80_exit_halt_for_interrupt(&machine.cpu);
    if (machine.cpu.halted != 0u || machine.cpu.program_counter != 0x1235u ||
        machine.cpu.r != 0x81u || machine.master_tick != 16u) {
        fputs("Z80 accepted-interrupt HALT exit boundary failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 EI/DI test failed\n", stderr);
        return 1;
    }
    machine.memory[0u] = 0xfbu;
    machine.memory[1u] = 0x00u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.iff1 != 1u || machine.cpu.iff2 != 1u ||
        machine.cpu.interrupt_enable_delay != 1u ||
        wz_z80_maskable_interrupts_acceptable(&machine.cpu)) {
        fputs("Z80 EI did not defer maskable interrupt acceptance\n", stderr);
        return 1;
    }
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.interrupt_enable_delay != 0u ||
        !wz_z80_maskable_interrupts_acceptable(&machine.cpu)) {
        fputs("Z80 EI delay did not expire after one instruction\n", stderr);
        return 1;
    }
    machine.memory[2u] = 0xf3u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.iff1 != 0u || machine.cpu.iff2 != 0u ||
        machine.cpu.interrupt_enable_delay != 0u ||
        wz_z80_maskable_interrupts_acceptable(&machine.cpu)) {
        fputs("Z80 DI did not immediately disable maskable interrupts\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 EI HALT delay test failed\n", stderr);
        return 1;
    }
    machine.memory[0u] = 0xfbu;
    machine.memory[1u] = 0x76u;
    if (wz_z80_step(&machine) != WZ_RESULT_OK ||
        wz_z80_step(&machine) != WZ_RESULT_OK ||
        machine.cpu.halted != 1u || machine.cpu.interrupt_enable_delay != 0u ||
        !wz_z80_maskable_interrupts_acceptable(&machine.cpu)) {
        fputs("Z80 EI HALT acceptance boundary failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM0 interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    interrupt_value = 0x3eu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.stack_pointer = 0x8000u;
    machine.cpu.r = 0x7fu;
    machine.memory[0x1234u] = 0x5au;
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    wz_trace_sink_init(&trace_sink, record_timing_trace, &timing_trace_log);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        set_fixture_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1235u || machine.cpu.stack_pointer != 0x8000u ||
        machine.cpu.main.a != 0x5au ||
        machine.cpu.iff1 != 0u || machine.cpu.iff2 != 0u || machine.cpu.r != 0u ||
        machine.master_tick != 20u || bus_log.count != 2u ||
        timing_trace_log.events[0].kind != WZ_TRACE_INTERRUPT ||
        timing_trace_log.events[0].value != WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE ||
        timing_trace_log.events[1].kind != WZ_TRACE_INTERRUPT ||
        timing_trace_log.events[1].value != WZ_TRACE_INTERRUPT_MASKABLE_ACCEPT ||
        timing_trace_log.events[2].kind != WZ_TRACE_CPU_BUS ||
        timing_trace_log.events[2].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        timing_trace_log.events[0].sequence + 1u != timing_trace_log.events[1].sequence ||
        timing_trace_log.events[1].sequence + 1u != timing_trace_log.events[2].sequence ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[0].master_tick != 0u || bus_log.requests[0].t_states != 7u ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 14u || bus_log.requests[1].address != 0x1234u) {
        fputs("Z80 IM0 injected primary opcode test failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM0 CB interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    interrupt_value = 0xcbu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.r = 0x7fu;
    machine.cpu.main.b = 0x80u;
    machine.memory[0x1234u] = 0x00u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        set_fixture_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1235u || machine.cpu.main.b != 0x01u ||
        machine.cpu.r != 0x01u || machine.master_tick != 22u || bus_log.count != 2u ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 10u || bus_log.requests[1].address != 0x1234u) {
        fputs("Z80 IM0 injected CB opcode test failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM0 ED interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    interrupt_value = 0xedu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.r = 0x7fu;
    machine.cpu.main.a = 0x01u;
    machine.memory[0x1234u] = 0x44u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        set_fixture_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1235u || machine.cpu.main.a != 0xffu ||
        machine.cpu.r != 0x01u || machine.master_tick != 22u || bus_log.count != 2u ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 10u || bus_log.requests[1].address != 0x1234u) {
        fputs("Z80 IM0 injected ED opcode test failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM0 DD interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    interrupt_value = 0xddu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.r = 0x7fu;
    machine.memory[0x1234u] = 0x21u;
    machine.memory[0x1235u] = 0x78u;
    machine.memory[0x1236u] = 0x56u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        set_fixture_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1237u || machine.cpu.ix != 0x5678u ||
        machine.cpu.r != 0x01u || machine.master_tick != 34u || bus_log.count != 4u ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 14u || bus_log.requests[1].address != 0x1234u ||
        bus_log.requests[2].master_tick != 22u || bus_log.requests[3].master_tick != 28u) {
        fputs("Z80 IM0 injected DD opcode test failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM0 FD interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    interrupt_value = 0xfdu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.r = 0x7fu;
    machine.memory[0x1234u] = 0x21u;
    machine.memory[0x1235u] = 0x78u;
    machine.memory[0x1236u] = 0x56u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        set_fixture_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1237u || machine.cpu.iy != 0x5678u ||
        machine.cpu.r != 0x01u || machine.master_tick != 34u || bus_log.count != 4u ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].cycle != WZ_BUS_M1_OPCODE_FETCH ||
        bus_log.requests[1].master_tick != 14u || bus_log.requests[1].address != 0x1234u ||
        bus_log.requests[2].master_tick != 22u || bus_log.requests[3].master_tick != 28u) {
        fputs("Z80 IM0 injected FD opcode test failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM0 RST interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    wz_bus_observer_init(&bus_observer, record_bus_request, &bus_log);
    wz_bus_input_init(&bus_input, read_bus_input, (void*)&interrupt_value);
    interrupt_value = 0xffu;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.program_counter = 0x1234u;
    machine.cpu.stack_pointer = 0x8000u;
    machine.cpu.r = 0x7fu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        set_fixture_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x0038u || machine.cpu.stack_pointer != 0x7ffeu ||
        machine.memory[0x7fffu] != 0x12u || machine.memory[0x7ffeu] != 0x34u ||
        machine.cpu.r != 0u || machine.master_tick != 28u || bus_log.count != 4u ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].master_tick != 14u || bus_log.requests[2].master_tick != 16u ||
        bus_log.requests[3].master_tick != 22u) {
        fputs("Z80 IM0 injected RST opcode test failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM1 interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    wz_trace_sink_init(&trace_sink, record_timing_trace, &timing_trace_log);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_UNSUPPORTED_OPERATION ||
        timing_trace_log.count != 1u ||
        timing_trace_log.events[0].kind != WZ_TRACE_INTERRUPT ||
        timing_trace_log.events[0].value != WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE ||
        machine.cpu.program_counter != 0u || machine.cpu.stack_pointer != 0xffffu ||
        machine.cpu.iff1 != 0u || machine.cpu.iff2 != 0u ||
        machine.master_tick != 0u || bus_log.count != 0u) {
        fputs("Z80 rejected maskable interrupt trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset after rejected interrupt trace test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_1;
    machine.cpu.program_counter = 0x3456u;
    machine.cpu.stack_pointer = 0x8000u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x0038u || machine.memory[0x7fffu] != 0x34u ||
        machine.memory[0x7ffeu] != 0x56u || machine.master_tick != 26u ||
        bus_log.count != 3u || bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].master_tick != 14u || bus_log.requests[2].master_tick != 20u) {
        fputs("Z80 IM1 interrupt trace failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 IM2 interrupt test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    interrupt_value = 0x10u;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2;
    machine.cpu.i = 0x80u;
    machine.cpu.program_counter = 0x3456u;
    machine.cpu.stack_pointer = 0x8000u;
    machine.memory[0x8010u] = 0x34u;
    machine.memory[0x8011u] = 0x12u;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        set_fixture_bus_input(&machine, &bus_input) != WZ_RESULT_OK ||
        wz_z80_accept_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x1234u || machine.cpu.memptr != 0x1234u ||
        machine.memory[0x7fffu] != 0x34u || machine.memory[0x7ffeu] != 0x56u ||
        machine.master_tick != 38u || bus_log.count != 5u ||
        bus_log.requests[0].cycle != WZ_BUS_INTERRUPT_ACKNOWLEDGE ||
        bus_log.requests[1].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[1].master_tick != 14u || bus_log.requests[1].address != 0x8010u ||
        bus_log.requests[2].cycle != WZ_BUS_MEMORY_READ ||
        bus_log.requests[2].master_tick != 20u || bus_log.requests[2].address != 0x8011u ||
        bus_log.requests[3].master_tick != 26u || bus_log.requests[4].master_tick != 32u) {
        fputs("Z80 IM2 interrupt trace failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before Z80 NMI test failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 0u;
    machine.cpu.halted = 1u;
    machine.cpu.program_counter = 0x2000u;
    machine.cpu.stack_pointer = 0x8000u;
    machine.cpu.r = 0x7fu;
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK ||
        wz_z80_accept_nmi(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x0066u || machine.cpu.memptr != 0x0066u ||
        machine.cpu.halted != 0u || machine.cpu.iff1 != 0u || machine.cpu.iff2 != 1u ||
        machine.cpu.r != 0u || machine.memory[0x7fffu] != 0x20u ||
        machine.memory[0x7ffeu] != 0x01u || machine.master_tick != 22u ||
        bus_log.count != 3u || bus_log.requests[0].cycle != WZ_BUS_INTERNAL ||
        bus_log.requests[0].master_tick != 0u || bus_log.requests[0].t_states != 5u ||
        bus_log.requests[1].master_tick != 10u || bus_log.requests[2].master_tick != 16u) {
        fputs("Z80 NMI interrupt trace failed\n", stderr);
        return 1;
    }
    if (wz_machine_set_bus_input(&machine, 0) != WZ_RESULT_OK) {
        fputs("Z80 interrupt input removal failed\n", stderr);
        return 1;
    }

    wz_state_writer_init(&writer, serialized, sizeof(serialized));
    machine.ula_output = 0x1bu;
    machine.ula_output_tick = 1234u;
    machine.maskable_interrupt_line_low = 1u;
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK ||
        writer.length != WZ_STATE_SNAPSHOT_CAPACITY ||
        wz_state_hash_machine(&machine, &first_hash) != WZ_RESULT_OK) {
        fputs("canonical state serialization failed\n", stderr);
        return 1;
    }
    machine.cpu.main.a = 0x12u;
    machine.cpu.alternate.f = 0x34u;
    machine.cpu.ix = 0xabcdu;
    machine.cpu.iy = 0x2345u;
    machine.cpu.memptr = 0x6789u;
    machine.cpu.i = 0x56u;
    machine.cpu.r = 0x78u;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_enable_delay = 1u;
    machine.cpu.interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2;
    machine.cpu.halted = 1u;
    if (wz_machine_set_tape_loading_mode(&machine,
            WZ_TAPE_LOADING_INSTANT_TRAP) != WZ_RESULT_OK) {
        fputs("tape loading mode setup for state round trip failed\n", stderr);
        return 1;
    }
    /* Construct the reserved value only to exercise native-state persistence. */
    machine.networking_mode = WZ_NETWORKING_EAR_MIC;
    if (wz_machine_set_keyboard_key(&machine, 2u, 1u, true) != WZ_RESULT_OK) {
        fputs("keyboard state setup failed\n", stderr);
        return 1;
    }
    wz_state_writer_init(&writer, serialized, sizeof(serialized));
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK ||
        wz_state_deserialize_machine(&restored, serialized, writer.length) != WZ_RESULT_OK ||
        restored.cpu.main.a != 0x12u ||
        restored.cpu.alternate.f != 0x34u ||
        restored.cpu.ix != 0xabcdu ||
        restored.cpu.iy != 0x2345u ||
        restored.cpu.memptr != 0x6789u ||
        restored.cpu.i != 0x56u ||
        restored.cpu.r != 0x78u ||
        restored.cpu.iff1 != 1u ||
        restored.cpu.iff2 != 1u ||
        restored.cpu.interrupt_enable_delay != 1u ||
        restored.cpu.interrupt_mode != (wz_byte_t)WZ_Z80_INTERRUPT_MODE_2 ||
        restored.cpu.halted != 1u ||
        restored.keyboard_rows[2u] != 0x1du ||
        restored.ula_output != 0x1bu || restored.ula_output_tick != 1234u ||
        restored.maskable_interrupt_line_low != 1u ||
        restored.tape_loading_mode != WZ_TAPE_LOADING_INSTANT_TRAP ||
        restored.networking_mode != WZ_NETWORKING_EAR_MIC) {
        fputs("Z80 state round trip failed\n", stderr);
        return 1;
    }
    serialized[71u] = 2u;
    restored.cpu.main.a = 0x7eu;
    if (wz_state_deserialize_machine(&restored, serialized, writer.length) !=
            WZ_RESULT_INVALID_STATE || restored.cpu.main.a != 0x7eu) {
        fputs("invalid tape loading mode was not rejected atomically\n", stderr);
        return 1;
    }
    serialized[72u] = 3u;
    if (wz_state_deserialize_machine(&restored, serialized, writer.length) !=
            WZ_RESULT_INVALID_STATE || restored.cpu.main.a != 0x7eu) {
        fputs("invalid networking mode was not rejected atomically\n", stderr);
        return 1;
    }
    serialized[71u] = (wz_byte_t)WZ_TAPE_LOADING_INSTANT_TRAP;
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reinitialization failed\n", stderr);
        return 1;
    }
    wz_state_writer_init(&writer, serialized, sizeof(serialized));
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK ||
        wz_state_hash_machine(&machine, &first_hash) != WZ_RESULT_OK) {
        fputs("canonical state baseline refresh failed\n", stderr);
        return 1;
    }
    machine.memory[0] = 0x42u;
    if (wz_state_hash_machine(&machine, &second_hash) != WZ_RESULT_OK ||
        first_hash == second_hash) {
        fputs("canonical state hash did not reflect machine state\n", stderr);
        return 1;
    }
    if (wz_state_deserialize_machine(&restored, serialized, writer.length) != WZ_RESULT_OK ||
        wz_state_hash_machine(&restored, &second_hash) != WZ_RESULT_OK ||
        first_hash != second_hash) {
        fputs("canonical state round trip failed\n", stderr);
        return 1;
    }

    wz_trace_sink_init(&trace_sink, record_trace, &trace_count);
    if (wz_headless_runner_init(&runner, &machine, &trace_sink) != WZ_RESULT_OK ||
        wz_headless_runner_advance(&runner, 3u) != WZ_RESULT_OK ||
        machine.master_tick != 3u || trace_count != 3u) {
        fputs("headless runner or trace sink failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before interrupt line schedule test failed\n", stderr);
        return 1;
    }
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    wz_trace_sink_init(&trace_sink, record_timing_trace, &timing_trace_log);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    wz_machine_update_interrupt_line(&machine);
    machine.master_tick = 64u;
    wz_machine_update_interrupt_line(&machine);
    machine.master_tick = 139776u;
    wz_machine_update_interrupt_line(&machine);
    machine.master_tick = 139840u;
    wz_machine_update_interrupt_line(&machine);
    if (wz_machine_maskable_interrupt_line_low(&machine) ||
        timing_trace_log.count != 4u ||
        timing_trace_log.events[0].value != WZ_TRACE_INTERRUPT_LINE_ASSERT ||
        timing_trace_log.events[0].master_tick != 0u ||
        timing_trace_log.events[1].value != WZ_TRACE_INTERRUPT_LINE_DEASSERT ||
        timing_trace_log.events[1].master_tick != 64u ||
        timing_trace_log.events[2].value != WZ_TRACE_INTERRUPT_LINE_ASSERT ||
        timing_trace_log.events[2].master_tick != 139776u ||
        timing_trace_log.events[3].value != WZ_TRACE_INTERRUPT_LINE_DEASSERT ||
        timing_trace_log.events[3].master_tick != 139840u) {
        fputs("profile-driven interrupt line schedule failed\n", stderr);
        return 1;
    }
    wz_machine_set_timing_trace(&machine, 0);

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before interrupt sampling test failed\n", stderr);
        return 1;
    }
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    wz_trace_sink_init(&trace_sink, record_timing_trace, &timing_trace_log);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_1;
    machine.cpu.program_counter = 0x3456u;
    machine.cpu.stack_pointer = 0x8000u;
    wz_machine_update_interrupt_line(&machine);
    if (wz_z80_sample_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        timing_trace_log.count < 3u ||
        timing_trace_log.events[0].value != WZ_TRACE_INTERRUPT_LINE_ASSERT ||
        timing_trace_log.events[1].value != WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE ||
        timing_trace_log.events[2].value != WZ_TRACE_INTERRUPT_MASKABLE_ACCEPT ||
        machine.cpu.program_counter != 0x0038u) {
        fputs("maskable interrupt sampling acceptance order failed\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before interrupt deassertion sample test failed\n", stderr);
        return 1;
    }
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    wz_trace_sink_init(&trace_sink, record_timing_trace, &timing_trace_log);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    wz_machine_update_interrupt_line(&machine);
    machine.master_tick = 64u;
    if (wz_z80_sample_maskable_interrupt(&machine) != WZ_RESULT_UNSUPPORTED_OPERATION ||
        timing_trace_log.count != 3u ||
        timing_trace_log.events[0].value != WZ_TRACE_INTERRUPT_LINE_ASSERT ||
        timing_trace_log.events[1].value != WZ_TRACE_INTERRUPT_LINE_DEASSERT ||
        timing_trace_log.events[2].value != WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE ||
        machine.cpu.program_counter != 0u) {
        fputs("maskable interrupt deassertion sampling failed\n", stderr);
        return 1;
    }
    wz_machine_set_timing_trace(&machine, 0);

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before combined Phase-3 evidence failed\n", stderr);
        return 1;
    }
    memset(&bus_log, 0, sizeof(bus_log));
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    wz_trace_sink_init(&trace_sink, record_timing_trace, &timing_trace_log);
    if (wz_machine_set_bus_observer(&machine, &bus_observer) != WZ_RESULT_OK) {
        fputs("combined Phase-3 bus observer setup failed\n", stderr);
        return 1;
    }
    wz_machine_set_timing_trace(&machine, &trace_sink);
    machine.master_tick = 28670u;
    wz_bus_request_init(&bus_request, WZ_BUS_MEMORY_READ, machine.master_tick,
                        0x4000u, 0u, 3u);
    if (wz_machine_bus_request(&machine, &bus_request) != WZ_RESULT_OK ||
        bus_request.contention_delay != 6u || bus_request.master_tick != 28682u ||
        bus_request.source != WZ_BUS_SOURCE_MEMORY) {
        fputs("combined Phase-3 contention evidence failed\n", stderr);
        return 1;
    }
    machine.master_tick = 0u;
    machine.cpu.iff1 = 1u;
    machine.cpu.iff2 = 1u;
    machine.cpu.interrupt_mode = (wz_byte_t)WZ_Z80_INTERRUPT_MODE_1;
    machine.cpu.program_counter = 0x3456u;
    machine.cpu.stack_pointer = 0x8000u;
    if (wz_z80_sample_maskable_interrupt(&machine) != WZ_RESULT_OK ||
        machine.cpu.program_counter != 0x0038u ||
        machine.cpu.stack_pointer != 0x7ffeu ||
        bus_log.count < 4u ||
        timing_trace_log.count < 4u ||
        timing_trace_log.events[0].kind != WZ_TRACE_CPU_BUS ||
        timing_trace_log.events[1].kind != WZ_TRACE_INTERRUPT ||
        timing_trace_log.events[1].value != WZ_TRACE_INTERRUPT_LINE_ASSERT ||
        timing_trace_log.events[2].kind != WZ_TRACE_INTERRUPT ||
        timing_trace_log.events[2].value != WZ_TRACE_INTERRUPT_MASKABLE_SAMPLE ||
        timing_trace_log.events[3].kind != WZ_TRACE_INTERRUPT ||
        timing_trace_log.events[3].value != WZ_TRACE_INTERRUPT_MASKABLE_ACCEPT) {
        fputs("combined Phase-3 interrupt evidence failed\n", stderr);
        return 1;
    }
    wz_machine_set_timing_trace(&machine, 0);
    wz_machine_set_bus_observer(&machine, 0);

    wz_scheduler_init(&scheduler);
    if (wz_scheduler_schedule(&scheduler, 10u, WZ_EVENT_EXTERNAL,
                              record_event, &dispatched) != WZ_RESULT_OK ||
        wz_scheduler_schedule(&scheduler, 10u, WZ_EVENT_CPU,
                              record_event, &dispatched) != WZ_RESULT_OK ||
        wz_scheduler_dispatch_next(&scheduler) != WZ_RESULT_OK ||
        dispatched != 1u) {
        fputs("same-tick scheduler ordering failed\n", stderr);
        return 1;
    }

    if (strstr(wz_machine_boot_message(), "Warajevo") == NULL) {
        fputs("bootstrap message does not identify the project\n", stderr);
        return 1;
    }

    remove(trace_path);
    if (wz_trace_file_create(&trace_file, trace_path, 1u,
                             (wz_dword_t)profile->kind, 0x1234u, UINT32_MAX) != WZ_RESULT_OK ||
        wz_trace_file_create(&duplicate, trace_path, 2u,
                             (wz_dword_t)profile->kind, 0x1234u, UINT32_MAX) == WZ_RESULT_OK) {
        fputs("exclusive trace creation failed\n", stderr);
        return 1;
    }
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    for (wz_qword_t index = 0u; index < 800000u; ++index) {
        wz_trace_emit(&trace_sink, WZ_TRACE_MASTER_TICK_ADVANCED, index);
    }
    if (wz_trace_file_freeze(&trace_file) != WZ_RESULT_OK) {
        fputs("trace freeze failed\n", stderr);
        return 1;
    }
    wz_trace_emit(&trace_sink, WZ_TRACE_MASTER_TICK_ADVANCED, 800001u);
    wz_trace_file_close(&trace_file);
    if (wz_trace_file_recover(trace_path, recover_trace, &recovered_last,
                              &recovered_count) != WZ_RESULT_OK ||
        recovered_count < (8u * 69888u) || recovered_last != 799999u) {
        fputs("trace wrap recovery failed\n", stderr);
        return 1;
    }
    trace_stream = fopen(trace_path, "r+b");
    if (trace_stream == NULL || fseek(trace_stream, 0, SEEK_END) != 0 ||
        ftell(trace_stream) != (long)WZ_TRACE_FILE_SIZE ||
        fseek(trace_stream,
              (long)(WZ_TRACE_HEADER_SIZE +
                     (799999u % ((WZ_TRACE_FILE_SIZE - WZ_TRACE_HEADER_SIZE) /
                                  WZ_TRACE_RECORD_SIZE)) * WZ_TRACE_RECORD_SIZE +
                     WZ_TRACE_COMMIT_OFFSET),
              SEEK_SET) != 0 || fputc(0, trace_stream) == EOF ||
        fclose(trace_stream) != 0) {
        fputs("trace size or truncation fixture failed\n", stderr);
        return 1;
    }
    recovered_last = 0u;
    if (wz_trace_file_recover(trace_path, recover_trace, &recovered_last,
                              &recovered_count) != WZ_RESULT_OK ||
        recovered_last != 799998u) {
        fputs("incomplete trace record was not rejected\n", stderr);
        return 1;
    }
    remove(trace_path);

    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        fputs("machine reset before timing-full retention test failed\n", stderr);
        return 1;
    }
    remove(timing_full_trace_path);
    if (wz_trace_file_create(&trace_file, timing_full_trace_path, 5u,
                             (wz_dword_t)profile->kind, 0x9abcu, UINT32_MAX) != WZ_RESULT_OK) {
        fputs("timing-full retention trace creation failed\n", stderr);
        return 1;
    }
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    wz_machine_set_timing_trace(&machine, &trace_sink);
    for (wz_qword_t index = 0u; index < 160000u; ++index) {
        if (wz_z80_step(&machine) != WZ_RESULT_OK) {
            wz_trace_file_close(&trace_file);
            remove(timing_full_trace_path);
            fputs("timing-full retention workload execution failed\n", stderr);
            return 1;
        }
    }
    if (wz_trace_file_freeze(&trace_file) != WZ_RESULT_OK) {
        wz_trace_file_close(&trace_file);
        remove(timing_full_trace_path);
        fputs("timing-full retention trace freeze failed\n", stderr);
        return 1;
    }
    wz_trace_file_close(&trace_file);
    memset(&retention_trace_log, 0, sizeof(retention_trace_log));
    if (wz_trace_file_recover(timing_full_trace_path, recover_retention_trace,
                              &retention_trace_log, &recovered_count) != WZ_RESULT_OK ||
        retention_trace_log.count != recovered_count ||
        retention_trace_log.count < (8u * 69888u) ||
        !retention_trace_log.saw_bus || !retention_trace_log.saw_instruction ||
        retention_trace_log.last_master_tick > machine.master_tick ||
        retention_trace_log.first_master_tick >
            machine.master_tick - (8u * 69888u)) {
        remove(timing_full_trace_path);
        fputs("timing-full eight-frame retention failed\n", stderr);
        return 1;
    }
    trace_stream = fopen(timing_full_trace_path, "rb");
    if (trace_stream == NULL || fseek(trace_stream, 0, SEEK_END) != 0 ||
        ftell(trace_stream) != (long)WZ_TRACE_FILE_SIZE || fclose(trace_stream) != 0) {
        remove(timing_full_trace_path);
        fputs("timing-full retention trace capacity failed\n", stderr);
        return 1;
    }
    wz_machine_set_timing_trace(&machine, 0);
    remove(timing_full_trace_path);

    remove(state_trace_path);
    if (wz_trace_file_create(&trace_file, state_trace_path, 3u,
                             (wz_dword_t)profile->kind, 0x5678u, UINT32_MAX) != WZ_RESULT_OK) {
        fputs("state trace creation failed\n", stderr);
        return 1;
    }
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    for (wz_byte_t chunk = 0u; chunk < 5u; ++chunk) {
        static const wz_qword_t state_chunks[5] = {
            UINT64_C(0x8877665544332211),
            UINT64_C(0x0123456789abcdef),
            UINT64_C(0x13572468def09abc),
            UINT64_C(0x020101013c9a2468),
            UINT64_C(0x0000000000000000)
        };
        wz_trace_event_t state_event = {0};
        state_event.kind = WZ_TRACE_CPU_STATE_SYNC;
        state_event.master_tick = 1234u;
        state_event.cycle = chunk;
        state_event.register_snapshot = state_chunks[chunk];
        wz_trace_emit_detail(&trace_sink, &state_event);
    }
    {
        wz_trace_event_t state_delta = {0};
        state_delta.kind = WZ_TRACE_CPU_STATE_DELTA;
        state_delta.master_tick = 1242u;
        state_delta.cycle = 2u;
        state_delta.register_snapshot = UINT64_C(0xbeef2468def09abc);
        wz_trace_emit_detail(&trace_sink, &state_delta);
    }
    if (wz_trace_file_freeze(&trace_file) != WZ_RESULT_OK) {
        fputs("state trace freeze failed\n", stderr);
        return 1;
    }
    wz_trace_file_close(&trace_file);
    memset(&timing_trace_log, 0, sizeof(timing_trace_log));
    if (wz_trace_file_recover(state_trace_path, recover_timing_trace, &timing_trace_log,
                              &recovered_count) != WZ_RESULT_OK ||
        recovered_count != 6u || timing_trace_log.count != 6u ||
        timing_trace_log.events[0].kind != WZ_TRACE_CPU_STATE_SYNC ||
        timing_trace_log.events[0].register_snapshot != UINT64_C(0x8877665544332211) ||
        timing_trace_log.events[0].master_tick != 1234u ||
        timing_trace_log.events[4].master_tick != 1234u ||
        timing_trace_log.events[4].cycle != 4u ||
        timing_trace_log.events[4].register_snapshot != 0u ||
        timing_trace_log.events[5].kind != WZ_TRACE_CPU_STATE_DELTA ||
        timing_trace_log.events[5].master_tick != 1242u ||
        timing_trace_log.events[5].cycle != 2u) {
        fputs("state trace recovery failed\n", stderr);
        return 1;
    }
    wz_trace_cpu_state_sync_init(&recovered_cpu_sync);
    {
        bool recovered_state = false;
    for (size_t index = 0u; index < timing_trace_log.count; ++index) {
            recovered_state = wz_trace_cpu_state_sync_apply(&recovered_cpu_sync,
                                                             &timing_trace_log.events[index]);
        }
        if (!recovered_state || recovered_cpu_sync.master_tick != 1242u ||
            recovered_cpu_sync.state.main.a != 0x11u ||
            recovered_cpu_sync.state.main.l != 0x88u ||
            recovered_cpu_sync.state.alternate.a != 0xefu ||
            recovered_cpu_sync.state.alternate.l != 0x01u ||
            recovered_cpu_sync.state.ix != 0x9abcu ||
            recovered_cpu_sync.state.iy != 0xdef0u ||
            recovered_cpu_sync.state.stack_pointer != 0x2468u ||
            recovered_cpu_sync.state.program_counter != 0xbeefu ||
            recovered_cpu_sync.state.memptr != 0x2468u ||
            recovered_cpu_sync.state.i != 0x9au || recovered_cpu_sync.state.r != 0x3cu ||
            recovered_cpu_sync.state.iff1 != 1u || recovered_cpu_sync.state.iff2 != 1u ||
            recovered_cpu_sync.state.interrupt_enable_delay != 1u ||
            recovered_cpu_sync.state.interrupt_mode != WZ_Z80_INTERRUPT_MODE_2 ||
            recovered_cpu_sync.state.halted != 0u) {
            fprintf(stderr, "state trace decoder failed: complete=%u tick=%llu pc=%04x sp=%04x im=%u\n",
                    recovered_state ? 1u : 0u,
                    (unsigned long long)recovered_cpu_sync.master_tick,
                    (unsigned)recovered_cpu_sync.state.program_counter,
                    (unsigned)recovered_cpu_sync.state.stack_pointer,
                    (unsigned)recovered_cpu_sync.state.interrupt_mode);
            return 1;
        }
    }
    remove(state_trace_path);

    wz_machine_destroy(&machine);
    wz_machine_destroy(&restored);
    return 0;
}
