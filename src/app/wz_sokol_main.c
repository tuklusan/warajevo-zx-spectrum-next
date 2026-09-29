/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. */

#if !defined(_WIN32) && !defined(__APPLE__)
#define _POSIX_C_SOURCE 200809L
#endif
#define SOKOL_NO_ENTRY
#define SOKOL_IMPL
#if defined(_WIN32)
#define SOKOL_D3D11
#elif defined(__APPLE__)
#define SOKOL_METAL
#else
#define SOKOL_GLCORE
#define SOKOL_X11
#endif
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_audio.h"
#include "sokol_glue.h"
#include "sokol_gl.h"
#include "sokol_time.h"
#define NK_IMPLEMENTATION
#include "app/wz_nuklear_config.h"
#include "sokol_nuklear.h"

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#include <process.h>
#else
#include <errno.h>
#include <time.h>
#include <unistd.h>
#endif

#include "core/wz_machine.h"
#include "diagnostics/wz_trace_file.h"
#include "core/wz_keyboard_matrix.h"
#include "core/wz_runner.h"
#include "core/wz_tape.h"
#include "core/wz_state.h"
#include "core/audio/wz_audio_mixer.h"
#include "app/wz_command_registry.h"
#include "app/wz_file_dialog.h"
#include "app/wz_file_open_run.h"
#include "app/wz_networking_commands.h"
#include "app/wz_tape_loading_commands.h"
#include "app/wz_tape_media_commands.h"
#include "app/wz_application_lifecycle.h"
#include "app/wz_control_port.h"
#include "app/wz_host_socket.h"
#include "app/wz_input_arbiter.h"
#include "app/wz_input_focus.h"
#include "app/wz_sokol_audio.h"
#include "app/wz_host_pacing.h"
#include "app/wz_host_audio_policy.h"
#include "app/wz_speed_policy.h"
#include "app/wz_telnet_client.h"
#include "app/wz_telnet_keyboard_command.h"
#include "app/wz_telnet_status.h"
#include "app/wz_telnet_alias_response.h"
#include "app/wz_telnet_negotiation.h"
#include "app/wz_telnet_input.h"
#include "app/wz_telnet_key_press.h"
#include "app/wz_ui_window.h"

#define WZ_HOST_COMMAND_CAPACITY 128u
#define WZ_HOST_RASTER_BYTES (WZ_RASTER_CANONICAL_WIDTH * WZ_RASTER_CANONICAL_HEIGHT)
#define WZ_HOST_TELNET_IO_CAPACITY 2048u
#define WZ_HOST_AUDIO_FRAME_SAMPLE_CAPACITY 2048u

static const uint8_t wz_host_palette[16u][4u] = {
    {0u, 0u, 0u, 255u}, {0u, 0u, 205u, 255u}, {205u, 0u, 0u, 255u},
    {205u, 0u, 205u, 255u}, {0u, 205u, 0u, 255u}, {0u, 205u, 205u, 255u},
    {205u, 205u, 0u, 255u}, {205u, 205u, 205u, 255u},
    {0u, 0u, 0u, 255u}, {0u, 0u, 255u, 255u}, {255u, 0u, 0u, 255u},
    {255u, 0u, 255u, 255u}, {0u, 255u, 0u, 255u}, {0u, 255u, 255u, 255u},
    {255u, 255u, 0u, 255u}, {255u, 255u, 255u, 255u}
};

typedef struct {
    wz_machine_t machine;
#ifndef NDEBUG
    wz_trace_file_t timing_trace_file;
    wz_trace_sink_t timing_trace_sink;
    bool timing_trace_initialized;
#endif
    wz_byte_t model_48k_rom[WZ_48K_ROM_SIZE];
    wz_qword_t model_48k_rom_identity;
    bool has_model_48k_rom;
    wz_sokol_audio_t audio;
    wz_host_pacing_t pacing;
    wz_speed_policy_t speed;
    wz_speed_policy_t audio_sample_speed;
    wz_qword_t audio_sample_remainder;
    wz_application_lifecycle_t lifecycle;
    wz_ui_window_t ui_window;
    wz_headless_runner_t runner;
    wz_input_arbiter_t input_arbiter;
    wz_telnet_key_press_state_t telnet_key_presses;
    wz_input_focus_controller_t input_focus;
    bool local_left_shift_down;
    bool local_right_shift_down;
    bool local_left_control_down;
    bool local_right_control_down;
    bool tape_enter_down;
    wz_control_port_owner_t control_port;
    wz_telnet_client_gate_t telnet_client;
    wz_command_registry_t command_registry;
    wz_command_metadata_t command_storage[WZ_HOST_COMMAND_CAPACITY];
    wz_networking_command_context_t networking_command_context;
    wz_tape_loading_command_context_t tape_loading_command_context;
    wz_tape_media_command_context_t tape_media_command_context;
    bool tape_manager_open;
    size_t tape_manager_selected_segment;
    size_t tape_manager_selected_block;
    size_t tape_manager_block_page_start;
    wz_raster_buffer_t raster;
    wz_byte_t raster_samples[WZ_HOST_RASTER_BYTES];
    wz_byte_t raster_rgba[WZ_HOST_RASTER_BYTES * 4u];
    wz_presentation_snapshot_t snapshot;
    wz_byte_t snapshot_samples[WZ_HOST_RASTER_BYTES];
    wz_telnet_negotiator_t telnet_negotiator;
    wz_telnet_command_buffer_t telnet_commands;
    uint8_t telnet_input[WZ_HOST_TELNET_IO_CAPACITY];
    uint8_t telnet_application[WZ_HOST_TELNET_IO_CAPACITY];
    uint8_t telnet_protocol[WZ_HOST_TELNET_IO_CAPACITY];
    uint8_t telnet_command[WZ_TELNET_COMMAND_CAPACITY];
    wz_tape_segment_t* tape_segments;
    size_t tape_segment_count;
    wz_byte_t* tape_image_data;
    size_t tape_image_length;
    wz_tap_block_t* tape_blocks;
    size_t tape_block_count;
    wz_tzx_block_t* tzx_blocks;
    size_t tzx_block_count;
    char tape_source_path[4096];
    char tape_format[8];
    wz_byte_t* microdrive_data;
    wz_mdr_image_t microdrive_image;
    char file_notification[WZ_COMMAND_MESSAGE_CAPACITY];
    sg_image raster_image;
    sg_view raster_view;
    sg_sampler raster_sampler;
    bool graphics_initialized;
    bool ui_toolkit_initialized;
    bool pacing_initialized;
    bool audio_sample_speed_initialized;
    bool socket_system_initialized;
    bool remote_settings_visible;
    bool initialized;
} wz_host_session_t;

static wz_host_session_t wz_host_session;
static void wz_host_release_local_keys(void);

static wz_qword_t wz_host_now_nanoseconds(void)
{
    double nanoseconds = stm_ns(stm_now());
    return nanoseconds > 0.0 ? (wz_qword_t)nanoseconds : 0u;
}

#ifndef NDEBUG
static void wz_host_timing_trace_start(wz_host_session_t* session)
{
    char path[1024];
    wz_qword_t session_id = wz_host_now_nanoseconds();
    unsigned long process_id;
    int path_length;
    wz_result_t result;

#if defined(_WIN32)
    char directory[MAX_PATH];
    DWORD directory_length = GetTempPathA((DWORD)sizeof(directory), directory);
    if (directory_length == 0u || directory_length >= sizeof(directory)) {
        (void)fprintf(stderr, "WZSN timing trace disabled: temporary path unavailable.\n");
        return;
    }
    process_id = (unsigned long)_getpid();
    path_length = snprintf(path, sizeof(path), "%swzsn-timing-%lu-%llu.wzt",
                           directory, process_id,
                           (unsigned long long)session_id);
#else
    const char* directory = getenv("TMPDIR");
    const char* separator;
    if (directory == NULL || directory[0] == '\0') directory = "/tmp";
    process_id = (unsigned long)getpid();
    separator = directory[strlen(directory) - 1u] == '/' ? "" : "/";
    path_length = snprintf(path, sizeof(path), "%s%swzsn-timing-%lu-%llu.wzt",
                           directory, separator, process_id,
                           (unsigned long long)session_id);
#endif
    if (path_length < 0 || (size_t)path_length >= sizeof(path)) {
        (void)fprintf(stderr, "WZSN timing trace disabled: path is too long.\n");
        return;
    }
    result = wz_trace_file_create(&session->timing_trace_file, path, session_id,
        (wz_dword_t)session->machine.profile->kind,
        session->machine.rom_identity, UINT32_MAX);
    if (result != WZ_RESULT_OK) {
        (void)fprintf(stderr, "WZSN timing trace disabled: create failed (%d).\n",
                      (int)result);
        return;
    }
    wz_trace_sink_init(&session->timing_trace_sink, wz_trace_file_emit,
                       &session->timing_trace_file);
    wz_machine_set_timing_trace(&session->machine, &session->timing_trace_sink);
    session->timing_trace_initialized = true;
    (void)fprintf(stderr, "WZSN timing trace: %s\n", path);
}

static void wz_host_timing_trace_stop(wz_host_session_t* session)
{
    if (!session->timing_trace_initialized) return;
    wz_machine_set_timing_trace(&session->machine, NULL);
    if (wz_trace_file_freeze(&session->timing_trace_file) != WZ_RESULT_OK) {
        (void)fprintf(stderr, "WZSN timing trace: freeze failed.\n");
    }
    wz_trace_file_close(&session->timing_trace_file);
    session->timing_trace_initialized = false;
}
#endif

static bool wz_host_sleep_nanoseconds(wz_qword_t nanoseconds, void* context)
{
    (void)context;
#if defined(_WIN32)
    {
        wz_qword_t milliseconds =
            nanoseconds / UINT64_C(1000000) +
            (nanoseconds % UINT64_C(1000000) != 0u ? 1u : 0u);
        if (milliseconds > UINT32_MAX) {
            milliseconds = UINT32_MAX;
        }
        Sleep((DWORD)milliseconds);
    }
#else
    {
        struct timespec request;
        request.tv_sec = (time_t)(nanoseconds / UINT64_C(1000000000));
        request.tv_nsec = (long)(nanoseconds % UINT64_C(1000000000));
        while (nanosleep(&request, &request) != 0) {
            if (errno != EINTR) {
                return false;
            }
        }
    }
#endif
    return true;
}

static void wz_host_audio_clear_frame_events(wz_machine_t* machine)
{
    machine->beeper.event_count = 0u;
    machine->ay.event_count = 0u;
}

static bool wz_host_audio_replay_ay_event(wz_ay_t* ay,
                                          const wz_ay_event_t* event)
{
    if (event->kind == WZ_AY_EVENT_REGISTER_SELECT) {
        return wz_ay_select_register(ay, event->value,
                                     event->master_tick) == WZ_RESULT_OK;
    }
    if (event->kind == WZ_AY_EVENT_REGISTER_WRITE) {
        return wz_ay_write_data(ay, event->value,
                                event->master_tick) == WZ_RESULT_OK;
    }
    return false;
}

