/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. New original material is separately identified where
applicable; see LICENSE.txt and NOTICE.md for complete terms and provenance.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

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

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <errno.h>
#include <time.h>
#endif

#include "core/wz_machine.h"
#include "core/wz_runner.h"
#include "core/wz_tape.h"
#include "core/audio/wz_audio_mixer.h"
#include "app/wz_command_registry.h"
#include "app/wz_application_lifecycle.h"
#include "app/wz_control_port.h"
#include "app/wz_host_socket.h"
#include "app/wz_input_arbiter.h"
#include "app/wz_sokol_audio.h"
#include "app/wz_host_pacing.h"
#include "app/wz_host_audio_policy.h"
#include "app/wz_speed_policy.h"
#include "app/wz_telnet_client.h"
#include "app/wz_telnet_keyboard_command.h"
#include "app/wz_telnet_negotiation.h"
#include "app/wz_telnet_input.h"
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
    wz_sokol_audio_t audio;
    wz_host_pacing_t pacing;
    wz_speed_policy_t speed;
    wz_speed_policy_t audio_sample_speed;
    wz_qword_t audio_sample_remainder;
    wz_application_lifecycle_t lifecycle;
    wz_ui_window_t ui_window;
    wz_headless_runner_t runner;
    wz_input_arbiter_t input_arbiter;
    wz_control_port_owner_t control_port;
    wz_telnet_client_gate_t telnet_client;
    wz_command_registry_t command_registry;
    wz_command_metadata_t command_storage[WZ_HOST_COMMAND_CAPACITY];
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
    sg_image raster_image;
    sg_view raster_view;
    sg_sampler raster_sampler;
    bool graphics_initialized;
    bool pacing_initialized;
    bool audio_sample_speed_initialized;
    bool socket_system_initialized;
    bool initialized;
} wz_host_session_t;

static wz_host_session_t wz_host_session;
static bool wz_host_menu_open;
static size_t wz_host_open_menu_index;

static wz_qword_t wz_host_now_nanoseconds(void)
{
    double nanoseconds = stm_ns(stm_now());
    return nanoseconds > 0.0 ? (wz_qword_t)nanoseconds : 0u;
}

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
    wz_host_audio_clear_frame_events(machine);
}

