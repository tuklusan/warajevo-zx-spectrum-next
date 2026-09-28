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
        fprintf(stderr, "Telnet model alias regression failed at line %d: %s\n", \
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
    char projected[64];
    REQUIRE(wz_telnet_model_alias_to_do("MODEL 48K", projected,
                                         sizeof(projected)));
    REQUIRE(strcmp(projected, "DO machine.model.set 48k") == 0);
    REQUIRE(wz_telnet_model_alias_to_do("MODEL 128K", projected,
                                         sizeof(projected)));
    REQUIRE(strcmp(projected, "DO machine.model.set 128k") == 0);
    REQUIRE(wz_telnet_model_alias_to_do("MODEL 16K", projected,
                                         sizeof(projected)));
    REQUIRE(strcmp(projected, "DO machine.model.set invalid") == 0);
    REQUIRE(expect("MODEL 48K", "OK DO machine.model.set 48k\r\n",
                   "OK MODEL 48K\r\n"));
    REQUIRE(expect("MODEL 128K", "OK DO machine.model.set 128k\r\n",
                   "OK MODEL 128K\r\n"));
    REQUIRE(expect("MODEL 16K", "ERR DO machine.model.set bad-model\r\n",
                   "ERR BAD_MODEL\r\n"));
    puts("Telnet model alias regression passed");
    return 0;
}
