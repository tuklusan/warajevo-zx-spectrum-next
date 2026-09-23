/*
 * Warajevo ZX Spectrum Next
 * Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
 * New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
 * Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
 * See LICENSE.txt and NOTICE.md for complete terms and provenance.
 */

#ifndef WZ_TELNET_NEGOTIATION_H
#define WZ_TELNET_NEGOTIATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum wz_telnet_parser_state {
    WZ_TELNET_DATA = 0,
    WZ_TELNET_IAC,
    WZ_TELNET_NEGOTIATION,
    WZ_TELNET_SUBNEGOTIATION,
    WZ_TELNET_SUBNEGOTIATION_IAC
} wz_telnet_parser_state_t;

typedef struct wz_telnet_negotiator {
    wz_telnet_parser_state_t state;
    uint8_t command;
} wz_telnet_negotiator_t;

#define WZ_TELNET_COMMAND_CAPACITY 1024u
#define WZ_TELNET_TOKEN_CAPACITY 32u

typedef enum wz_telnet_command_error {
    WZ_TELNET_COMMAND_ERROR_NONE = 0,
    WZ_TELNET_COMMAND_ERROR_MALFORMED_ASCII,
    WZ_TELNET_COMMAND_ERROR_LINE_TOO_LONG
} wz_telnet_command_error_t;

typedef struct wz_telnet_command_buffer {
    uint8_t bytes[WZ_TELNET_COMMAND_CAPACITY];
    size_t length;
    bool discarding;
    bool line_too_long;
} wz_telnet_command_buffer_t;

void wz_telnet_command_buffer_init(wz_telnet_command_buffer_t* buffer);
bool wz_telnet_command_buffer_feed(
    wz_telnet_command_buffer_t* buffer,
    const uint8_t* input,
    size_t input_length,
    uint8_t* command_output,
    size_t command_capacity,
    size_t* command_length,
    bool* malformed,
    wz_telnet_command_error_t* error);

bool wz_telnet_command_tokenize(
    const uint8_t* input,
    size_t input_length,
    char* storage,
    size_t storage_capacity,
    const char** tokens,
    size_t token_capacity,
    size_t* token_count);

void wz_telnet_negotiator_init(wz_telnet_negotiator_t* parser);
bool wz_telnet_negotiator_feed(
    wz_telnet_negotiator_t* parser,
    const uint8_t* input,
    size_t input_length,
    uint8_t* application_output,
    size_t application_capacity,
    size_t* application_length,
    uint8_t* protocol_output,
    size_t protocol_capacity,
    size_t* protocol_length);

#endif