static bool wz_host_read_file(const char* path, wz_byte_t** data, size_t* length)
{
    FILE* file;
    long file_length;
    wz_byte_t* storage;
    size_t read_length;
    if (path == NULL || data == NULL || length == NULL || path[0] == '\0') return false;
    file = fopen(path, "rb");
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

static bool wz_host_load_external_tap(wz_host_session_t* session, const char* path)
{
    wz_byte_t* data = NULL;
    size_t length = 0u;
    size_t segment_count = 0u;
    wz_tape_segment_t* segments = NULL;
    bool loaded = false;
    if (session == NULL || !wz_host_read_file(path, &data, &length)) return false;
    if (wz_tape_parse_standard_tap(data, length, 2u, NULL, 0u,
                                   &segment_count) == WZ_RESULT_BUFFER_TOO_SMALL &&
        segment_count != 0u) {
        segments = (wz_tape_segment_t*)malloc(segment_count * sizeof(*segments));
        if (segments != NULL &&
            wz_tape_parse_standard_tap(data, length, 2u, segments,
                                       segment_count, &segment_count) == WZ_RESULT_OK &&
            wz_machine_mount_tape(&session->machine, segments, segment_count) == WZ_RESULT_OK) {
            free(session->tape_segments);
            session->tape_segments = segments;
            session->tape_segment_count = segment_count;
            segments = NULL;
            loaded = true;
        }
    }
    free(segments);
    free(data);
    return loaded;
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
    (void)snprintf(result->message, sizeof(result->message), "%s",
                   wz_ui_layout_speed_label((size_t)speed));
    return WZ_RESULT_OK;
}

static bool wz_host_register_commands(void)
{
    static const wz_command_metadata_t commands[] = {
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
    if (wz_telnet_alias_to_do(command, projected, sizeof(projected))) {
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
            .client_active = wz_telnet_client_is_active(&wz_host_session.telnet_client),
            .model = "48K",
            .state = "RUNNING",
            .speed = "100%",
            .audio = "enabled",
            .networking = "none"
        };
        (void)wz_telnet_status_format(&status, output, sizeof(output), &length);
    } else if (wz_telnet_keyboard_command_key_down(command, &physical_key)) {
        key_ok = wz_telnet_input_set_key(&wz_host_session.input_arbiter,
                                         physical_key, true);
        (void)wz_telnet_keyboard_command_format_response(
            key_ok ? WZ_TELNET_KEYBOARD_RESPONSE_OK : WZ_TELNET_KEYBOARD_RESPONSE_BAD_STATE,
            output, sizeof(output), &length);
    } else if (wz_telnet_keyboard_command_key_up(command, &physical_key)) {
        key_ok = wz_telnet_input_set_key(&wz_host_session.input_arbiter,
                                         physical_key, false);
        (void)wz_telnet_keyboard_command_format_response(
            key_ok ? WZ_TELNET_KEYBOARD_RESPONSE_OK : WZ_TELNET_KEYBOARD_RESPONSE_BAD_STATE,
            output, sizeof(output), &length);
    } else if (wz_telnet_keyboard_command_key_press(command, &physical_key)) {
        key_ok = wz_telnet_input_set_key(&wz_host_session.input_arbiter,
                                         physical_key, true) &&
            wz_telnet_input_set_key(&wz_host_session.input_arbiter,
                                    physical_key, false);
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

static void wz_host_ui_quad(float left, float top, float right, float bottom,
                            float width, float height,
                            float red, float green, float blue)
{
    const float x0 = left / width * 2.0f - 1.0f;
    const float x1 = right / width * 2.0f - 1.0f;
    const float y0 = 1.0f - top / height * 2.0f;
    const float y1 = 1.0f - bottom / height * 2.0f;
    sgl_c4f(red, green, blue, 1.0f);
    sgl_begin_quads();
    sgl_v2f(x0, y1);
    sgl_v2f(x1, y1);
    sgl_v2f(x1, y0);
    sgl_v2f(x0, y0);
    sgl_end();
}

static void wz_host_render_native_ui(float width, float height)
{
    size_t index;
    const size_t menu_count = wz_ui_layout_menu_count();
    const size_t toolbar_count = wz_ui_layout_toolbar_count();
    const float menu_height = WZ_UI_MENU_BAR_HEIGHT;
    const float toolbar_height = WZ_UI_TOOLBAR_HEIGHT;
    const float status_height = 24.0f;
    const float viewport_top = menu_height + toolbar_height;
    const float viewport_bottom = height - status_height;
    wz_host_ui_quad(0.0f, 0.0f, width, menu_height, width, height,
                    0.04f, 0.08f, 0.20f);
    wz_host_ui_quad(0.0f, menu_height, width, viewport_top, width, height,
                    0.12f, 0.12f, 0.14f);
    wz_host_ui_quad(0.0f, viewport_bottom, width, height, width, height,
                    0.04f, 0.20f, 0.12f);
    for (index = 0u; index < menu_count; ++index) {
        float left = width * (float)index / (float)menu_count;
        float right = width * (float)(index + 1u) / (float)menu_count;
        wz_host_ui_quad(left + 1.0f, 2.0f, right - 1.0f,
                        menu_height - 2.0f, width, height,
                        0.08f, 0.16f + 0.02f * (float)(index & 3u), 0.32f);
    }
    for (index = 0u; index < toolbar_count; ++index) {
        float left = width * (float)index / (float)toolbar_count;
        float right = width * (float)(index + 1u) / (float)toolbar_count;
        wz_host_ui_quad(left + 1.0f, menu_height + 2.0f,
                        right - 1.0f, viewport_top - 2.0f,
                        width, height, 0.22f, 0.22f, 0.26f);
    }
    if (wz_host_menu_open) {
        const size_t item_count = wz_ui_layout_menu_command_count(
            &wz_host_session.command_registry, wz_host_open_menu_index);
        float left = width * (float)wz_host_open_menu_index /
            (float)menu_count;
        float right = left + 240.0f;
        if (right > width) right = width;
        if (item_count != 0u) {
            wz_host_ui_quad(left, menu_height, right,
                            menu_height + (float)item_count * 24.0f,
                            width, height, 0.10f, 0.10f, 0.12f);
            for (index = 0u; index < item_count; ++index) {
                float top = menu_height + (float)index * 24.0f;
                wz_host_ui_quad(left + 2.0f, top + 1.0f, right - 2.0f,
                                top + 23.0f, width, height,
                                0.18f, 0.18f, 0.22f);
            }
        }
    }
}

static void wz_host_render_raster(void)
{
    size_t index;
    const float window_width = (float)sapp_width();
    const float window_height = (float)sapp_height();
    const float viewport_top = 56.0f;
    const float viewport_bottom = window_height - 24.0f;
    const float viewport_height = viewport_bottom - viewport_top;
    const float viewport_width = viewport_height *
        (float)WZ_RASTER_CANONICAL_WIDTH / (float)WZ_RASTER_CANONICAL_HEIGHT;
    const float viewport_left = (window_width - viewport_width) * 0.5f;
    const float viewport_right = viewport_left + viewport_width;
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
    wz_host_render_native_ui(window_width, window_height);
    sgl_draw();
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
            if (tape_path != NULL && !wz_host_load_external_tap(&wz_host_session, tape_path)) {
                (void)fprintf(stderr, "WZSN_TAPE_PATH could not be loaded: %s\n", tape_path);
            }
        }
        if (!wz_ui_window_init(&wz_host_session.ui_window)) {
            wz_machine_destroy(&wz_host_session.machine);
            wz_host_session.initialized = false;
            wz_control_port_owner_close(&wz_host_session.control_port);
            return;
        }
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
        (void)wz_sokol_audio_init(&wz_host_session.audio);
    }
}

static void wz_host_session_shutdown(void)
{
    if (wz_host_session.initialized) {
        wz_telnet_client_disconnect(&wz_host_session.telnet_client);
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
    wz_control_port_owner_close(&wz_host_session.control_port);
    if (wz_host_session.socket_system_initialized) {
        wz_host_socket_system_shutdown();
        wz_host_session.socket_system_initialized = false;
    }
    if (wz_host_session.graphics_initialized) {
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

/* Project host input ownership into the single machine-owned keyboard matrix.
 * The arbiter is deliberately host-side state; the core must receive the
 * resolved level before each execution slice so a Telnet key is observable by
 * the emulated ULA while it scans the keyboard. */
static void wz_host_apply_keyboard_input(void)
{
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
    wz_host_telnet_poll();
    wz_host_apply_keyboard_input();
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
    if (wz_host_session.pacing_initialized) {
        if (!wz_host_pacing_wait(
                &wz_host_session.pacing, wz_host_now_nanoseconds(),
                wz_host_session.machine.master_tick,
                wz_host_sleep_nanoseconds, NULL,
                &requested_sleep_nanoseconds)) {
            return;
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
    if (event->type == SAPP_EVENTTYPE_KEY_DOWN && event->key_code == SAPP_KEYCODE_ESCAPE) {
        if (wz_application_request_quit(&wz_host_session.lifecycle) == WZ_RESULT_OK) {
            sapp_request_quit();
        }
        return;
    }
    if (wz_host_session.initialized &&
        event->type == SAPP_EVENTTYPE_MOUSE_DOWN &&
        event->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
        size_t menu_index;
        size_t menu_command_index;
        wz_command_result_t result;
        if (wz_ui_layout_menu_hit_test(
                event->mouse_x, event->mouse_y, (float)sapp_width(),
                &menu_index)) {
            if (wz_host_menu_open && menu_index == wz_host_open_menu_index) {
                wz_host_menu_open = false;
            } else {
                wz_host_open_menu_index = menu_index;
                wz_host_menu_open = true;
            }
            return;
        }
        if (wz_host_menu_open && wz_ui_layout_menu_command_hit_test(
                &wz_host_session.command_registry, wz_host_open_menu_index,
                event->mouse_x, event->mouse_y, (float)sapp_width(),
                &menu_command_index)) {
            (void)wz_ui_layout_activate_menu_command(
                &wz_host_session.command_registry, wz_host_open_menu_index,
                menu_command_index, (wz_command_arguments_t){NULL, 0u},
                &result);
            wz_host_menu_open = false;
            return;
        }
        wz_host_menu_open = false;
        size_t toolbar_index;
        if (wz_ui_layout_toolbar_hit_test(
                event->mouse_x, event->mouse_y, (float)sapp_width(),
                &toolbar_index)) {
            (void)wz_ui_layout_activate_toolbar(
                &wz_host_session.command_registry, toolbar_index,
                (wz_command_arguments_t){NULL, 0u}, &result);
        }
    }
}

int main(void)
{
    sapp_run(&(sapp_desc){
        .init_cb = wz_host_init,
        .frame_cb = wz_host_frame,
        .cleanup_cb = wz_host_cleanup,
        .event_cb = wz_host_event,
        .width = 640,
        .height = 480,
        .window_title = "Warajevo ZX Spectrum Next",
        .swap_interval = 0,
    });
    return 0;
}
