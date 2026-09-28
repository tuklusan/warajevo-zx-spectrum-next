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

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "alias response regression failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static bool expect(const char* alias, const char* dispatched,
                   const char* expected)
{
    char output[64];
    size_t length = 0u;
    return wz_telnet_alias_response_rewrite(alias, dispatched, output,
                                            sizeof(output), &length) &&
        length == strlen(expected) && strcmp(output, expected) == 0;
}

int main(void)
{
    REQUIRE(expect("RESET", "OK DO machine.reset reset\r\n", "OK RESET\r\n"));
    REQUIRE(expect("PAUSE", "OK DO machine.pause paused\r\n", "OK PAUSE\r\n"));
    REQUIRE(expect("RESUME", "OK DO machine.resume running\r\n", "OK RESUME\r\n"));
    REQUIRE(expect("SPEED 25", "OK DO machine.speed.set 25%\r\n", "OK SPEED 25\r\n"));
    REQUIRE(expect("SPEED 50", "OK DO machine.speed.set 50%\r\n", "OK SPEED 50\r\n"));
    REQUIRE(expect("SPEED 100", "OK DO machine.speed.set 100%\r\n", "OK SPEED 100\r\n"));
    REQUIRE(expect("SPEED 200", "OK DO machine.speed.set 200%\r\n", "OK SPEED 200\r\n"));
    REQUIRE(expect("SPEED 400", "OK DO machine.speed.set 400%\r\n", "OK SPEED 400\r\n"));
    REQUIRE(expect("SPEED 800", "OK DO machine.speed.set 800%\r\n", "OK SPEED 800\r\n"));
    REQUIRE(expect("SPEED UNLIMITED", "OK DO machine.speed.set Unlimited\r\n", "OK SPEED UNLIMITED\r\n"));
    REQUIRE(!expect("RESET", "ERR DO machine.reset failed\r\n", "OK RESET\r\n"));
    puts("alias response regression passed");
    return 0;
}
