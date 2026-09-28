/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_telnet_keyboard_command.h"
#include "app/wz_telnet_negotiation.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(test) do { \
    if (!(test)) { \
        fprintf(stderr, "Telnet protocol regression failed at line %d: %s\n", \
                __LINE__, #test); \
        return 1; \
    } \
    ++cases; \
} while (0)

int main(void)
{
    unsigned cases = 0u;
    char output[4096];
    size_t output_length = 0u;
    REQUIRE(wz_telnet_help_format(output, sizeof(output), &output_length));
    REQUIRE(output_length != 0u && strstr(output, "STATUS") != NULL &&
            strstr(output, "SCREENSHOT") != NULL && strstr(output, "END\r\n") != NULL);

    {
        const uint8_t quoted[] = "DO machine.model.set \"128k\"\r\n";
        char storage[64];
        const char* tokens[4];
        size_t token_count = 0u;
        REQUIRE(wz_telnet_command_tokenize(quoted, sizeof(quoted) - 3u,
                    storage, sizeof(storage), tokens, 4u, &token_count));
        REQUIRE(token_count == 3u && strcmp(tokens[2], "128k") == 0);
    }
    {
        const uint8_t escaped[] = "DO host.path.read \"a\\\"b\\\\c\"";
        char storage[64];
        const char* tokens[4];
        size_t token_count = 0u;
        REQUIRE(wz_telnet_command_tokenize(escaped, sizeof(escaped) - 1u,
                    storage, sizeof(storage), tokens, 4u, &token_count));
        REQUIRE(token_count == 3u && strcmp(tokens[2], "a\"b\\c") == 0);
    }
    {
        const uint8_t expansion[] = "DO host.path.read \"$HOME `id`\"";
        char storage[64];
        const char* tokens[4];
        size_t token_count = 0u;
        REQUIRE(wz_telnet_command_tokenize(expansion, sizeof(expansion) - 1u,
                    storage, sizeof(storage), tokens, 4u, &token_count));
        REQUIRE(token_count == 3u && strcmp(tokens[2], "$HOME `id`") == 0);
    }
    {
        wz_telnet_command_buffer_t buffer;
        uint8_t command[WZ_TELNET_COMMAND_CAPACITY + 1u];
        bool malformed = false;
        wz_telnet_command_error_t error = WZ_TELNET_COMMAND_ERROR_NONE;
        size_t command_length = 0u;
        uint8_t line[WZ_TELNET_COMMAND_CAPACITY + 2u];
        memset(line, 'A', sizeof(line));
        line[WZ_TELNET_COMMAND_CAPACITY + 1u] = '\n';
        wz_telnet_command_buffer_init(&buffer);
        REQUIRE(wz_telnet_command_buffer_feed(&buffer, line,
                    WZ_TELNET_COMMAND_CAPACITY, command, sizeof(command),
                    &command_length, &malformed, &error));
        REQUIRE(command_length == 0u && !malformed);
        REQUIRE(wz_telnet_command_buffer_feed(&buffer, line + WZ_TELNET_COMMAND_CAPACITY + 1u,
                    1u, command, sizeof(command), &command_length, &malformed, &error));
        REQUIRE(command_length == WZ_TELNET_COMMAND_CAPACITY && !malformed);
        wz_telnet_command_buffer_init(&buffer);
        REQUIRE(wz_telnet_command_buffer_feed(&buffer, line, sizeof(line),
                    command, sizeof(command), &command_length, &malformed, &error));
        REQUIRE(malformed && error == WZ_TELNET_COMMAND_ERROR_LINE_TOO_LONG);
        {
            const uint8_t recovery[] = "HELP\n";
            REQUIRE(wz_telnet_command_buffer_feed(&buffer, recovery,
                        sizeof(recovery) - 1u, command, sizeof(command),
                        &command_length, &malformed, &error));
            REQUIRE(!malformed && command_length == 4u &&
                    memcmp(command, "HELP", 4u) == 0);
        }
    }
    {
        const uint8_t polluted[] = {'H', 'E', 'L', 'P', 0x1b, '[', '2', 'J', '\n'};
        wz_telnet_command_buffer_t buffer;
        uint8_t command[32];
        size_t command_length = 0u;
        bool malformed = false;
        wz_telnet_command_error_t error = WZ_TELNET_COMMAND_ERROR_NONE;
        wz_telnet_command_buffer_init(&buffer);
        REQUIRE(wz_telnet_command_buffer_feed(&buffer, polluted, sizeof(polluted),
                    command, sizeof(command), &command_length, &malformed, &error));
        REQUIRE(malformed && error == WZ_TELNET_COMMAND_ERROR_MALFORMED_ASCII);
    }
    {
        wz_telnet_status_snapshot_t status = {
            .control_port = 30740u, .ipv4_up = true, .ipv6_up = true,
            .client_active = true, .model = "48K", .state = "PAUSED",
            .speed = "UNLIMITED", .audio = "MUTED", .networking = "NONE"
        };
        REQUIRE(wz_telnet_status_format(&status, output, sizeof(output), &output_length));
        REQUIRE(strstr(output, "PORT=30740") != NULL &&
                strstr(output, "STATE=PAUSED") != NULL &&
                strstr(output, "SPEED=UNLIMITED") != NULL &&
                strstr(output, "END\r\n") != NULL);
    }
    {
        wz_telnet_negotiator_t parser;
        const uint8_t request[] = {255u, 253u, 1u, 'H', 'I'};
        uint8_t application[8], protocol[8];
        size_t application_length = 0u, protocol_length = 0u;
        wz_telnet_negotiator_init(&parser);
        REQUIRE(wz_telnet_negotiator_feed(&parser, request, sizeof(request),
                    application, sizeof(application), &application_length,
                    protocol, sizeof(protocol), &protocol_length));
        REQUIRE(application_length == 2u && memcmp(application, "HI", 2u) == 0);
        REQUIRE(protocol_length == 3u && protocol[0] == 255u &&
                protocol[1] == 252u && protocol[2] == 1u);
    }
    REQUIRE(wz_telnet_screenshot_parse("SCREENSHOT"));
    REQUIRE(!wz_telnet_screenshot_parse("SCREENSHOT extra"));
    {
        size_t length = 0u;
        REQUIRE(wz_telnet_screenshot_format_response(
                    WZ_TELNET_SCREENSHOT_NO_RASTER, "", output,
                    sizeof(output), &length));
        REQUIRE(length != 0u && strncmp(output, "ERR", 3u) == 0 &&
                strstr(output, "\r\n") != NULL);
    }
    printf("Telnet protocol regression passed (%u cases)\n", cases);
    return 0;
}