static void wz_host_audio_render_frame(wz_host_session_t* session,
                                       wz_master_tick_t start_tick,
                                       wz_byte_t initial_beeper_level,
                                       const wz_ay_t* initial_ay)
{
    wz_audio_sample_t samples[WZ_HOST_AUDIO_FRAME_SAMPLE_CAPACITY];
    wz_machine_t* machine = &session->machine;
    const wz_machine_profile_t* profile = machine->profile;
    wz_qword_t master_hz;
    wz_qword_t elapsed_ticks;
    wz_qword_t scaled_samples;
    wz_qword_t effective_sample_rate;
    wz_qword_t render_sample_rate;
    wz_qword_t rate_remainder;
    wz_ay_t audio_ay;
    wz_master_tick_t sample_cursor;
    wz_qword_t sample_whole_ticks;
    wz_qword_t sample_remainder_ticks;
    wz_qword_t sample_tick_phase = 0u;
    size_t sample_count;
    size_t event_index = 0u;
    size_t event_count = machine->ay.event_count;

    if (!wz_host_audio_enabled(session->speed) ||
        !wz_sokol_audio_valid(&session->audio) || initial_ay == NULL ||
        profile == NULL || profile->master_hz_den == 0u) {
        session->audio_sample_remainder = 0u;
        session->audio_sample_speed_initialized = false;
        wz_sokol_audio_discard_pending(&session->audio);
        wz_host_audio_clear_frame_events(machine);
        return;
    }
    if (!session->audio_sample_speed_initialized ||
        session->audio_sample_speed != session->speed) {
        session->audio_sample_remainder = 0u;
        session->audio_sample_speed = session->speed;
        session->audio_sample_speed_initialized = true;
    }

    master_hz = profile->master_hz_num / profile->master_hz_den;
    elapsed_ticks = machine->master_tick - start_tick;
    effective_sample_rate =
        (wz_qword_t)WZ_CANONICAL_AUDIO_SAMPLE_RATE * 100u /
        wz_speed_policy_percent(session->speed);
    if (master_hz == 0u || effective_sample_rate == 0u ||
        effective_sample_rate > master_hz ||
        elapsed_ticks > (UINT64_MAX - session->audio_sample_remainder) /
                            effective_sample_rate) {
        session->audio_sample_remainder = 0u;
        wz_host_audio_clear_frame_events(machine);
        return;
    }
    scaled_samples = elapsed_ticks * effective_sample_rate +
                     session->audio_sample_remainder;
    sample_count = (size_t)(scaled_samples / master_hz);
    rate_remainder = scaled_samples % master_hz;
    if (sample_count == 0u ||
        sample_count > WZ_HOST_AUDIO_FRAME_SAMPLE_CAPACITY) {
        session->audio_sample_remainder = rate_remainder;
        wz_host_audio_clear_frame_events(machine);
        return;
    }
    session->audio_sample_remainder = rate_remainder;

    /* Select a rate whose final sample boundary cannot pass this frame. */
    {
        wz_qword_t rate_numerator = (wz_qword_t)sample_count * master_hz;
        render_sample_rate = rate_numerator / elapsed_ticks;
        if (rate_numerator % elapsed_ticks != 0u) {
            ++render_sample_rate;
        }
    }
    if (!wz_beeper_render_pcm(machine->beeper.events,
                              machine->beeper.event_count,
                              initial_beeper_level, start_tick, master_hz,
                              render_sample_rate, samples, sample_count)) {
        wz_host_audio_clear_frame_events(machine);
        return;
    }

    audio_ay = *initial_ay;
    audio_ay.event_count = 0u;
    sample_cursor = start_tick;
    sample_whole_ticks = master_hz / render_sample_rate;
    sample_remainder_ticks = master_hz % render_sample_rate;
    for (size_t sample_index = 0u; sample_index < sample_count; ++sample_index) {
        wz_qword_t duration = sample_whole_ticks;
        wz_master_tick_t sample_end;

        if (sample_tick_phase >= render_sample_rate - sample_remainder_ticks) {
            ++duration;
            sample_tick_phase -= render_sample_rate - sample_remainder_ticks;
        } else {
            sample_tick_phase += sample_remainder_ticks;
        }
        if (duration == 0u || UINT64_MAX - sample_cursor < duration) {
            wz_host_audio_clear_frame_events(machine);
            return;
        }
        sample_end = sample_cursor + duration;
        while (event_index < event_count &&
               machine->ay.events[event_index].master_tick < sample_end) {
            const wz_ay_event_t* event = &machine->ay.events[event_index];
            if (event->master_tick < sample_cursor ||
                wz_ay_advance_master_ticks(
                    &audio_ay, event->master_tick - sample_cursor) != WZ_RESULT_OK ||
                !wz_host_audio_replay_ay_event(&audio_ay, event)) {
                wz_host_audio_clear_frame_events(machine);
                return;
            }
            sample_cursor = event->master_tick;
            ++event_index;
        }
        if (wz_ay_advance_master_ticks(&audio_ay,
                                       sample_end - sample_cursor) != WZ_RESULT_OK) {
            wz_host_audio_clear_frame_events(machine);
            return;
        }
        sample_cursor = sample_end;
        samples[sample_index] =
            wz_audio_mixer_sample(samples[sample_index], &audio_ay);
    }

    while (event_index < event_count &&
           machine->ay.events[event_index].master_tick <= machine->master_tick) {
        const wz_ay_event_t* event = &machine->ay.events[event_index];
        if (event->master_tick < sample_cursor ||
            wz_ay_advance_master_ticks(&audio_ay,
                                       event->master_tick - sample_cursor) != WZ_RESULT_OK ||
            !wz_host_audio_replay_ay_event(&audio_ay, event)) {
            wz_host_audio_clear_frame_events(machine);
            return;
        }
        sample_cursor = event->master_tick;
        ++event_index;
    }
    if (sample_cursor < machine->master_tick &&
        wz_ay_advance_master_ticks(&audio_ay,
                                   machine->master_tick - sample_cursor) != WZ_RESULT_OK) {
        wz_host_audio_clear_frame_events(machine);
        return;
    }
    (void)wz_sokol_audio_push(&session->audio, session->speed,
                             samples, sample_count);
    session->ui_window.layout.audio_degraded =
        wz_sokol_audio_degraded(&session->audio);
    wz_host_audio_clear_frame_events(machine);
}

static bool wz_host_read_file(const char* path, wz_byte_t** data, size_t* length)
{
    FILE* file;
    long file_length;
    wz_byte_t* storage;
    size_t read_length;
    if (path == NULL || data == NULL || length == NULL || path[0] == '\0') return false;
#if defined(_WIN32)
    {
        int wide_length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                               path, -1, NULL, 0);
        wchar_t* wide_path;
        if (wide_length <= 0) return false;
        wide_path = (wchar_t*)malloc((size_t)wide_length * sizeof(*wide_path));
        if (wide_path == NULL) return false;
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1,
                                wide_path, wide_length) != wide_length) {
            free(wide_path);
            return false;
        }
        file = _wfopen(wide_path, L"rb");
        free(wide_path);
    }
#else
    file = fopen(path, "rb");
#endif
    if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
        if (file != NULL) fclose(file);
        return false;
    }
    file_length = ftell(file);
    if (file_length <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }
    storage = (wz_byte_t*)malloc((size_t)file_length);
    if (storage == NULL) {
        fclose(file);
        return false;
    }
    read_length = fread(storage, 1u, (size_t)file_length, file);
    fclose(file);
    if (read_length != (size_t)file_length) {
        free(storage);
        return false;
    }
    *data = storage;
    *length = read_length;
    return true;
}

static bool wz_host_load_external_rom(wz_host_session_t* session, const char* path)
{
    wz_byte_t* data = NULL;
    size_t length = 0u;
    bool loaded;
    if (session == NULL || !wz_host_read_file(path, &data, &length)) return false;
    loaded = wz_machine_load_48k_rom(&session->machine, data, length) == WZ_RESULT_OK;
    free(data);
    return loaded;
}

static bool wz_host_extension_is(const char* path, const char* expected)
{
    const char* extension = strrchr(path, '.');
    if (extension == NULL) return false;
    while (*extension != '\0' && *expected != '\0') {
        char left = *extension >= 'A' && *extension <= 'Z'
            ? (char)(*extension - 'A' + 'a') : *extension;
        char right = *expected >= 'A' && *expected <= 'Z'
            ? (char)(*expected - 'A' + 'a') : *expected;
        if (left != right) return false;
        ++extension;
        ++expected;
    }
    return *extension == '\0' && *expected == '\0';
}

static bool wz_host_mount_tape_segments(wz_host_session_t* session,
                                        wz_tape_segment_t* segments,
                                        size_t segment_count)
{
    if (wz_machine_mount_tape(&session->machine, segments, segment_count) !=
        WZ_RESULT_OK) return false;
    free(session->tape_segments);
    session->tape_segments = segments;
    session->tape_segment_count = segment_count;
    session->ui_window.layout.tape_mounted = true;
    return true;
}

