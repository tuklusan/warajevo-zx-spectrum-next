/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_telnet_alias_response.h"

#include <stdio.h>
#include <string.h>

static bool write_response(const char* response, char* output,
                           size_t capacity, size_t* length)
{
    int written = snprintf(output, capacity, "%s", response);
    if (written < 0 || (size_t)written >= capacity) {
        *length = 0u;
        return false;
    }
    *length = (size_t)written;
    return true;
}

bool wz_telnet_alias_response_rewrite(const char* alias,
                                      const char* dispatch_response,
                                      char* output,
                                      size_t output_capacity,
                                      size_t* output_length)
{
    static const char* const speeds[] = {
        "25", "50", "100", "200", "400", "800", "UNLIMITED"
    };
    static const char* const dispatch_speeds[] = {
        "25%", "50%", "100%", "200%", "400%", "800%", "Unlimited"
    };
    char response[64];
    size_t index;
    int written;

    if (alias == NULL || dispatch_response == NULL || output == NULL ||
        output_length == NULL || output_capacity == 0u) {
        if (output_length != NULL) *output_length = 0u;
        return false;
    }
    if (strcmp(alias, "RESET") == 0 &&
        strcmp(dispatch_response, "OK DO machine.reset reset\r\n") == 0) {
        return write_response("OK RESET\r\n", output, output_capacity,
                              output_length);
    }
    if (strcmp(alias, "PAUSE") == 0 &&
        strcmp(dispatch_response, "OK DO machine.pause paused\r\n") == 0) {
        return write_response("OK PAUSE\r\n", output, output_capacity,
                              output_length);
    }
    if (strcmp(alias, "RESUME") == 0 &&
        strcmp(dispatch_response, "OK DO machine.resume running\r\n") == 0) {
        return write_response("OK RESUME\r\n", output, output_capacity,
                              output_length);
    }
    if (strncmp(alias, "SPEED ", 6u) != 0) {
        return false;
    }
    for (index = 0u; index < sizeof(speeds) / sizeof(speeds[0]); ++index) {
        if (strcmp(alias + 6u, speeds[index]) == 0) {
            written = snprintf(response, sizeof(response),
                               "OK DO machine.speed.set %s\r\n",
                               dispatch_speeds[index]);
            if (written < 0 || (size_t)written >= sizeof(response) ||
                strcmp(dispatch_response, response) != 0) {
                return false;
            }
            written = snprintf(response, sizeof(response), "OK SPEED %s\r\n",
                               speeds[index]);
            if (written < 0 || (size_t)written >= sizeof(response)) {
                *output_length = 0u;
                return false;
            }
            return write_response(response, output, output_capacity,
                                  output_length);
        }
    }
    return false;
}
