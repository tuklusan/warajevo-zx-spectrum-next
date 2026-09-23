/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_TELNET_KEYBOARD_COMMAND_H
#define WZ_APP_WZ_TELNET_KEYBOARD_COMMAND_H

#include <stdbool.h>
#include <stddef.h>

#include "app/wz_command_registry.h"
#include "core/wz_presentation_snapshot.h"

typedef enum {
    WZ_TELNET_KEYBOARD_RESPONSE_OK = 0,
    WZ_TELNET_KEYBOARD_RESPONSE_BAD_KEY,
    WZ_TELNET_KEYBOARD_RESPONSE_BAD_STATE
} wz_telnet_keyboard_response_t;

bool wz_telnet_keyboard_command_key_down(const char* command,
                                         size_t* physical_key);
bool wz_telnet_keyboard_command_key_up(const char* command,
                                       size_t* physical_key);
bool wz_telnet_keyboard_command_key_press(const char* command,
                                          size_t* physical_key);

bool wz_telnet_keyboard_command_format_response(
    wz_telnet_keyboard_response_t response,
    char* output,
    size_t output_capacity,
    size_t* output_length);

typedef enum {
    WZ_TELNET_ERROR_BAD_COMMAND = 0,
    WZ_TELNET_ERROR_BAD_COMMAND_ID,
    WZ_TELNET_ERROR_BAD_ARGUMENT,
    WZ_TELNET_ERROR_LINE_TOO_LONG,
    WZ_TELNET_ERROR_BAD_KEY,
    WZ_TELNET_ERROR_BAD_MODEL,
    WZ_TELNET_ERROR_BAD_SPEED
} wz_telnet_error_t;

bool wz_telnet_error_format(wz_telnet_error_t error, char* output,
                             size_t output_capacity, size_t* output_length);

bool wz_telnet_help_format(char* output,
                            size_t output_capacity,
                            size_t* output_length);

typedef struct wz_telnet_status_snapshot {
    unsigned control_port;
    bool ipv4_up;
    bool ipv6_up;
    bool client_active;
    const char* model;
    const char* state;
    const char* speed;
    const char* audio;
    const char* networking;
} wz_telnet_status_snapshot_t;

bool wz_telnet_status_format(const wz_telnet_status_snapshot_t* snapshot,
                             char* output,
                             size_t output_capacity,
                             size_t* output_length);

bool wz_telnet_reset_format_response(bool reset_succeeded,
                                     char* output,
                                     size_t output_capacity,
                                     size_t* output_length);

typedef enum {
    WZ_TELNET_PAUSE_RESUME_PAUSE = 0,
    WZ_TELNET_PAUSE_RESUME_RESUME
} wz_telnet_pause_resume_command_t;

bool wz_telnet_pause_resume_parse(const char* command,
                                  wz_telnet_pause_resume_command_t* parsed);
bool wz_telnet_pause_resume_apply(wz_telnet_pause_resume_command_t command,
                                  bool* paused);
bool wz_telnet_pause_resume_format_response(
    wz_telnet_pause_resume_command_t command,
    char* output,
    size_t output_capacity,
    size_t* output_length);

typedef enum {
    WZ_TELNET_MODEL_48K = 0,
    WZ_TELNET_MODEL_128K,
    WZ_TELNET_MODEL_BAD
} wz_telnet_model_command_t;

bool wz_telnet_model_parse(const char* command,
                           wz_telnet_model_command_t* parsed);
bool wz_telnet_model_apply(wz_telnet_model_command_t command, bool* paused);
bool wz_telnet_model_format_response(wz_telnet_model_command_t command,
                                     char* output,
                                     size_t output_capacity,
                                     size_t* output_length);

typedef enum {
    WZ_TELNET_SPEED_25 = 0,
    WZ_TELNET_SPEED_50,
    WZ_TELNET_SPEED_100,
    WZ_TELNET_SPEED_200,
    WZ_TELNET_SPEED_400,
    WZ_TELNET_SPEED_800,
    WZ_TELNET_SPEED_UNLIMITED,
    WZ_TELNET_SPEED_BAD
} wz_telnet_speed_command_t;

bool wz_telnet_speed_parse(const char* command,
                           wz_telnet_speed_command_t* parsed);
bool wz_telnet_speed_apply(wz_telnet_speed_command_t command);
bool wz_telnet_speed_format_response(wz_telnet_speed_command_t command,
                                     char* output,
                                     size_t output_capacity,
                                     size_t* output_length);

typedef enum {
    WZ_TELNET_SCREENSHOT_OK = 0,
    WZ_TELNET_SCREENSHOT_NO_RASTER,
    WZ_TELNET_SCREENSHOT_CANNOT_CREATE_FILE,
    WZ_TELNET_SCREENSHOT_ENCODE_FAILED,
    WZ_TELNET_SCREENSHOT_WRITE_FAILED
} wz_telnet_screenshot_result_t;

bool wz_telnet_screenshot_parse(const char* command);
wz_telnet_screenshot_result_t wz_telnet_screenshot_save(
    const wz_presentation_snapshot_t* snapshot,
    char* path, size_t path_capacity);
bool wz_telnet_screenshot_format_response(
    wz_telnet_screenshot_result_t result, const char* path,
    char* output, size_t output_capacity, size_t* output_length);

bool wz_telnet_menu_parse(const char* command);
bool wz_telnet_menu_format_root(char* output, size_t output_capacity,
                                 size_t* output_length);
bool wz_telnet_menu_tree_parse(const char* command);
bool wz_telnet_menu_tree_format(const wz_command_registry_t* registry,
                                char* output, size_t output_capacity,
                                size_t* output_length);
bool wz_telnet_menu_id_parse(const char* command, char* id, size_t id_capacity);
bool wz_telnet_menu_id_format(const wz_command_registry_t* registry,
                              const char* id, char* output,
                              size_t output_capacity, size_t* output_length);
bool wz_telnet_menu_find_parse(const char* command, char* text, size_t text_capacity);
bool wz_telnet_menu_find_format(const wz_command_registry_t* registry,
                                const char* text, char* output,
                                size_t output_capacity, size_t* output_length);
bool wz_telnet_describe_parse(const char* command, char* id, size_t id_capacity);
bool wz_telnet_describe_format(const wz_command_registry_t* registry,
                               const char* id, char* output,
                               size_t output_capacity, size_t* output_length);
bool wz_telnet_do_parse(const char* command, char* id, size_t id_capacity,
                        char* arguments, size_t arguments_capacity);
bool wz_telnet_do_format(const wz_command_registry_t* registry,
                         const char* id, const char* arguments,
                         char* output, size_t output_capacity,
                         size_t* output_length);
bool wz_telnet_alias_to_do(const char* alias, char* output,
                           size_t output_capacity);

#endif