static wz_result_t wz_host_index_standard_tap(const wz_byte_t* data,
                                               size_t length,
                                               wz_tap_block_t** blocks_out,
                                               size_t* count_out)
{
    size_t offset = 0u;
    size_t block_count = 0u;
    size_t block_index = 0u;
    wz_tap_block_t* blocks;

    if (data == NULL || length == 0u || blocks_out == NULL ||
        count_out == NULL) return WZ_RESULT_INVALID_ARGUMENT;
    *blocks_out = NULL;
    *count_out = 0u;
    while (offset < length) {
        size_t block_length;
        if (length - offset < 2u) return WZ_RESULT_PARSE_ERROR;
        block_length = (size_t)wz_read_le16(data + offset);
        offset += 2u;
        if (block_length == 0u || block_length > length - offset ||
            block_count == SIZE_MAX) return WZ_RESULT_PARSE_ERROR;
        ++block_count;
        offset += block_length;
    }
    if (block_count > SIZE_MAX / sizeof(*blocks)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    blocks = (wz_tap_block_t*)malloc(block_count * sizeof(*blocks));
    if (blocks == NULL) return WZ_RESULT_INVALID_STATE;
    offset = 0u;
    while (offset < length && block_index < block_count) {
        size_t block_length = (size_t)wz_read_le16(data + offset);
        offset += 2u;
        blocks[block_index].data = data + offset;
        blocks[block_index].length = block_length;
        offset += block_length;
        ++block_index;
    }
    *blocks_out = blocks;
    *count_out = block_count;
    return WZ_RESULT_OK;
}

static size_t wz_host_read_le24(const wz_byte_t* data)
{
    return (size_t)data[0] | ((size_t)data[1] << 8u) |
        ((size_t)data[2] << 16u);
}

static bool wz_host_load_external_tape(wz_host_session_t* session,
                                       const char* path)
{
    wz_byte_t* data = NULL;
    size_t length = 0u;
    size_t segment_count = 0u;
    wz_tape_segment_t* segments = NULL;
    wz_tap_block_t* tap_blocks = NULL;
    wz_tzx_block_t* blocks = NULL;
    size_t block_count = 0u;
    size_t tap_block_count = 0u;
    const wz_machine_profile_t* profile;
    wz_result_t parsed = WZ_RESULT_INVALID_STATE;
    bool loaded = false;
    if (session == NULL || !wz_host_read_file(path, &data, &length)) return false;
    profile = session->machine.profile;
    if (profile == NULL || profile->master_hz_den == 0u) goto cleanup;
    if (wz_host_extension_is(path, ".tap")) {
        parsed = wz_tape_parse_standard_tap(data, length,
            profile->master_ticks_per_cpu_tstate, NULL, 0u, &segment_count);
        if (parsed == WZ_RESULT_BUFFER_TOO_SMALL && segment_count != 0u &&
            segment_count <= SIZE_MAX / sizeof(*segments)) {
            segments = (wz_tape_segment_t*)malloc(segment_count * sizeof(*segments));
            if (segments != NULL) parsed = wz_tape_parse_standard_tap(
                data, length, profile->master_ticks_per_cpu_tstate, segments,
                segment_count, &segment_count);
        }
        if (parsed == WZ_RESULT_OK) {
            parsed = wz_host_index_standard_tap(data, length, &tap_blocks,
                                                &tap_block_count);
        }
    } else if (wz_host_extension_is(path, ".tzx")) {
        parsed = wz_tape_parse_tzx(data, length, NULL, 0u, &block_count);
        if (parsed == WZ_RESULT_BUFFER_TOO_SMALL && block_count != 0u &&
            block_count <= SIZE_MAX / sizeof(*blocks)) {
            blocks = (wz_tzx_block_t*)malloc(block_count * sizeof(*blocks));
            if (blocks == NULL || wz_tape_parse_tzx(data, length, blocks,
                    block_count, &block_count) != WZ_RESULT_OK) goto cleanup;
            parsed = wz_tape_expand_tzx_timing(blocks, block_count,
                profile->master_ticks_per_cpu_tstate, NULL, 0u, &segment_count);
            if (parsed == WZ_RESULT_BUFFER_TOO_SMALL && segment_count != 0u &&
                segment_count <= SIZE_MAX / sizeof(*segments)) {
                segments = (wz_tape_segment_t*)malloc(segment_count * sizeof(*segments));
                if (segments != NULL) parsed = wz_tape_expand_tzx_timing(
                    blocks, block_count, profile->master_ticks_per_cpu_tstate,
                    segments, segment_count, &segment_count);
            }
        }
    } else if (wz_host_extension_is(path, ".wav")) {
        wz_qword_t master_hz = profile->master_hz_num / profile->master_hz_den;
        if (master_hz > UINT32_MAX) goto cleanup;
        parsed = wz_tape_parse_wav_pcm(data, length, (wz_dword_t)master_hz,
                                      128u, 8u, NULL, 0u, &segment_count);
        if (parsed == WZ_RESULT_BUFFER_TOO_SMALL && segment_count != 0u &&
            segment_count <= SIZE_MAX / sizeof(*segments)) {
            segments = (wz_tape_segment_t*)malloc(segment_count * sizeof(*segments));
            if (segments != NULL) parsed = wz_tape_parse_wav_pcm(
                data, length, (wz_dword_t)master_hz, 128u, 8u, segments,
                segment_count, &segment_count);
        }
    }
    if (parsed == WZ_RESULT_OK && segments != NULL && segment_count != 0u &&
        wz_host_mount_tape_segments(session, segments, segment_count)) {
        segments = NULL;
        free(session->tape_image_data);
        free(session->tape_blocks);
        free(session->tzx_blocks);
        session->tape_image_data = data;
        session->tape_image_length = length;
        session->tape_blocks = tap_blocks;
        session->tape_block_count = tap_block_count;
        session->tzx_blocks = blocks;
        session->tzx_block_count = block_count;
        data = NULL;
        tap_blocks = NULL;
        blocks = NULL;
        (void)snprintf(session->tape_source_path,
                       sizeof(session->tape_source_path), "%s", path);
        (void)snprintf(session->tape_format, sizeof(session->tape_format),
            "%s", wz_host_extension_is(path, ".tap") ? "TAP" :
            wz_host_extension_is(path, ".tzx") ? "TZX" : "WAV");
        session->tape_manager_selected_segment = 0u;
        session->tape_manager_selected_block = 0u;
        session->tape_manager_block_page_start = 0u;
        loaded = true;
    }
cleanup:
    free(blocks);
    free(tap_blocks);
    free(segments);
    free(data);
    return loaded;
}

static bool wz_host_load_external_snapshot(wz_host_session_t* session,
                                           const char* path)
{
    wz_byte_t* data = NULL;
    size_t length = 0u;
    wz_snapshot_state_t snapshot;
    wz_result_t result = WZ_RESULT_INVALID_STATE;
    if (session == NULL || !wz_host_read_file(path, &data, &length)) return false;
    if (wz_host_extension_is(path, ".sna") &&
        length == WZ_SNA_128K_LENGTH) {
        result = wz_state_load_sna_128k(&session->machine, data, length);
    } else if (wz_host_extension_is(path, ".z80") &&
               length >= WZ_Z80_V2_HEADER_LENGTH && data[6u] == 0u &&
               data[7u] == 0u && wz_read_le16(data + 30u) == 23u &&
               data[34u] == 4u && session->machine.profile != NULL &&
               session->machine.profile->kind == WZ_MACHINE_128K_PAL) {
        result = wz_state_load_z80_v2_128k(&session->machine, data, length);
    } else if (wz_host_extension_is(path, ".sna") &&
               length == WZ_SNA_48K_LENGTH) {
        wz_snapshot_state_init(&snapshot);
        result = wz_snapshot_state_load_sna_48k(&snapshot, data, length);
        if (result == WZ_RESULT_OK) result = wz_state_deserialize_machine(
            &session->machine, wz_snapshot_state_data(&snapshot),
            wz_snapshot_state_length(&snapshot));
    } else if (wz_host_extension_is(path, ".z80")) {
        wz_snapshot_state_init(&snapshot);
        if (length > 30u && data[6u] == 0u) {
            result = wz_snapshot_state_load_z80_v1(&snapshot, data, length);
        } else if (length > WZ_Z80_V2_HEADER_LENGTH &&
                   wz_read_le16(data + 30u) == 23u) {
            result = wz_snapshot_state_load_z80_v2(&snapshot, data, length);
        } else if (length > WZ_Z80_V3_HEADER_LENGTH &&
                   (wz_read_le16(data + 30u) == 54u ||
                    wz_read_le16(data + 30u) == 55u)) {
            result = wz_snapshot_state_load_z80_v3(&snapshot, data, length);
        }
        if (result == WZ_RESULT_OK) result = wz_state_deserialize_machine(
            &session->machine, wz_snapshot_state_data(&snapshot),
            wz_snapshot_state_length(&snapshot));
    }
    free(data);
    return result == WZ_RESULT_OK;
}

static bool wz_host_load_external_microdrive(wz_host_session_t* session,
                                             const char* path)
{
    wz_byte_t* data = NULL;
    size_t length = 0u;
    wz_mdr_image_t image;
    if (session == NULL || wz_mdr_transport_is_dirty(&session->machine.microdrive) ||
        !wz_host_read_file(path, &data, &length)) return false;
    if (wz_mdr_image_init(&image, data, length) != WZ_RESULT_OK ||
        wz_mdr_transport_mount(&session->machine.microdrive, &image) !=
            WZ_RESULT_OK) {
        free(data);
        return false;
    }
    free(session->microdrive_data);
    session->microdrive_data = data;
    session->microdrive_image = image;
    session->machine.microdrive.image = &session->microdrive_image;
    session->ui_window.layout.microdrive1_mounted = true;
    session->ui_window.layout.microdrive_mounted[0] = true;
    return true;
}

static bool wz_host_open_run_tape(const char* path, void* context)
{
    return wz_host_load_external_tape((wz_host_session_t*)context, path);
}

static bool wz_host_open_run_snapshot(const char* path, void* context)
{
    return wz_host_load_external_snapshot((wz_host_session_t*)context, path);
}

static bool wz_host_open_run_microdrive(const char* path, void* context)
{
    return wz_host_load_external_microdrive((wz_host_session_t*)context, path);
}

static bool wz_host_tape_media_load(const char* path, void* context)
{
    return wz_host_load_external_tape((wz_host_session_t*)context, path);
}

static void wz_host_tape_media_release(void* context)
{
    wz_host_session_t* session = (wz_host_session_t*)context;
    if (session == NULL) return;
    free(session->tape_segments);
    session->tape_segments = NULL;
    session->tape_segment_count = 0u;
    free(session->tape_image_data);
    session->tape_image_data = NULL;
    session->tape_image_length = 0u;
    free(session->tape_blocks);
    session->tape_blocks = NULL;
    session->tape_block_count = 0u;
    free(session->tzx_blocks);
    session->tzx_blocks = NULL;
    session->tzx_block_count = 0u;
    session->tape_source_path[0] = '\0';
    session->tape_format[0] = '\0';
    session->tape_manager_selected_segment = 0u;
    session->tape_manager_selected_block = 0u;
    session->tape_manager_block_page_start = 0u;
    session->ui_window.layout.tape_mounted = false;
    (void)snprintf(session->file_notification,
                   sizeof(session->file_notification), "Tape ejected");
}

static void wz_host_tape_manager_open(void* context)
{
    wz_host_session_t* session = (wz_host_session_t*)context;
    if (session != NULL) session->tape_manager_open = true;
}

static void wz_host_ui_insert_tape(void)
{
    char path[4096];
    wz_file_dialog_result_t dialog_result;
    wz_command_result_t result;
    wz_qword_t sleep_nanoseconds;
    wz_host_release_local_keys();
    if (!wz_input_focus_dialog_enter(&wz_host_session.input_focus)) {
        (void)snprintf(wz_host_session.file_notification,
            sizeof(wz_host_session.file_notification), "File dialog unavailable");
        return;
    }
    dialog_result = wz_file_dialog_open(path, sizeof(path));
    (void)wz_input_focus_dialog_leave(&wz_host_session.input_focus);
    if (wz_host_session.pacing_initialized) {
        (void)wz_host_pacing_wait(&wz_host_session.pacing,
            wz_host_now_nanoseconds(), wz_host_session.machine.master_tick,
            NULL, NULL, &sleep_nanoseconds);
    }
    if (dialog_result == WZ_FILE_DIALOG_CANCELLED) {
        (void)snprintf(wz_host_session.file_notification,
            sizeof(wz_host_session.file_notification), "Tape insert cancelled");
        return;
    }
    if (dialog_result != WZ_FILE_DIALOG_SELECTED) {
        (void)snprintf(wz_host_session.file_notification,
            sizeof(wz_host_session.file_notification), "File dialog failed");
        return;
    }
    if (wz_command_registry_dispatch(&wz_host_session.command_registry,
            WZ_TAPE_INSERT_COMMAND_ID,
            (wz_command_arguments_t){path, strlen(path)}, &result) !=
        WZ_RESULT_OK) {
        (void)snprintf(wz_host_session.file_notification,
            sizeof(wz_host_session.file_notification), "%s",
            result.reason == NULL ? "Tape insert failed" : result.reason);
        return;
    }
    (void)snprintf(wz_host_session.file_notification,
        sizeof(wz_host_session.file_notification), "Tape inserted");
}

static wz_result_t wz_host_command_open_run(
    const void* context, wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    wz_host_session_t* session = (wz_host_session_t*)context;
    char path[4096];
    wz_file_dialog_result_t dialog_result;
    wz_open_run_handlers_t handlers;
    wz_open_run_result_t open_result;
    wz_qword_t sleep_nanoseconds;
    (void)arguments;
    if (session == NULL || result == NULL) return WZ_RESULT_INVALID_ARGUMENT;
    wz_host_release_local_keys();
    if (!wz_input_focus_dialog_enter(&session->input_focus)) {
        result->reason = "file-dialog-focus-unavailable";
        return WZ_RESULT_INVALID_STATE;
    }
    dialog_result = wz_file_dialog_open(path, sizeof(path));
    (void)wz_input_focus_dialog_leave(&session->input_focus);
    if (session->pacing_initialized &&
        wz_host_pacing_set_speed(&session->pacing, session->speed)) {
        (void)wz_host_pacing_wait(&session->pacing, wz_host_now_nanoseconds(),
            session->machine.master_tick, NULL, NULL, &sleep_nanoseconds);
    }
    if (dialog_result == WZ_FILE_DIALOG_CANCELLED) {
        (void)snprintf(result->message, sizeof(result->message), "cancelled");
        (void)snprintf(session->file_notification,
                       sizeof(session->file_notification), "Open / Run cancelled");
        return WZ_RESULT_OK;
    }
    if (dialog_result != WZ_FILE_DIALOG_SELECTED) {
        result->reason = "file-dialog-failed";
        (void)snprintf(session->file_notification,
                       sizeof(session->file_notification), "File dialog failed");
        return WZ_RESULT_INVALID_STATE;
    }
    handlers.tape = wz_host_open_run_tape;
    handlers.snapshot = wz_host_open_run_snapshot;
    handlers.microdrive = wz_host_open_run_microdrive;
    handlers.conversion = NULL;
    handlers.context = session;
    open_result = wz_file_open_run_dispatch(path, &handlers);
    if (open_result != WZ_OPEN_RUN_OK) {
        result->reason = open_result == WZ_OPEN_RUN_UNSUPPORTED_FORMAT
            ? "unsupported-file-format" : "file-open-or-load-failed";
        (void)snprintf(session->file_notification,
            sizeof(session->file_notification), "%s",
            open_result == WZ_OPEN_RUN_HANDLER_UNAVAILABLE
                ? "This format is not available in the current workflow"
                : open_result == WZ_OPEN_RUN_UNSUPPORTED_FORMAT
                    ? "Unsupported file format"
                    : "The selected file could not be loaded");
        return WZ_RESULT_INVALID_STATE;
    }
    session->ui_window.layout.model_k =
        session->machine.profile != NULL &&
        session->machine.profile->kind == WZ_MACHINE_128K_PAL ? 128u : 48u;
    (void)snprintf(session->file_notification,
                   sizeof(session->file_notification), "File loaded");
    (void)snprintf(result->message, sizeof(result->message), "loaded");
    return WZ_RESULT_OK;
}

static wz_result_t wz_host_command_model_set(
    const void* context, wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    wz_host_session_t* session = (wz_host_session_t*)context;
    const wz_machine_profile_t* profile;
    const char* response;
    if (session == NULL || result == NULL || arguments.data == NULL) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (arguments.size == 3u &&
        memcmp(arguments.data, "48k", 3u) == 0) {
        profile = wz_machine_profile_48k_pal();
        response = "48k";
    } else if (arguments.size == 4u &&
               memcmp(arguments.data, "128k", 4u) == 0) {
        profile = wz_machine_profile_128k_pal();
        response = "128k";
    } else {
        result->reason = "bad-model";
        return WZ_RESULT_PARSE_ERROR;
    }
    if (profile->kind == WZ_MACHINE_128K_PAL &&
        session->machine.profile->kind == WZ_MACHINE_48K_PAL &&
        session->machine.has_48k_rom != 0u) {
        memcpy(session->model_48k_rom, session->machine.memory,
               sizeof(session->model_48k_rom));
        session->model_48k_rom_identity = session->machine.rom_identity;
        session->has_model_48k_rom = true;
    }
    if (wz_machine_reconfigure_profile(&session->machine, profile) !=
        WZ_RESULT_OK) {
        result->reason = "model-change-failed";
        return WZ_RESULT_INVALID_STATE;
    }
    if (profile->kind == WZ_MACHINE_48K_PAL &&
        session->has_model_48k_rom &&
        session->model_48k_rom_identity == profile->expected_rom_identity &&
        wz_machine_load_48k_rom(&session->machine, session->model_48k_rom,
                                sizeof(session->model_48k_rom)) != WZ_RESULT_OK) {
        result->reason = "model-rom-restore-failed";
        return WZ_RESULT_INVALID_STATE;
    }
    session->audio_sample_remainder = 0u;
    session->audio_sample_speed_initialized = false;
    (void)snprintf(result->message, sizeof(result->message), "%s", response);
    return WZ_RESULT_OK;
}

static wz_result_t wz_host_command_reset(
    const void* context, wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    wz_host_session_t* session = (wz_host_session_t*)context;
    (void)arguments;
    if (session == NULL || result == NULL) return WZ_RESULT_INVALID_ARGUMENT;
    if (wz_machine_reset(&session->machine) != WZ_RESULT_OK) {
        result->reason = "machine-reset-failed";
        return WZ_RESULT_INVALID_STATE;
    }
    if (session->pacing_initialized) {
        (void)wz_host_pacing_set_speed(&session->pacing, session->speed);
    }
    (void)snprintf(result->message, sizeof(result->message), "reset");
    return WZ_RESULT_OK;
}

static wz_result_t wz_host_command_pause(
    const void* context, wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    wz_host_session_t* session = (wz_host_session_t*)context;
    (void)arguments;
    if (session == NULL || result == NULL) return WZ_RESULT_INVALID_ARGUMENT;
    if (!session->ui_window.layout.paused) {
        session->ui_window.layout.paused = true;
        session->ui_window.layout.audio_muted = true;
    }
    (void)snprintf(result->message, sizeof(result->message), "paused");
    return WZ_RESULT_OK;
}

static wz_result_t wz_host_command_resume(
    const void* context, wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    wz_host_session_t* session = (wz_host_session_t*)context;
    (void)arguments;
    if (session == NULL || result == NULL) return WZ_RESULT_INVALID_ARGUMENT;
    if (session->ui_window.layout.paused && session->pacing_initialized &&
        !wz_host_pacing_set_speed(&session->pacing, session->speed)) {
        result->reason = "pacing-reanchor-failed";
        return WZ_RESULT_INVALID_STATE;
    }
    session->ui_window.layout.paused = false;
    session->ui_window.layout.audio_muted =
        !wz_host_audio_enabled(session->speed);
    (void)snprintf(result->message, sizeof(result->message), "running");
    return WZ_RESULT_OK;
}

static wz_result_t wz_host_command_pause_resume(
    const void* context, wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    const wz_host_session_t* session = (const wz_host_session_t*)context;
    const char* command_id;
    (void)arguments;
    if (session == NULL || result == NULL) return WZ_RESULT_INVALID_ARGUMENT;
    command_id = session->ui_window.layout.paused ? "machine.resume" :
        "machine.pause";
    return wz_command_registry_dispatch(
        &session->command_registry, command_id,
        (wz_command_arguments_t){NULL, 0u}, result);
}

static wz_result_t wz_host_command_screenshot(
    const void* context, wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    wz_host_session_t* session = (wz_host_session_t*)context;
    char path[1024];
    wz_telnet_screenshot_result_t screenshot_result;
    (void)arguments;
    if (session == NULL || result == NULL) return WZ_RESULT_INVALID_ARGUMENT;
    screenshot_result = wz_telnet_screenshot_save(&session->snapshot,
                                                  path, sizeof(path));
    if (screenshot_result != WZ_TELNET_SCREENSHOT_OK) {
        result->reason = "screenshot-failed";
        return WZ_RESULT_INVALID_STATE;
    }
    {
        int written = snprintf(result->message, sizeof(result->message),
                               "PATH=\"%s\"", path);
        if (written < 0 || (size_t)written >= sizeof(result->message)) {
            result->reason = "screenshot-path-too-long";
            return WZ_RESULT_BUFFER_TOO_SMALL;
        }
    }
    return WZ_RESULT_OK;
}

static wz_result_t wz_host_command_speed(
    const void* context, wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    wz_host_session_t* session = (wz_host_session_t*)context;
    char command[48];
    wz_telnet_speed_command_t speed;
    if (session == NULL || result == NULL || arguments.data == NULL ||
        arguments.size == 0u || arguments.size >= sizeof(command)) {
        if (result != NULL) result->reason = "bad-speed";
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    (void)snprintf(command, sizeof(command), "SPEED %.*s",
                   (int)arguments.size, (const char*)arguments.data);
    if (!wz_telnet_speed_parse(command, &speed) ||
        !wz_telnet_speed_apply(speed) ||
        !wz_speed_policy_valid((wz_speed_policy_t)speed) ||
        !wz_ui_layout_select_speed(&session->ui_window.layout,
                                   (wz_speed_policy_t)speed)) {
        result->reason = "bad-speed";
        return WZ_RESULT_PARSE_ERROR;
    }
    if (session->pacing_initialized &&
        !wz_host_pacing_set_speed(&session->pacing,
                                  (wz_speed_policy_t)speed)) {
        result->reason = "speed-pacing-failed";
        return WZ_RESULT_INVALID_STATE;
    }
    session->speed = (wz_speed_policy_t)speed;
    session->ui_window.layout.audio_muted =
        session->ui_window.layout.paused ||
        !wz_host_audio_enabled((wz_speed_policy_t)speed);
    (void)snprintf(result->message, sizeof(result->message), "%s",
                   wz_ui_layout_speed_label((size_t)speed));
    return WZ_RESULT_OK;
}

static bool wz_host_register_commands(void)
{
    static const wz_command_metadata_t commands[] = {
        {
            "file.open_run", "Open / Run...",
            "Choose a supported tape, snapshot, or Microdrive image",
            "file", "NONE", "wz-command-result",
            "wz_host_command_open_run", "native-file-dialog", NULL,
            WZ_COMMAND_LOCAL_ONLY, NULL, wz_host_command_open_run,
            &wz_host_session, true, false, NULL
        },
        {
            "machine.pause_resume", "Pause", "Pause or resume the emulated machine",
            "machine", "NONE", NULL, "wz_host_command_pause_resume", "local",
            NULL, WZ_COMMAND_REMOTE_SAFE, NULL, wz_host_command_pause_resume,
            &wz_host_session, true, true, NULL
        },
        {
            "machine.pause", "Pause", "Pause the emulated machine",
            NULL, "NONE", NULL, "wz_host_command_pause", "shared",
            NULL, WZ_COMMAND_REMOTE_SAFE, NULL,
            wz_host_command_pause, &wz_host_session, true, true, NULL
        },
        {
            "machine.resume", "Resume", "Resume the emulated machine",
            NULL, "NONE", NULL, "wz_host_command_resume", "shared",
            NULL, WZ_COMMAND_REMOTE_SAFE, NULL,
            wz_host_command_resume, &wz_host_session, true, true, NULL
        },
        {
            "machine.model.set", "Set model", "Switch between certified 48K and 128K profiles",
            "machine", "48K|128K", NULL, "wz_host_command_model_set", "telnet",
            NULL, WZ_COMMAND_REMOTE_SAFE, NULL, wz_host_command_model_set,
            &wz_host_session, true, true, NULL
        },
        {
            "machine.reset", "Reset", "Reset the emulated machine",
            "machine", "NONE", NULL, "wz_host_command_reset", "telnet",
            NULL, WZ_COMMAND_REMOTE_SAFE, NULL, wz_host_command_reset,
            &wz_host_session, true, true, NULL
        },
        {
            "machine.speed.set", "Set speed", "Set emulation speed",
            "machine", "PERCENT", "SPEED", "wz_host_command_speed", "telnet",
            NULL, WZ_COMMAND_REMOTE_SAFE, NULL, wz_host_command_speed,
            &wz_host_session, true, true, NULL
        },
        {
            "host.screenshot.temp", "Screenshot", "Save the current raster",
            "host", "NONE", "PATH", "wz_host_command_screenshot", "telnet",
            NULL, WZ_COMMAND_REMOTE_SAFE, NULL, wz_host_command_screenshot,
            &wz_host_session, false, true, NULL
        }
    };
    size_t index;
    if (wz_command_registry_bind_owner_thread(
            &wz_host_session.command_registry) != WZ_RESULT_OK) {
        return false;
    }
    for (index = 0u; index < sizeof(commands) / sizeof(commands[0]); ++index) {
        if (wz_command_registry_register(&wz_host_session.command_registry,
                                         commands[index]) != WZ_RESULT_OK) {
            return false;
        }
    }
    wz_host_session.networking_command_context.machine =
        &wz_host_session.machine;
    wz_host_session.networking_command_context.flush_callback = NULL;
    wz_host_session.networking_command_context.flush_context = NULL;
    wz_host_session.networking_command_context.discard_dirty_media = false;
    if (wz_networking_commands_register(
            &wz_host_session.command_registry,
            &wz_host_session.networking_command_context) != WZ_RESULT_OK) {
        return false;
    }
    wz_host_session.tape_loading_command_context.machine =
        &wz_host_session.machine;
    if (wz_tape_loading_commands_register(
            &wz_host_session.command_registry,
            &wz_host_session.tape_loading_command_context) != WZ_RESULT_OK) {
        return false;
    }
    wz_host_session.tape_media_command_context.machine =
        &wz_host_session.machine;
    wz_host_session.tape_media_command_context.load =
        wz_host_tape_media_load;
    wz_host_session.tape_media_command_context.release =
        wz_host_tape_media_release;
    wz_host_session.tape_media_command_context.open_manager =
        wz_host_tape_manager_open;
    wz_host_session.tape_media_command_context.context = &wz_host_session;
    if (wz_tape_media_commands_register(
            &wz_host_session.command_registry,
            &wz_host_session.tape_media_command_context) != WZ_RESULT_OK) {
        return false;
    }
    return wz_command_registry_finalize(&wz_host_session.command_registry) ==
        WZ_RESULT_OK;
}

static void wz_host_telnet_send(const char* output, size_t length)
{
    if (wz_telnet_client_is_active(&wz_host_session.telnet_client) &&
        output != NULL && length != 0u) {
        (void)wz_host_socket_send(wz_host_session.telnet_client.active_client,
                                  output, length);
    }
}

static void wz_host_telnet_process_command(const char* command)
{
    char output[WZ_HOST_TELNET_IO_CAPACITY];
    char projected[WZ_HOST_TELNET_IO_CAPACITY];
    char id[WZ_HOST_TELNET_IO_CAPACITY];
    char arguments[WZ_HOST_TELNET_IO_CAPACITY];
    char value[WZ_HOST_TELNET_IO_CAPACITY];
    size_t length = 0u;
    size_t physical_key = 0u;
    bool key_ok = false;
    if (command == NULL) return;
    if (wz_telnet_alias_to_do(command, projected, sizeof(projected)) ||
        wz_telnet_model_alias_to_do(command, projected, sizeof(projected))) {
        char dispatch_response[WZ_HOST_TELNET_IO_CAPACITY];
        size_t dispatch_length = 0u;
        if (wz_telnet_do_parse(projected, id, sizeof(id), arguments,
                               sizeof(arguments)) &&
            wz_telnet_do_format(&wz_host_session.command_registry, id,
                                arguments, dispatch_response,
                                sizeof(dispatch_response), &dispatch_length)) {
            size_t alias_length = 0u;
            if (!wz_telnet_alias_response_rewrite(
                    command, dispatch_response, output, sizeof(output),
                    &alias_length)) {
                if (dispatch_length >= sizeof(output)) {
                    output[0] = '\0';
                    alias_length = 0u;
                } else {
                    memcpy(output, dispatch_response, dispatch_length + 1u);
                    alias_length = dispatch_length;
                }
            }
            wz_host_telnet_send(output, alias_length);
            return;
        }
        wz_host_telnet_process_command(projected);
        return;
    } else if (strcmp(command, "HELP") == 0) {
        (void)wz_telnet_help_format(output, sizeof(output), &length);
    } else if (wz_telnet_menu_tree_parse(command)) {
        (void)wz_telnet_menu_tree_format(&wz_host_session.command_registry,
                                         output, sizeof(output), &length);
    } else if (wz_telnet_menu_find_parse(command, value, sizeof(value))) {
        (void)wz_telnet_menu_find_format(&wz_host_session.command_registry,
                                         value, output, sizeof(output), &length);
    } else if (wz_telnet_menu_parse(command)) {
        (void)wz_telnet_menu_format_root(output, sizeof(output), &length);
    } else if (wz_telnet_menu_id_parse(command, id, sizeof(id))) {
        (void)wz_telnet_menu_id_format(&wz_host_session.command_registry, id,
                                       output, sizeof(output), &length);
    } else if (wz_telnet_describe_parse(command, id, sizeof(id))) {
        (void)wz_telnet_describe_format(&wz_host_session.command_registry, id,
                                        output, sizeof(output), &length);
    } else if (wz_telnet_do_parse(command, id, sizeof(id), arguments,
                                  sizeof(arguments))) {
        (void)wz_telnet_do_format(&wz_host_session.command_registry, id,
                                   arguments, output, sizeof(output), &length);
    } else if (strcmp(command, "STATUS") == 0) {
        wz_telnet_status_snapshot_t status = {
            .control_port = wz_host_session.control_port.selected_port,
            .ipv4_up = wz_host_session.control_port.ipv4_active,
            .ipv6_up = wz_host_session.control_port.ipv6_active,
            .client_active = wz_telnet_client_is_active(
                &wz_host_session.telnet_client)
        };
        if (wz_telnet_status_project_machine(
                &status, wz_host_session.machine.profile,
                wz_machine_networking_mode(&wz_host_session.machine),
                wz_host_session.ui_window.layout.paused,
                wz_host_session.speed,
                wz_sokol_audio_valid(&wz_host_session.audio),
                wz_host_session.ui_window.layout.audio_muted,
                wz_sokol_audio_degraded(&wz_host_session.audio))) {
            (void)wz_telnet_status_format(&status, output, sizeof(output),
                                          &length);
        } else {
            (void)wz_telnet_error_format(WZ_TELNET_ERROR_BAD_COMMAND,
                                          output, sizeof(output), &length);
        }
    } else if (wz_telnet_keyboard_command_key_down(command, &physical_key)) {
        wz_telnet_key_press_cancel(&wz_host_session.telnet_key_presses,
                                   physical_key);
        key_ok = wz_telnet_input_set_key(&wz_host_session.input_arbiter,
                                         physical_key, true);
        (void)wz_telnet_keyboard_command_format_response(
            key_ok ? WZ_TELNET_KEYBOARD_RESPONSE_OK : WZ_TELNET_KEYBOARD_RESPONSE_BAD_STATE,
            output, sizeof(output), &length);
    } else if (wz_telnet_keyboard_command_key_up(command, &physical_key)) {
        wz_telnet_key_press_cancel(&wz_host_session.telnet_key_presses,
                                   physical_key);
        key_ok = wz_telnet_input_set_key(&wz_host_session.input_arbiter,
                                         physical_key, false);
        (void)wz_telnet_keyboard_command_format_response(
            key_ok ? WZ_TELNET_KEYBOARD_RESPONSE_OK : WZ_TELNET_KEYBOARD_RESPONSE_BAD_STATE,
            output, sizeof(output), &length);
    } else if (wz_telnet_keyboard_command_key_press(command, &physical_key)) {
        const wz_machine_profile_t* profile =
            wz_host_session.machine.profile;
        if (profile == NULL) {
            key_ok = false;
        } else {
            wz_qword_t frame_ticks =
                (wz_qword_t)profile->tstates_per_frame *
                (wz_qword_t)profile->master_ticks_per_cpu_tstate;
            key_ok = wz_telnet_key_press_schedule(
                &wz_host_session.telnet_key_presses,
                &wz_host_session.input_arbiter, physical_key,
                wz_host_session.machine.master_tick, frame_ticks);
        }
        (void)wz_telnet_keyboard_command_format_response(
            key_ok ? WZ_TELNET_KEYBOARD_RESPONSE_OK : WZ_TELNET_KEYBOARD_RESPONSE_BAD_STATE,
            output, sizeof(output), &length);
    } else if (wz_telnet_keyboard_command_release_all(command)) {
        wz_telnet_key_press_cancel_all(&wz_host_session.telnet_key_presses);
        key_ok = wz_telnet_input_release_all(&wz_host_session.input_arbiter);
        (void)wz_telnet_keyboard_command_format_response(
            key_ok ? WZ_TELNET_KEYBOARD_RESPONSE_OK : WZ_TELNET_KEYBOARD_RESPONSE_BAD_STATE,
            output, sizeof(output), &length);
    } else if (wz_telnet_screenshot_parse(command)) {
        char path[1024];
        wz_telnet_screenshot_result_t result = wz_telnet_screenshot_save(
            &wz_host_session.snapshot, path, sizeof(path));
        (void)wz_telnet_screenshot_format_response(result, path, output,
                                                   sizeof(output), &length);
    } else {
        (void)wz_telnet_error_format(WZ_TELNET_ERROR_BAD_COMMAND,
                                      output, sizeof(output), &length);
    }
    wz_host_telnet_send(output, length);
}

static void wz_host_telnet_poll(void)
{
    int received;
    size_t application_length = 0u;
    size_t protocol_length = 0u;
    size_t command_length = 0u;
    bool malformed = false;
    wz_telnet_command_error_t error = WZ_TELNET_COMMAND_ERROR_NONE;
    if (!wz_telnet_client_is_active(&wz_host_session.telnet_client)) return;
    received = wz_host_socket_receive(wz_host_session.telnet_client.active_client,
                                      wz_host_session.telnet_input,
                                      sizeof(wz_host_session.telnet_input));
    if (received == 0 || (received < 0 && !wz_host_socket_would_block())) {
        wz_telnet_client_disconnect(&wz_host_session.telnet_client);
        wz_telnet_key_press_cancel_all(&wz_host_session.telnet_key_presses);
        return;
    }
    if (received < 0) return;
    if (!wz_telnet_negotiator_feed(&wz_host_session.telnet_negotiator,
                                   wz_host_session.telnet_input,
                                   (size_t)received,
                                   wz_host_session.telnet_application,
                                   sizeof(wz_host_session.telnet_application),
                                   &application_length,
                                   wz_host_session.telnet_protocol,
                                   sizeof(wz_host_session.telnet_protocol),
                                   &protocol_length)) return;
    wz_host_telnet_send((const char*)wz_host_session.telnet_protocol,
                        protocol_length);
    if (!wz_telnet_command_buffer_feed(&wz_host_session.telnet_commands,
                                       wz_host_session.telnet_application,
                                       application_length,
                                       wz_host_session.telnet_command,
                                       sizeof(wz_host_session.telnet_command),
                                       &command_length, &malformed, &error)) return;
    if (malformed) {
        const char* response = error == WZ_TELNET_COMMAND_ERROR_LINE_TOO_LONG
            ? "ERR LINE_TOO_LONG\r\n" : "ERR BAD_COMMAND\r\n";
        wz_host_telnet_send(response, strlen(response));
    } else if (command_length != 0u) {
        wz_host_session.telnet_command[command_length] = '\0';
        wz_host_telnet_process_command((const char*)wz_host_session.telnet_command);
    }
}

static wz_result_t wz_host_ui_set_speed(wz_speed_policy_t speed)
{
    wz_command_result_t result;
    return wz_ui_layout_activate_speed(
        &wz_host_session.command_registry, speed, &result);
}

static wz_result_t wz_host_ui_set_model(const char* model)
{
    wz_command_result_t result;
    return wz_ui_layout_activate_model(
        &wz_host_session.command_registry, model, &result);
}

static void wz_host_ui_draw_speed_items(struct nk_context* context)
{
    size_t index;
    for (index = 0u; index < WZ_SPEED_COUNT; ++index) {
        const char* label = wz_ui_layout_speed_label(index);
        if (label != NULL && nk_menu_item_label(context, label, NK_TEXT_LEFT)) {
            (void)wz_host_ui_set_speed((wz_speed_policy_t)index);
        }
    }
}

static void wz_host_ui_draw_menus(struct nk_context* context, float width)
{
    size_t root_index;
    size_t root_count = wz_command_registry_menu_root_count();
    struct nk_vec2 saved_padding = context->style.window.padding;
    context->style.window.padding = nk_vec2(0.0f, 0.0f);
    if (!nk_begin(context, "Application menu",
            nk_rect(0.0f, 0.0f, width, WZ_UI_MENU_BAR_HEIGHT),
            NK_WINDOW_NO_SCROLLBAR)) {
        nk_end(context);
        context->style.window.padding = saved_padding;
        return;
    }
    nk_menubar_begin(context);
    nk_layout_row_begin(context, NK_DYNAMIC, WZ_UI_MENU_BAR_HEIGHT - 2.0f,
                        (int)root_count);
    for (root_index = 0u; root_index < root_count; ++root_index) {
        const wz_command_menu_root_t* root =
            wz_command_registry_menu_root_at(root_index);
        size_t command_index;
        nk_layout_row_push(context, root_count == 0u ? 0.0f :
                           1.0f / (float)root_count);
        if (root == NULL || !nk_menu_begin_label(
                context, root->label, NK_TEXT_LEFT, nk_vec2(230.0f, 300.0f))) {
            continue;
        }
        nk_layout_row_dynamic(context, 24.0f, 1);
        for (command_index = 0u;
             command_index < wz_command_registry_count(
                 &wz_host_session.command_registry);
             ++command_index) {
            const wz_command_metadata_t* command =
                wz_command_registry_at(&wz_host_session.command_registry,
                                       command_index);
            const char* disabled_reason = NULL;
            bool enabled;
            if (command == NULL || command->menu_group == NULL ||
                strcmp(command->menu_group, root->id) != 0) {
                continue;
            }
            enabled = wz_command_registry_state(
                &wz_host_session.command_registry, command->id,
                &disabled_reason) == WZ_COMMAND_ENABLED;
            if (strcmp(command->id, WZ_TAPE_INSERT_COMMAND_ID) == 0) {
                if (!enabled) nk_widget_disable_begin(context);
                if (nk_menu_item_label(context, command->label, NK_TEXT_LEFT) &&
                    enabled) {
                    wz_host_ui_insert_tape();
                }
                if (!enabled) nk_widget_disable_end(context);
            } else if (strcmp(command->id, "machine.speed.set") == 0) {
                if (!enabled) nk_widget_disable_begin(context);
                if (nk_menu_begin_label(context, "Emulation Speed",
                                        NK_TEXT_LEFT,
                                        nk_vec2(160.0f, 7.0f * 25.0f))) {
                    nk_layout_row_dynamic(context, 24.0f, 1);
                    wz_host_ui_draw_speed_items(context);
                    nk_menu_end(context);
                }
                if (!enabled) nk_widget_disable_end(context);
            } else if (strcmp(command->id, "machine.model.set") == 0) {
                if (!enabled) nk_widget_disable_begin(context);
                if (nk_menu_begin_label(context, "Model", NK_TEXT_LEFT,
                                        nk_vec2(160.0f, 2.0f * 25.0f))) {
                    nk_layout_row_dynamic(context, 24.0f, 1);
                    if (nk_menu_item_label(context, "48K", NK_TEXT_LEFT)) {
                        (void)wz_host_ui_set_model("48k");
                    }
                    if (nk_menu_item_label(context, "128K", NK_TEXT_LEFT)) {
                        (void)wz_host_ui_set_model("128k");
                    }
                    nk_menu_end(context);
                }
                if (!enabled) nk_widget_disable_end(context);
            } else if (command->parameter_schema != NULL &&
                       strcmp(command->parameter_schema, "NONE") != 0) {
                nk_widget_disable_begin(context);
                (void)nk_menu_item_label(context, command->label, NK_TEXT_LEFT);
                nk_widget_disable_end(context);
            } else {
                if (!enabled) nk_widget_disable_begin(context);
                {
                    const char* label = command->label;
                    if (strcmp(command->id, "machine.pause_resume") == 0) {
                        label = wz_host_session.ui_window.layout.paused ?
                            "Resume" : "Pause";
                    }
                    if (nk_menu_item_label(context, label, NK_TEXT_LEFT)) {
                        wz_command_result_t result;
                        (void)wz_command_registry_dispatch(
                            &wz_host_session.command_registry, command->id,
                            (wz_command_arguments_t){NULL, 0u}, &result);
                    }
                }
                if (!enabled) nk_widget_disable_end(context);
            }
        }
        nk_menu_end(context);
    }
    nk_layout_row_end(context);
    nk_menubar_end(context);
    nk_end(context);
    context->style.window.padding = saved_padding;
}

static void wz_host_ui_draw_toolbar(struct nk_context* context, float width)
{
    size_t index;
    const size_t item_count = wz_ui_layout_toolbar_count();
    struct nk_vec2 saved_padding = context->style.window.padding;
    context->style.window.padding = nk_vec2(0.0f, 0.0f);
    if (!nk_begin(context, "Host controls",
                  nk_rect(0.0f, WZ_UI_MENU_BAR_HEIGHT, width,
                          WZ_UI_TOOLBAR_HEIGHT),
                  NK_WINDOW_NO_SCROLLBAR)) {
        nk_end(context);
        context->style.window.padding = saved_padding;
        return;
    }
    nk_layout_row_dynamic(context, WZ_UI_TOOLBAR_HEIGHT - 4.0f,
                          (int)item_count);
    for (index = 0u; index < item_count; ++index) {
        const wz_ui_toolbar_item_t* item = wz_ui_layout_toolbar_at(index);
        bool enabled = false;
        if (item != NULL && strcmp(item->command_id, "machine.speed") == 0) {
            const char* speed_label = wz_ui_layout_speed_label(
                (size_t)wz_host_session.speed);
            const wz_command_metadata_t* speed_command =
                wz_command_registry_find(&wz_host_session.command_registry,
                                         "machine.speed.set");
            enabled = speed_command != NULL && wz_command_registry_state(
                &wz_host_session.command_registry, speed_command->id, NULL) ==
                WZ_COMMAND_ENABLED;
            if (!enabled) nk_widget_disable_begin(context);
            if (nk_combo_begin_label(context,
                    speed_label == NULL ? "Speed" : speed_label,
                    nk_vec2(145.0f, 7.0f * 25.0f))) {
                nk_layout_row_dynamic(context, 24.0f, 1);
                for (size_t speed_index = 0u;
                     speed_index < WZ_SPEED_COUNT; ++speed_index) {
                    const char* label = wz_ui_layout_speed_label(speed_index);
                    if (label != NULL && nk_combo_item_label(
                            context, label, NK_TEXT_LEFT)) {
                        (void)wz_host_ui_set_speed(
                            (wz_speed_policy_t)speed_index);
                    }
                }
                nk_combo_end(context);
            }
            if (!enabled) nk_widget_disable_end(context);
        } else if (item != NULL &&
                   strcmp(item->command_id, "media.tape") == 0) {
            const wz_tape_loading_mode_t mode =
                wz_machine_tape_loading_mode(&wz_host_session.machine);
            char mode_label[48];
            const char* mode_name = mode == WZ_TAPE_LOADING_INSTANT_TRAP ?
                "Instant" : "Normal";
            (void)snprintf(mode_label, sizeof(mode_label), "%s / %s",
                wz_host_session.machine.tape_mounted != 0u ? "Tape" : "Empty",
                mode_name);
            const wz_command_metadata_t* normal_command =
                wz_command_registry_find(&wz_host_session.command_registry,
                    WZ_TAPE_LOADING_NORMAL_COMMAND_ID);
            const bool tape_enabled = normal_command != NULL &&
                wz_command_registry_state(&wz_host_session.command_registry,
                    normal_command->id, NULL) == WZ_COMMAND_ENABLED;
            if (!tape_enabled) nk_widget_disable_begin(context);
            if (nk_combo_begin_label(context, mode_label,
                                     nk_vec2(180.0f, 5.0f * 25.0f))) {
                nk_layout_row_dynamic(context, 24.0f, 1);
                for (size_t action_index = 0u;
                     action_index < wz_ui_layout_tape_action_count();
                     ++action_index) {
                    const wz_ui_toolbar_item_t* action =
                        wz_ui_layout_tape_action_at(action_index);
                    const wz_command_metadata_t* action_command =
                        action == NULL ? NULL : wz_command_registry_find(
                            &wz_host_session.command_registry,
                            action->command_id);
                    const char* action_label;
                    char selected_label[48];
                    bool action_enabled;
                    if (action == NULL) continue;
                    action_label = action->label;
                    if ((strcmp(action->command_id,
                                WZ_TAPE_LOADING_NORMAL_COMMAND_ID) == 0 &&
                         mode == WZ_TAPE_LOADING_NORMAL) ||
                        (strcmp(action->command_id,
                                WZ_TAPE_LOADING_INSTANT_COMMAND_ID) == 0 &&
                         mode == WZ_TAPE_LOADING_INSTANT_TRAP)) {
                        (void)snprintf(selected_label, sizeof(selected_label),
                                       "%s [selected]", action->label);
                        action_label = selected_label;
                    }
                    action_enabled = action_command != NULL &&
                        wz_command_registry_state(
                            &wz_host_session.command_registry,
                            action_command->id, NULL) == WZ_COMMAND_ENABLED &&
                        action_command->parameter_schema != NULL &&
                        (strcmp(action_command->parameter_schema, "NONE") == 0 ||
                         strcmp(action_command->id,
                                WZ_TAPE_INSERT_COMMAND_ID) == 0);
                    if (!action_enabled) {
                        nk_widget_disable_begin(context);
                        (void)nk_combo_item_label(context, action_label,
                                                  NK_TEXT_LEFT);
                        nk_widget_disable_end(context);
                    } else if (nk_combo_item_label(context, action_label,
                                                   NK_TEXT_LEFT)) {
                        if (strcmp(action_command->id,
                                   WZ_TAPE_INSERT_COMMAND_ID) == 0) {
                            wz_host_ui_insert_tape();
                        } else {
                            wz_command_result_t result;
                            (void)wz_command_registry_dispatch(
                                &wz_host_session.command_registry,
                                action_command->id,
                                (wz_command_arguments_t){NULL, 0u}, &result);
                        }
                    }
                }
                nk_combo_end(context);
            }
            if (!tape_enabled) nk_widget_disable_end(context);
        } else {
            const wz_command_metadata_t* command = item == NULL ? NULL :
                wz_command_registry_find(&wz_host_session.command_registry,
                                         item->command_id);
            enabled = command != NULL && command->parameter_schema != NULL &&
                strcmp(command->parameter_schema, "NONE") == 0 &&
                wz_command_registry_state(&wz_host_session.command_registry,
                                          command->id, NULL) == WZ_COMMAND_ENABLED;
            if (!enabled) nk_widget_disable_begin(context);
            if (item != NULL && nk_button_label(context,
                    strcmp(item->command_id, "machine.pause_resume") == 0 ?
                        (wz_host_session.ui_window.layout.paused ?
                            "Resume" : "Pause") : item->label) && enabled) {
                wz_command_result_t result;
                (void)wz_command_registry_dispatch(
                    &wz_host_session.command_registry, command->id,
                    (wz_command_arguments_t){NULL, 0u}, &result);
            }
            if (!enabled) nk_widget_disable_end(context);
        }
    }
    nk_end(context);
    context->style.window.padding = saved_padding;
}

static void wz_host_ui_draw_tape_manager(struct nk_context* context,
                                         float width)
{
    char line[192];
    const wz_machine_t* machine = &wz_host_session.machine;
    size_t first_segment;
    size_t end_segment;
    size_t current_segment;
    size_t block_count;
    float panel_width;
    if (!wz_host_session.tape_manager_open) return;
    panel_width = width >= 480.0f ? 460.0f : width - 16.0f;
    if (panel_width < 180.0f) return;
    if (!nk_begin(context, "Tape Manager",
            nk_rect(width - panel_width - 8.0f,
                    WZ_UI_MENU_BAR_HEIGHT + WZ_UI_TOOLBAR_HEIGHT + 8.0f,
                    panel_width, 380.0f),
            NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE |
                NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE)) {
        nk_end(context);
        return;
    }
    nk_layout_row_dynamic(context, 24.0f, 1);
    if (nk_button_label(context, "Close Tape Manager")) {
        wz_host_session.tape_manager_open = false;
        nk_end(context);
        return;
    }
    if (machine->tape_mounted == 0u) {
        nk_label(context, "Transport: Empty", NK_TEXT_LEFT);
        nk_end(context);
        return;
    }
    if (wz_host_session.tape_segments == NULL ||
        wz_host_session.tape_segment_count == 0u) {
        nk_label(context, "Mounted tape has no readable signal segments",
                 NK_TEXT_LEFT);
        nk_end(context);
        return;
    }
    nk_label(context, "Source:", NK_TEXT_LEFT);
    nk_label_wrap(context,
        wz_host_session.tape_source_path[0] == '\0' ? "unknown" :
            wz_host_session.tape_source_path);
    (void)snprintf(line, sizeof(line), "Format: %s | Loading mode: %s",
        wz_host_session.tape_format[0] == '\0' ? "unknown" :
            wz_host_session.tape_format,
        wz_machine_tape_loading_mode(machine) == WZ_TAPE_LOADING_NORMAL ?
            "Normal" : "Instant / Trap");
    nk_label(context, line, NK_TEXT_LEFT);
    current_segment = machine->tape_state.segment_index;
    if (current_segment >= wz_host_session.tape_segment_count) {
        current_segment = wz_host_session.tape_segment_count - 1u;
    }
    (void)snprintf(line, sizeof(line),
        "Transport: %s | Signal segment %llu / %llu",
        machine->tape_state.motor_on ? "Playing" :
            machine->tape_state.at_end ? "At end" : "Stopped",
        (unsigned long long)(current_segment + 1u),
        (unsigned long long)wz_host_session.tape_segment_count);
    nk_label(context, line, NK_TEXT_LEFT);
    block_count = strcmp(wz_host_session.tape_format, "TAP") == 0 ?
        wz_host_session.tape_block_count :
        strcmp(wz_host_session.tape_format, "TZX") == 0 ?
            wz_host_session.tzx_block_count : 0u;
    if (block_count != 0u) {
        size_t selected = wz_host_session.tape_manager_selected_block;
        first_segment = wz_host_session.tape_manager_block_page_start;
        if (first_segment >= block_count) {
            first_segment = ((block_count - 1u) / 9u) * 9u;
            wz_host_session.tape_manager_block_page_start = first_segment;
        }
        end_segment = first_segment + 9u;
        if (end_segment > block_count) end_segment = block_count;
        nk_label(context, "Tape blocks (read only)", NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 24.0f, 2);
        if (nk_button_label(context, "Previous blocks") && first_segment > 0u) {
            first_segment = first_segment >= 9u ? first_segment - 9u : 0u;
            wz_host_session.tape_manager_block_page_start = first_segment;
            if (selected < first_segment || selected >= first_segment + 9u) {
                selected = first_segment;
                wz_host_session.tape_manager_selected_block = selected;
            }
        }
        if (nk_button_label(context, "Next blocks") && end_segment < block_count) {
            first_segment = end_segment;
            wz_host_session.tape_manager_block_page_start = first_segment;
            selected = first_segment;
            wz_host_session.tape_manager_selected_block = selected;
        }
        end_segment = first_segment + 9u;
        if (end_segment > block_count) end_segment = block_count;
        nk_layout_row_dynamic(context, 24.0f, 1);
        for (size_t index = first_segment; index < end_segment; ++index) {
            if (strcmp(wz_host_session.tape_format, "TAP") == 0) {
                const wz_tap_block_t* block = &wz_host_session.tape_blocks[index];
                const char* type = "Data";
                char name[11] = {0};
                if (block->data == NULL || block->length == 0u) {
                    (void)snprintf(line, sizeof(line),
                        "%sBlock %llu | invalid TAP block metadata",
                        index == selected ? "> " : "  ",
                        (unsigned long long)(index + 1u));
                    nk_label(context, line, NK_TEXT_LEFT);
                    continue;
                }
                bool has_header = block->length == 19u && block->data[0] == 0u;
                if (has_header) {
                    switch (block->data[1]) {
                    case 0u: type = "Program header"; break;
                    case 1u: type = "Number array header"; break;
                    case 2u: type = "Character array header"; break;
                    case 3u: type = "Code header"; break;
                    default: type = "Unknown header"; break;
                    }
                    for (size_t character = 0u; character < 10u; ++character) {
                        unsigned char value = block->data[2u + character];
                        name[character] = value >= 0x20u && value <= 0x7eu ?
                            (char)value : '.';
                    }
                    for (size_t character = 10u; character > 0u; --character) {
                        if (name[character - 1u] != ' ') break;
                        name[character - 1u] = '\0';
                    }
                }
                if (has_header) {
                    (void)snprintf(line, sizeof(line),
                        "%sBlock %llu | %s \"%.10s\" | flag %02X | data %u bytes",
                        index == selected ? "> " : "  ",
                        (unsigned long long)(index + 1u), type, name,
                        (unsigned int)block->data[0],
                        (unsigned int)wz_read_le16(block->data + 12u));
                } else {
                    (void)snprintf(line, sizeof(line),
                        "%sBlock %llu | %s | flag %02X | logical %llu / stored %llu bytes",
                        index == selected ? "> " : "  ",
                        (unsigned long long)(index + 1u), type,
                        (unsigned int)block->data[0],
                        (unsigned long long)(block->length > 2u ?
                            block->length - 2u : 0u),
                        (unsigned long long)(block->length + 2u));
                }
            } else {
                const wz_tzx_block_t* block = &wz_host_session.tzx_blocks[index];
                const char* disposition = block->disposition == WZ_TZX_SUPPORTED ?
                    "supported" : block->disposition == WZ_TZX_IGNORED ?
                        "ignored" : "unsupported";
                const char* type = "TZX block";
                const char* length_kind = "body";
                size_t logical_length = block->data_length;
                switch (block->block_id) {
                case 0x10u: type = "Standard data"; break;
                case 0x11u: type = "Turbo data"; break;
                case 0x12u: type = "Pure tone"; break;
                case 0x13u: type = "Pulse sequence"; break;
                case 0x14u: type = "Pure data"; break;
                case 0x15u: type = "Direct recording"; break;
                case 0x18u: type = "CSW recording"; break;
                case 0x19u: type = "Generalized data"; break;
                case 0x20u: type = "Pause / stop"; break;
                case 0x21u: type = "Group start"; break;
                case 0x22u: type = "Group end"; break;
                case 0x23u: type = "Jump"; break;
                case 0x24u: type = "Loop start"; break;
                case 0x25u: type = "Loop end"; break;
                case 0x26u: type = "Call sequence"; break;
                case 0x27u: type = "Return"; break;
                case 0x28u: type = "Select"; break;
                case 0x2au: type = "Stop if 48K"; break;
                case 0x2bu: type = "Signal level"; break;
                case 0x30u: type = "Text"; break;
                case 0x31u: type = "Message"; break;
                case 0x32u: type = "Archive info"; break;
                case 0x33u: type = "Hardware type"; break;
                case 0x35u: type = "Custom info"; break;
                case 0x5au: type = "Glue"; break;
                default: break;
                }
                if (block->data == NULL || block->data_length == 0u) {
                    (void)snprintf(line, sizeof(line),
                        "%sBlock %llu | invalid TZX block metadata",
                        index == selected ? "> " : "  ",
                        (unsigned long long)(index + 1u));
                } else {
                    if (block->block_id == 0x10u && block->data_length >= 4u) {
                        size_t encoded_length =
                            (size_t)wz_read_le16(block->data + 2u);
                        if (encoded_length <= block->data_length - 4u) {
                            logical_length = encoded_length > 2u ?
                                encoded_length - 2u : 0u;
                            length_kind = "logical";
                        }
                    } else if (block->block_id == 0x11u &&
                               block->data_length >= 18u) {
                        size_t encoded_length =
                            wz_host_read_le24(block->data + 15u);
                        if (encoded_length <= block->data_length - 18u) {
                            logical_length = encoded_length > 2u ?
                                encoded_length - 2u : 0u;
                            length_kind = "logical";
                        }
                    }
                    (void)snprintf(line, sizeof(line),
                        "%sBlock %llu | %s (%02X) | %s | %s %llu / stored %llu bytes",
                        index == selected ? "> " : "  ",
                        (unsigned long long)(index + 1u), type,
                        (unsigned int)block->block_id, disposition,
                        length_kind, (unsigned long long)logical_length,
                        (unsigned long long)block->block_length);
                }
            }
            if (nk_button_label(context, line)) {
                wz_host_session.tape_manager_selected_block = index;
            }
        }
    } else {
        nk_label(context, "Transport signal segments (read only)", NK_TEXT_LEFT);
        first_segment = wz_host_session.tape_manager_selected_segment > 4u ?
            wz_host_session.tape_manager_selected_segment - 4u : 0u;
        end_segment = first_segment + 9u;
        if (end_segment > wz_host_session.tape_segment_count) {
            end_segment = wz_host_session.tape_segment_count;
        }
        nk_layout_row_dynamic(context, 24.0f, 2);
        if (nk_button_label(context, "Previous segments") &&
            wz_host_session.tape_manager_selected_segment > 0u) {
            wz_host_session.tape_manager_selected_segment =
                wz_host_session.tape_manager_selected_segment > 9u ?
                    wz_host_session.tape_manager_selected_segment - 9u : 0u;
        }
        if (nk_button_label(context, "Next segments") &&
            end_segment < wz_host_session.tape_segment_count) {
            wz_host_session.tape_manager_selected_segment = end_segment;
        }
        nk_layout_row_dynamic(context, 24.0f, 1);
        for (size_t index = first_segment; index < end_segment; ++index) {
            const wz_tape_segment_t* segment =
                &wz_host_session.tape_segments[index];
            (void)snprintf(line, sizeof(line), "%sSegment %llu | EAR %s | %llu master ticks",
                index == wz_host_session.tape_manager_selected_segment ? "> " : "  ",
                (unsigned long long)(index + 1u),
                segment->ear_level != 0u ? "high" : "low",
                (unsigned long long)segment->duration);
            if (nk_button_label(context, line)) {
                wz_host_session.tape_manager_selected_segment = index;
            }
        }
    }
    nk_end(context);
}

static void wz_host_ui_draw_status(struct nk_context* context,
                                  float width, float height)
{
    const wz_ui_layout_state_t* state =
        wz_ui_window_layout(&wz_host_session.ui_window);
    const wz_ui_remote_control_status_t* remote_status =
        wz_ui_window_remote_control(&wz_host_session.ui_window);
    char status[WZ_UI_STATUS_CAPACITY];
    char control_status[WZ_UI_STATUS_CAPACITY];
    struct nk_vec2 saved_padding;
    const char* model = wz_host_session.machine.profile == NULL ?
        "Unavailable" : wz_host_session.machine.profile->name;
    if (state == NULL || remote_status == NULL) return;
    wz_ui_remote_control_indicator(
        remote_status, control_status, sizeof(control_status));
    (void)snprintf(status, sizeof(status),
        "%s | %s | %s | Keyboard %s | Audio %s | Tape %s | MDV1 %s | Net %s%s%s",
        model,
        wz_ui_layout_speed_label((size_t)wz_host_session.speed),
        state->paused ? "Paused" : "Running",
        !wz_input_focus_is_focused(&wz_host_session.input_focus) ?
            "Inactive" :
            wz_input_focus_forwards_viewport_keys(&wz_host_session.input_focus) ?
                "Spectrum" : "UI",
        state->audio_muted && state->audio_degraded ? "Muted/Degraded" :
            state->audio_muted ? "Muted" :
            state->audio_degraded ? "Degraded" : "On",
        wz_host_session.machine.tape_mounted == 0u ? "Empty" :
            wz_host_session.machine.tape_state.at_end ? "At end" :
            wz_host_session.machine.tape_state.motor_on ? "Playing" : "Stopped",
        state->microdrive1_mounted ? "Mounted" : "Empty",
        state->networking_mode == NULL ? "Unavailable" : state->networking_mode,
        wz_host_session.file_notification[0] == '\0' ? "" : " | ",
        wz_host_session.file_notification);
    saved_padding = context->style.window.padding;
    context->style.window.padding = nk_vec2(0.0f, 0.0f);
    if (nk_begin(context, "Machine status",
            nk_rect(0.0f, height - 48.0f, width, 48.0f),
            NK_WINDOW_NO_SCROLLBAR)) {
        nk_layout_row_dynamic(context, 20.0f, 1);
        nk_label(context, status, NK_TEXT_LEFT);
        nk_layout_row_dynamic(context, 20.0f, 1);
        if (nk_button_label(context, control_status)) {
            wz_host_session.remote_settings_visible = true;
        }
    }
    nk_end(context);
    context->style.window.padding = saved_padding;
}

static void wz_host_ui_draw_remote_settings(struct nk_context* context,
                                           float width, float height)
{
    const wz_ui_remote_control_status_t* remote_status =
        wz_ui_window_remote_control(&wz_host_session.ui_window);
    char details[WZ_UI_REMOTE_STATUS_CAPACITY];
    float panel_width;
    float panel_height;
    float panel_x;
    float panel_y;
    if (!wz_host_session.remote_settings_visible || remote_status == NULL) {
        return;
    }
    panel_width = width >= 440.0f ? 420.0f : width - 16.0f;
    panel_height = height >= 238.0f ? 190.0f : height - 48.0f;
    if (panel_width < 160.0f || panel_height < 120.0f) {
        return;
    }
    panel_x = width - panel_width - 8.0f;
    panel_y = height - panel_height - 56.0f;
    if (!nk_begin(context, "Telnet Keyboard & Remote Control",
            nk_rect(panel_x, panel_y, panel_width, panel_height),
            NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE |
                NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE)) {
        nk_end(context);
        return;
    }
    wz_ui_remote_control_status_page(remote_status, details, sizeof(details));
    nk_layout_row_dynamic(context, panel_height - 70.0f, 1);
    nk_label_wrap(context, details);
    nk_layout_row_dynamic(context, 24.0f, 1);
    if (nk_button_label(context, "Close")) {
        wz_host_session.remote_settings_visible = false;
    }
    nk_end(context);
}

static void wz_host_render_native_ui(struct nk_context* context,
                                     float width, float height)
{
    if (context == NULL) return;
    wz_host_ui_draw_menus(context, width);
    wz_host_ui_draw_toolbar(context, width);
    wz_host_ui_draw_tape_manager(context, width);
    wz_host_ui_draw_remote_settings(context, width, height);
    wz_host_ui_draw_status(context, width, height);
}

static void wz_host_render_raster(void)
{
    size_t index;
    const float window_width = (float)sapp_width();
    const float window_height = (float)sapp_height();
    const float viewport_top = 56.0f;
    const float viewport_bottom = window_height - 48.0f;
    const float viewport_height = viewport_bottom - viewport_top;
    const float viewport_width = viewport_height *
        (float)WZ_RASTER_CANONICAL_WIDTH / (float)WZ_RASTER_CANONICAL_HEIGHT;
    const float viewport_left = (window_width - viewport_width) * 0.5f;
    const float viewport_right = viewport_left + viewport_width;
    struct nk_context* ui_context = wz_host_session.ui_toolkit_initialized ?
        snk_new_frame() : NULL;
    {
        float scale = sapp_dpi_scale();
        if (scale <= 0.0f) scale = 1.0f;
        wz_host_render_native_ui(ui_context,
            (float)sapp_width() / scale, (float)sapp_height() / scale);
    }
    if (!wz_host_session.graphics_initialized ||
        wz_machine_render_raster(&wz_host_session.machine,
                                 &wz_host_session.raster) != WZ_RESULT_OK) {
        return;
    }
    (void)wz_presentation_snapshot_publish(&wz_host_session.snapshot,
                                           &wz_host_session.raster);
    for (index = 0u; index < WZ_HOST_RASTER_BYTES; ++index) {
        unsigned sample = wz_host_session.raster.samples[index];
        const uint8_t* color = sample < 16u ? wz_host_palette[sample] :
            wz_host_palette[(sample - 16u) & 15u];
        wz_host_session.raster_rgba[index * 4u] = color[0];
        wz_host_session.raster_rgba[index * 4u + 1u] = color[1];
        wz_host_session.raster_rgba[index * 4u + 2u] = color[2];
        wz_host_session.raster_rgba[index * 4u + 3u] = color[3];
    }
    sg_update_image(wz_host_session.raster_image, &(sg_image_data){
        .mip_levels[0] = {
            .ptr = wz_host_session.raster_rgba,
            .size = sizeof(wz_host_session.raster_rgba)
        }
    });
    sg_begin_pass(&(sg_pass){
        .action = {
            .colors[0] = {
                .load_action = SG_LOADACTION_CLEAR,
                .clear_value = {0.02f, 0.02f, 0.02f, 1.0f}
            }
        },
        .swapchain = sglue_swapchain()
    });
    sgl_defaults();
    sgl_viewport(0, 0, sapp_width(), sapp_height(), true);
    sgl_enable_texture();
    sgl_texture(wz_host_session.raster_view, wz_host_session.raster_sampler);
    sgl_begin_quads();
    sgl_t2f(0.0f, 0.0f); sgl_v2f(viewport_left / window_width * 2.0f - 1.0f,
                                  1.0f - viewport_bottom / window_height * 2.0f);
    sgl_t2f(1.0f, 0.0f); sgl_v2f(viewport_right / window_width * 2.0f - 1.0f,
                                  1.0f - viewport_bottom / window_height * 2.0f);
    sgl_t2f(1.0f, 1.0f); sgl_v2f(viewport_right / window_width * 2.0f - 1.0f,
                                  1.0f - viewport_top / window_height * 2.0f);
    sgl_t2f(0.0f, 1.0f); sgl_v2f(viewport_left / window_width * 2.0f - 1.0f,
                                  1.0f - viewport_top / window_height * 2.0f);
    sgl_end();
    sgl_draw();
    sgl_disable_texture();
    if (wz_host_session.ui_toolkit_initialized) {
        snk_render(sapp_width(), sapp_height());
    }
    sg_end_pass();
    sg_commit();
}

static void wz_host_session_init(void)
{
    const wz_machine_profile_t* profile;
    stm_setup();
    wz_host_session.speed = WZ_SPEED_100;
    sg_setup(&(sg_desc){.environment = sglue_environment()});
    sgl_setup(&(sgl_desc_t){0});
    wz_host_session.graphics_initialized = sg_isvalid();
    if (wz_host_session.graphics_initialized) {
        snk_setup(&(snk_desc_t){.dpi_scale = sapp_dpi_scale()});
        wz_host_session.ui_toolkit_initialized = true;
        (void)wz_presentation_snapshot_init(&wz_host_session.snapshot,
                                            WZ_RASTER_CANONICAL_WIDTH,
                                            WZ_RASTER_CANONICAL_HEIGHT,
                                            wz_host_session.snapshot_samples,
                                            sizeof(wz_host_session.snapshot_samples));
        (void)wz_raster_buffer_init(&wz_host_session.raster,
                                    WZ_RASTER_CANONICAL_WIDTH,
                                    WZ_RASTER_CANONICAL_HEIGHT,
                                    wz_host_session.raster_samples,
                                    sizeof(wz_host_session.raster_samples));
        wz_host_session.raster_image = sg_make_image(&(sg_image_desc){
            .width = WZ_RASTER_CANONICAL_WIDTH,
            .height = WZ_RASTER_CANONICAL_HEIGHT,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .data.mip_levels[0] = {
                .ptr = wz_host_session.raster_rgba,
                .size = sizeof(wz_host_session.raster_rgba)
            }
        });
        wz_host_session.raster_view = sg_make_view(&(sg_view_desc){
            .texture = {.image = wz_host_session.raster_image}
        });
        wz_host_session.raster_sampler = sg_make_sampler(&(sg_sampler_desc){
            .min_filter = SG_FILTER_NEAREST,
            .mag_filter = SG_FILTER_NEAREST,
            .wrap_u = SG_WRAP_CLAMP_TO_EDGE,
            .wrap_v = SG_WRAP_CLAMP_TO_EDGE
        });
    }
    wz_control_port_owner_init(&wz_host_session.control_port);
    wz_telnet_client_gate_init(&wz_host_session.telnet_client);
    wz_telnet_negotiator_init(&wz_host_session.telnet_negotiator);
    wz_telnet_command_buffer_init(&wz_host_session.telnet_commands);
    wz_input_arbiter_init(&wz_host_session.input_arbiter);
    wz_telnet_key_press_state_init(&wz_host_session.telnet_key_presses);
    wz_input_focus_init(&wz_host_session.input_focus,
                        &wz_host_session.input_arbiter);
    (void)wz_application_lifecycle_init(&wz_host_session.lifecycle, 0, 0);
    wz_host_session.socket_system_initialized = wz_host_socket_system_init();
    if (wz_host_session.socket_system_initialized) {
        (void)wz_control_port_probe(&wz_host_session.control_port);
    }
    wz_telnet_client_bind_input(&wz_host_session.telnet_client,
                                &wz_host_session.input_arbiter);
    (void)wz_command_registry_init(&wz_host_session.command_registry,
                                   wz_host_session.command_storage,
                                   WZ_HOST_COMMAND_CAPACITY);
    wz_host_session.initialized =
        wz_machine_init(&wz_host_session.machine, wz_machine_profile_48k_pal()) == WZ_RESULT_OK;
    if (wz_host_session.initialized) {
        profile = wz_host_session.machine.profile;
        if (profile != NULL && profile->master_hz_den != 0u &&
            profile->master_hz_num / profile->master_hz_den != 0u) {
            wz_host_session.pacing_initialized = wz_host_pacing_init(
                &wz_host_session.pacing,
                profile->master_hz_num / profile->master_hz_den,
                wz_host_session.speed, wz_host_now_nanoseconds(),
                wz_host_session.machine.master_tick);
        }
        if (!wz_host_session.pacing_initialized) {
            wz_machine_destroy(&wz_host_session.machine);
            wz_host_session.initialized = false;
            wz_control_port_owner_close(&wz_host_session.control_port);
            if (wz_host_session.socket_system_initialized) {
                wz_host_socket_system_shutdown();
                wz_host_session.socket_system_initialized = false;
            }
            return;
        }
        {
            const char* rom_path = getenv("WZSN_ROM_PATH");
            const char* tape_path = getenv("WZSN_TAPE_PATH");
            if (rom_path != NULL && !wz_host_load_external_rom(&wz_host_session, rom_path)) {
                (void)fprintf(stderr, "WZSN_ROM_PATH could not be loaded: %s\n", rom_path);
            }
            if (tape_path != NULL && !wz_host_load_external_tape(&wz_host_session, tape_path)) {
                (void)fprintf(stderr, "WZSN_TAPE_PATH could not be loaded: %s\n", tape_path);
            }
        }
        if (!wz_ui_window_init(&wz_host_session.ui_window)) {
            wz_machine_destroy(&wz_host_session.machine);
            wz_host_session.initialized = false;
            wz_control_port_owner_close(&wz_host_session.control_port);
            return;
        }
        wz_host_session.ui_window.layout.tape_mounted =
            wz_host_session.machine.tape_mounted != 0u;
        wz_host_session.ui_window.layout.microdrive1_mounted =
            wz_host_session.machine.microdrive.image_present != 0u;
        wz_host_session.ui_window.layout.microdrive_mounted[0] =
            wz_host_session.machine.microdrive.image_present != 0u;
        (void)wz_headless_runner_init(&wz_host_session.runner,
                                      &wz_host_session.machine, 0);
        if (!wz_host_register_commands()) {
            wz_machine_destroy(&wz_host_session.machine);
            wz_host_session.initialized = false;
            wz_control_port_owner_close(&wz_host_session.control_port);
            return;
        }
        wz_ui_window_sync_remote_control(&wz_host_session.ui_window,
                                         &wz_host_session.control_port,
                                         &wz_host_session.telnet_client);
#ifndef NDEBUG
        wz_host_timing_trace_start(&wz_host_session);
#endif
        (void)wz_sokol_audio_init(&wz_host_session.audio);
    }
}

static void wz_host_session_shutdown(void)
{
    if (wz_host_session.initialized) {
        wz_telnet_client_disconnect(&wz_host_session.telnet_client);
#ifndef NDEBUG
        wz_host_timing_trace_stop(&wz_host_session);
#endif
        wz_machine_destroy(&wz_host_session.machine);
        wz_ui_window_destroy(&wz_host_session.ui_window);
        wz_sokol_audio_shutdown(&wz_host_session.audio);
        (void)wz_application_mark_terminated(&wz_host_session.lifecycle);
        wz_host_session.initialized = false;
    }
    wz_telnet_client_disconnect(&wz_host_session.telnet_client);
    free(wz_host_session.tape_segments);
    wz_host_session.tape_segments = NULL;
    wz_host_session.tape_segment_count = 0u;
    free(wz_host_session.tape_image_data);
    wz_host_session.tape_image_data = NULL;
    wz_host_session.tape_image_length = 0u;
    free(wz_host_session.tape_blocks);
    wz_host_session.tape_blocks = NULL;
    free(wz_host_session.tzx_blocks);
    wz_host_session.tzx_blocks = NULL;
    free(wz_host_session.microdrive_data);
    wz_host_session.microdrive_data = NULL;
    wz_host_session.microdrive_image.data = NULL;
    wz_control_port_owner_close(&wz_host_session.control_port);
    if (wz_host_session.socket_system_initialized) {
        wz_host_socket_system_shutdown();
        wz_host_session.socket_system_initialized = false;
    }
    if (wz_host_session.graphics_initialized) {
        if (wz_host_session.ui_toolkit_initialized) {
            snk_shutdown();
            wz_host_session.ui_toolkit_initialized = false;
        }
        sg_destroy_sampler(wz_host_session.raster_sampler);
        sg_destroy_view(wz_host_session.raster_view);
        sg_destroy_image(wz_host_session.raster_image);
        sgl_shutdown();
        sg_shutdown();
        wz_host_session.graphics_initialized = false;
    }
}

static void wz_host_init(void)
{
    wz_host_session_init();
}

static void wz_host_cleanup(void)
{
    wz_host_session_shutdown();
}

static bool wz_host_keycode_to_spectrum_key(sapp_keycode key_code,
                                             size_t* physical_key)
{
    static const wz_keyboard_key_t letters[26] = {
        WZ_KEY_A, WZ_KEY_B, WZ_KEY_C, WZ_KEY_D, WZ_KEY_E, WZ_KEY_F,
        WZ_KEY_G, WZ_KEY_H, WZ_KEY_I, WZ_KEY_J, WZ_KEY_K, WZ_KEY_L,
        WZ_KEY_M, WZ_KEY_N, WZ_KEY_O, WZ_KEY_P, WZ_KEY_Q, WZ_KEY_R,
        WZ_KEY_S, WZ_KEY_T, WZ_KEY_U, WZ_KEY_V, WZ_KEY_W, WZ_KEY_X,
        WZ_KEY_Y, WZ_KEY_Z
    };
    static const wz_keyboard_key_t digits[10] = {
        WZ_KEY_0, WZ_KEY_1, WZ_KEY_2, WZ_KEY_3, WZ_KEY_4,
        WZ_KEY_5, WZ_KEY_6, WZ_KEY_7, WZ_KEY_8, WZ_KEY_9
    };
    if (physical_key == NULL) return false;
    if (key_code >= SAPP_KEYCODE_A && key_code <= SAPP_KEYCODE_Z) {
        size_t index = (size_t)(key_code - SAPP_KEYCODE_A);
        if (index < sizeof(letters) / sizeof(letters[0])) {
            *physical_key = (size_t)letters[index];
            return true;
        }
        return false;
    }
    if (key_code >= SAPP_KEYCODE_0 && key_code <= SAPP_KEYCODE_9) {
        size_t index = (size_t)(key_code - SAPP_KEYCODE_0);
        if (index < sizeof(digits) / sizeof(digits[0])) {
            *physical_key = (size_t)digits[index];
            return true;
        }
        return false;
    }
    switch (key_code) {
    case SAPP_KEYCODE_ENTER:
        *physical_key = WZ_KEY_ENTER;
        return true;
    case SAPP_KEYCODE_SPACE:
        *physical_key = WZ_KEY_SPACE;
        return true;
    default:
        return false;
    }
}

static bool wz_host_set_local_key(sapp_keycode key_code, bool pressed)
{
    size_t physical_key;
    if (key_code == SAPP_KEYCODE_LEFT_SHIFT) {
        wz_host_session.local_left_shift_down = pressed;
        physical_key = WZ_KEY_SHIFT;
        pressed = wz_host_session.local_left_shift_down ||
                  wz_host_session.local_right_shift_down;
    } else if (key_code == SAPP_KEYCODE_RIGHT_SHIFT) {
        wz_host_session.local_right_shift_down = pressed;
        physical_key = WZ_KEY_SHIFT;
        pressed = wz_host_session.local_left_shift_down ||
                  wz_host_session.local_right_shift_down;
    } else if (key_code == SAPP_KEYCODE_LEFT_CONTROL) {
        wz_host_session.local_left_control_down = pressed;
        physical_key = WZ_KEY_SYMBOL_SHIFT;
        pressed = wz_host_session.local_left_control_down ||
                  wz_host_session.local_right_control_down;
    } else if (key_code == SAPP_KEYCODE_RIGHT_CONTROL) {
        wz_host_session.local_right_control_down = pressed;
        physical_key = WZ_KEY_SYMBOL_SHIFT;
        pressed = wz_host_session.local_left_control_down ||
                  wz_host_session.local_right_control_down;
    } else if (!wz_host_keycode_to_spectrum_key(key_code, &physical_key)) {
        return false;
    }
    (void)wz_input_arbiter_set(&wz_host_session.input_arbiter,
        WZ_INPUT_SOURCE_LOCAL, physical_key, pressed);
    return true;
}

static void wz_host_release_local_keys(void)
{
    wz_host_session.local_left_shift_down = false;
    wz_host_session.local_right_shift_down = false;
    wz_host_session.local_left_control_down = false;
    wz_host_session.local_right_control_down = false;
    (void)wz_input_arbiter_release_source(&wz_host_session.input_arbiter,
                                         WZ_INPUT_SOURCE_LOCAL);
}

static void wz_host_input_focus_from_mouse(const sapp_event* event)
{
    float scale = sapp_dpi_scale();
    float width;
    float height;
    float panel_width;
    float panel_height;
    float panel_x;
    float panel_y;
    bool ui_target;
    if (event == NULL) return;
    if (scale <= 0.0f) scale = 1.0f;
    width = (float)sapp_width() / scale;
    height = (float)sapp_height() / scale;
    ui_target = event->mouse_y <=
        WZ_UI_MENU_BAR_HEIGHT + WZ_UI_TOOLBAR_HEIGHT ||
        event->mouse_y >= height - 48.0f;
    if (wz_host_session.remote_settings_visible) {
        panel_width = width >= 440.0f ? 420.0f : width - 16.0f;
        panel_height = height >= 238.0f ? 190.0f : height - 48.0f;
        panel_x = width - panel_width - 8.0f;
        panel_y = height - panel_height - 56.0f;
        if (event->mouse_x >= panel_x &&
            event->mouse_x <= panel_x + panel_width &&
            event->mouse_y >= panel_y &&
            event->mouse_y <= panel_y + panel_height) {
            ui_target = true;
        }
    }
    if (ui_target) wz_host_release_local_keys();
    (void)wz_input_focus_set_target(&wz_host_session.input_focus,
        ui_target ? WZ_INPUT_FOCUS_TEXT_CONTROL : WZ_INPUT_FOCUS_VIEWPORT);
}

/* Project host input ownership into the single machine-owned keyboard matrix.
 * The arbiter is deliberately host-side state; the core must receive the
 * resolved level before each execution slice so a Telnet key is observable by
 * the emulated ULA while it scans the keyboard. */
static void wz_host_apply_keyboard_input(void)
{
    bool enter_down = wz_input_arbiter_key_down(
        &wz_host_session.input_arbiter, WZ_KEY_ENTER);
    if (enter_down && !wz_host_session.tape_enter_down &&
        wz_host_session.machine.tape_mounted != 0u) {
        if (wz_tape_state_at_end(&wz_host_session.machine.tape_state)) {
            (void)wz_machine_rewind_tape(&wz_host_session.machine);
        }
        (void)wz_machine_set_tape_motor(&wz_host_session.machine, true);
    }
    wz_host_session.tape_enter_down = enter_down;
    for (size_t key = 0u; key < WZ_INPUT_ARBITER_KEY_COUNT; ++key) {
        bool pressed = wz_input_arbiter_key_down(
            &wz_host_session.input_arbiter, key);
        (void)wz_machine_set_keyboard_key(
            &wz_host_session.machine, (wz_byte_t)(key / 5u),
            (wz_byte_t)(key % 5u), pressed);
    }
}

static void wz_host_frame(void)
{
    wz_qword_t requested_sleep_nanoseconds;
    wz_qword_t frame_ticks;
    unsigned frame_count = 1u;

    if (!wz_host_session.initialized) {
        return;
    }
    if (!wz_telnet_client_is_active(&wz_host_session.telnet_client)) {
        if (wz_host_session.control_port.ipv4_active) {
            (void)wz_telnet_client_accept(&wz_host_session.telnet_client,
                                          wz_host_session.control_port.ipv4_socket);
        }
        if (!wz_telnet_client_is_active(&wz_host_session.telnet_client) &&
            wz_host_session.control_port.ipv6_active) {
            (void)wz_telnet_client_accept(&wz_host_session.telnet_client,
                                          wz_host_session.control_port.ipv6_socket);
        }
    }
    (void)wz_telnet_key_press_drain(
        &wz_host_session.telnet_key_presses,
        &wz_host_session.input_arbiter,
        wz_host_session.machine.master_tick);
    wz_host_telnet_poll();
    wz_host_apply_keyboard_input();
    if (!wz_host_session.ui_window.layout.paused) {
        frame_ticks = (wz_master_tick_t)wz_host_session.machine.profile->tstates_per_frame *
            wz_host_session.machine.profile->master_ticks_per_cpu_tstate;
        if (!wz_speed_policy_is_unlimited(wz_host_session.speed)) {
            unsigned percent = wz_speed_policy_percent(wz_host_session.speed);
            frame_count = percent > 100u ? percent / 100u : 1u;
        }
        if (wz_host_session.pacing_initialized) {
            if (!wz_host_pacing_wait(
                    &wz_host_session.pacing, wz_host_now_nanoseconds(),
                    wz_host_session.machine.master_tick, NULL, NULL,
                    &requested_sleep_nanoseconds)) {
                return;
            }
        }
        for (unsigned frame_index = 0u; frame_index < frame_count; ++frame_index) {
            if (frame_index != 0u) {
                (void)wz_telnet_key_press_drain(
                    &wz_host_session.telnet_key_presses,
                    &wz_host_session.input_arbiter,
                    wz_host_session.machine.master_tick);
                wz_host_apply_keyboard_input();
            }
            wz_master_tick_t frame_start_tick = wz_host_session.machine.master_tick;
            wz_byte_t initial_beeper_level = wz_host_session.machine.beeper.level;
            wz_ay_t initial_ay;
            const wz_ay_t* initial_ay_state = &wz_host_session.machine.ay;
            if (wz_host_audio_enabled(wz_host_session.speed) &&
                wz_sokol_audio_valid(&wz_host_session.audio)) {
                initial_ay = wz_host_session.machine.ay;
                initial_ay_state = &initial_ay;
            }
            if (wz_headless_runner_execute(&wz_host_session.runner, frame_ticks) !=
                WZ_RESULT_OK) {
                return;
            }
            wz_host_audio_render_frame(&wz_host_session, frame_start_tick,
                                       initial_beeper_level, initial_ay_state);
        }
        (void)wz_telnet_key_press_drain(
            &wz_host_session.telnet_key_presses,
            &wz_host_session.input_arbiter,
            wz_host_session.machine.master_tick);
        if (wz_host_session.pacing_initialized) {
            if (!wz_host_pacing_wait(
                    &wz_host_session.pacing, wz_host_now_nanoseconds(),
                    wz_host_session.machine.master_tick,
                    wz_host_sleep_nanoseconds, NULL,
                    &requested_sleep_nanoseconds)) {
                return;
            }
        }
    }
    wz_host_render_raster();
    wz_ui_window_sync_remote_control(&wz_host_session.ui_window,
                                     &wz_host_session.control_port,
                                     &wz_host_session.telnet_client);
}

static void wz_host_event(const sapp_event* event)
{
    if (event == NULL) return;
    if (wz_host_session.ui_toolkit_initialized) {
        (void)snk_handle_event(event);
    }
    if (event->type == SAPP_EVENTTYPE_FOCUSED) {
        (void)wz_input_focus_gained(&wz_host_session.input_focus);
        return;
    }
    if (event->type == SAPP_EVENTTYPE_UNFOCUSED) {
        wz_host_release_local_keys();
        (void)wz_input_focus_lost(&wz_host_session.input_focus);
        return;
    }
    if (event->type == SAPP_EVENTTYPE_MOUSE_DOWN) {
        wz_host_input_focus_from_mouse(event);
        return;
    }
    if ((event->type == SAPP_EVENTTYPE_KEY_DOWN ||
        event->type == SAPP_EVENTTYPE_KEY_UP) &&
        wz_input_focus_forwards_viewport_keys(&wz_host_session.input_focus) &&
        wz_host_set_local_key(event->key_code,
            event->type == SAPP_EVENTTYPE_KEY_DOWN)) {
        return;
    }
    if (event->type == SAPP_EVENTTYPE_KEY_DOWN &&
        event->key_code == SAPP_KEYCODE_ESCAPE &&
        wz_input_focus_forwards_viewport_keys(&wz_host_session.input_focus)) {
        if (wz_application_request_quit(&wz_host_session.lifecycle) == WZ_RESULT_OK) {
            sapp_request_quit();
        }
        return;
    }
}

int main(void)
{
    sapp_run(&(sapp_desc){
        .init_cb = wz_host_init,
        .frame_cb = wz_host_frame,
        .cleanup_cb = wz_host_cleanup,
        .event_cb = wz_host_event,
        .width = 1024,
        .height = 768,
        .window_title = "Warajevo ZX Spectrum Next",
        .swap_interval = 0,
    });
    return 0;
}
