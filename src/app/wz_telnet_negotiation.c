/*
 * Warajevo ZX Spectrum Next
 * Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
 * New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
 * Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
 * See LICENSE.txt and NOTICE.md for complete terms and provenance.
 */

#include "app/wz_telnet_negotiation.h"

#define WZ_TELNET_IAC_BYTE 255u
#define WZ_TELNET_DONT 254u
#define WZ_TELNET_DO 253u
#define WZ_TELNET_WONT 252u
#define WZ_TELNET_WILL 251u
#define WZ_TELNET_SB 250u
#define WZ_TELNET_SE 240u

static bool append_byte(uint8_t* output, size_t capacity,
                        size_t* length, uint8_t value)
{
    if (*length >= capacity) return false;
    output[(*length)++] = value;
    return true;
}

static bool is_ascii_application_byte(uint8_t byte)
{
    return byte == (uint8_t)'\t' || (byte >= 0x20u && byte <= 0x7eu);
}

bool wz_telnet_command_tokenize(
    const uint8_t* input,
    size_t input_length,
    char* storage,
    size_t storage_capacity,
    const char** tokens,
    size_t token_capacity,
    size_t* token_count)
{
    size_t index = 0u;
    size_t used = 0u;
    size_t count = 0u;
    if ((input == NULL && input_length != 0u) || storage == NULL ||
        tokens == NULL || token_count == NULL || storage_capacity == 0u)
        return false;
    *token_count = 0u;
    while (index < input_length) {
        size_t start;
        bool quoted = false;
        while (index < input_length &&
               (input[index] == (uint8_t)' ' || input[index] == (uint8_t)'\t'))
            ++index;
        if (index == input_length) break;
        if (count >= token_capacity) return false;
        start = used;
        if (input[index] == (uint8_t)'\"') {
            quoted = true;
            ++index;
            while (index < input_length && input[index] != (uint8_t)'\"') {
                uint8_t byte = input[index++];
                if (byte == (uint8_t)'\\') {
                    if (index == input_length ||
                        (input[index] != (uint8_t)'\\' &&
                         input[index] != (uint8_t)'\"')) return false;
                    byte = input[index++];
                } else if (!is_ascii_application_byte(byte)) return false;
                if (used + 1u >= storage_capacity) return false;
                storage[used++] = (char)byte;
            }
            if (index == input_length || input[index++] != (uint8_t)'\"')
                return false;
        }
        while (!quoted && index < input_length &&
               input[index] != (uint8_t)' ' && input[index] != (uint8_t)'\t') {
            uint8_t byte = input[index++];
            if (!is_ascii_application_byte(byte) || byte == (uint8_t)'\"')
                return false;
            if (used + 1u >= storage_capacity) return false;
            storage[used++] = (char)byte;
        }
        if (quoted && index < input_length && input[index] != (uint8_t)' ' &&
            input[index] != (uint8_t)'\t') return false;
        if (used + 1u >= storage_capacity) return false;
        storage[used++] = '\0';
        tokens[count++] = storage + start;
    }
    *token_count = count;
    return true;
}

void wz_telnet_negotiator_init(wz_telnet_negotiator_t* parser)
{
    if (parser == NULL) return;
    parser->state = WZ_TELNET_DATA;
    parser->command = 0u;
}

void wz_telnet_command_buffer_init(wz_telnet_command_buffer_t* buffer)
{
    if (buffer == NULL) return;
    buffer->length = 0u;
    buffer->discarding = false;
    buffer->line_too_long = false;
}

bool wz_telnet_command_buffer_feed(
    wz_telnet_command_buffer_t* buffer,
    const uint8_t* input,
    size_t input_length,
    uint8_t* command_output,
    size_t command_capacity,
    size_t* command_length,
    bool* malformed,
    wz_telnet_command_error_t* error)
{
    size_t index;
    if (buffer == NULL || (input == NULL && input_length != 0u) ||
        command_output == NULL || command_length == NULL || malformed == NULL ||
        error == NULL)
        return false;
    *command_length = 0u;
    *malformed = false;
    *error = WZ_TELNET_COMMAND_ERROR_NONE;
    for (index = 0u; index < input_length; ++index) {
        uint8_t byte = input[index];
        if (byte == (uint8_t)'\n') {
            if (buffer->discarding) {
                *malformed = true;
                *error = buffer->line_too_long
                    ? WZ_TELNET_COMMAND_ERROR_LINE_TOO_LONG
                    : WZ_TELNET_COMMAND_ERROR_MALFORMED_ASCII;
            } else if (buffer->length > command_capacity) {
                buffer->length = 0u;
                return false;
            } else {
                size_t copy_index;
                for (copy_index = 0u; copy_index < buffer->length; ++copy_index)
                    command_output[copy_index] = buffer->bytes[copy_index];
                *command_length = buffer->length;
            }
            buffer->length = 0u;
            buffer->discarding = false;
            buffer->line_too_long = false;
            continue;
        }
        if (buffer->discarding) continue;
        if (byte == (uint8_t)'\r') continue;
        if (!is_ascii_application_byte(byte)) {
            buffer->discarding = true;
            buffer->line_too_long = false;
            continue;
        }
        if (buffer->length >= WZ_TELNET_COMMAND_CAPACITY) {
            buffer->discarding = true;
            buffer->line_too_long = true;
            continue;
        }
        buffer->bytes[buffer->length++] = byte;
    }
    return true;
}

bool wz_telnet_negotiator_feed(
    wz_telnet_negotiator_t* parser,
    const uint8_t* input,
    size_t input_length,
    uint8_t* application_output,
    size_t application_capacity,
    size_t* application_length,
    uint8_t* protocol_output,
    size_t protocol_capacity,
    size_t* protocol_length)
{
    size_t index;
    if (parser == NULL || (input == NULL && input_length != 0u) ||
        application_output == NULL || application_length == NULL ||
        protocol_output == NULL || protocol_length == NULL) return false;
    *application_length = 0u;
    *protocol_length = 0u;
    for (index = 0u; index < input_length; ++index) {
        uint8_t byte = input[index];
        switch (parser->state) {
        case WZ_TELNET_DATA:
            if (byte == WZ_TELNET_IAC_BYTE) {
                parser->state = WZ_TELNET_IAC;
            } else if (!append_byte(application_output, application_capacity,
                                     application_length, byte)) return false;
            break;
        case WZ_TELNET_IAC:
            if (byte == WZ_TELNET_IAC_BYTE) {
                if (!append_byte(application_output, application_capacity,
                                 application_length, byte)) return false;
                parser->state = WZ_TELNET_DATA;
            } else if (byte == WZ_TELNET_WILL || byte == WZ_TELNET_WONT ||
                       byte == WZ_TELNET_DO || byte == WZ_TELNET_DONT) {
                parser->command = byte;
                parser->state = WZ_TELNET_NEGOTIATION;
            } else if (byte == WZ_TELNET_SB) {
                parser->state = WZ_TELNET_SUBNEGOTIATION;
            } else {
                parser->state = WZ_TELNET_DATA;
            }
            break;
        case WZ_TELNET_NEGOTIATION: {
            uint8_t response = (parser->command == WZ_TELNET_DO ||
                                parser->command == WZ_TELNET_DONT)
                ? WZ_TELNET_WONT : WZ_TELNET_DONT;
            if (!append_byte(protocol_output, protocol_capacity,
                             protocol_length, WZ_TELNET_IAC_BYTE) ||
                !append_byte(protocol_output, protocol_capacity,
                             protocol_length, response) ||
                !append_byte(protocol_output, protocol_capacity,
                             protocol_length, byte)) return false;
            parser->state = WZ_TELNET_DATA;
            break;
        }
        case WZ_TELNET_SUBNEGOTIATION:
            if (byte == WZ_TELNET_IAC)
                parser->state = WZ_TELNET_SUBNEGOTIATION_IAC;
            break;
        case WZ_TELNET_SUBNEGOTIATION_IAC:
            if (byte == WZ_TELNET_SE)
                parser->state = WZ_TELNET_DATA;
            else if (byte != WZ_TELNET_IAC)
                parser->state = WZ_TELNET_SUBNEGOTIATION;
            break;
        }
    }
    return true;
}
