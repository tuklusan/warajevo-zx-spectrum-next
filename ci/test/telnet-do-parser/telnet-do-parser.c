/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_telnet_keyboard_command.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "Telnet DO parser failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    char id[64];
    char arguments[128];

    REQUIRE(wz_telnet_do_parse("DO machine.name.set \"ZX Spectrum Next\"",
                               id, sizeof(id), arguments, sizeof(arguments)));
    REQUIRE(strcmp(id, "machine.name.set") == 0);
    REQUIRE(strcmp(arguments, "ZX Spectrum Next") == 0);

    REQUIRE(wz_telnet_do_parse("DO host.path.read \"a\\\"b\\\\c\"",
                               id, sizeof(id), arguments, sizeof(arguments)));
    REQUIRE(strcmp(arguments, "a\"b\\c") == 0);

    REQUIRE(wz_telnet_do_parse("DO host.path.read \"$HOME *.tap `id`\"",
                               id, sizeof(id), arguments, sizeof(arguments)));
    REQUIRE(strcmp(arguments, "$HOME *.tap `id`") == 0);

    REQUIRE(wz_telnet_do_parse("DO\tmachine.model.set\t128k", id, sizeof(id),
                               arguments, sizeof(arguments)));
    REQUIRE(strcmp(arguments, "128k") == 0);

    REQUIRE(!wz_telnet_do_parse("DO machine.model.set 128k extra", id,
                                sizeof(id), arguments, sizeof(arguments)));
    REQUIRE(!wz_telnet_do_parse("DO machine.name.set \"bad\\q\"", id,
                                sizeof(id), arguments, sizeof(arguments)));
    REQUIRE(!wz_telnet_do_parse("DO machine.name.set \"\"", id, sizeof(id),
                                arguments, sizeof(arguments)));
    REQUIRE(!wz_telnet_do_parse("DO machine.model.set \"128k\"tail", id,
                                sizeof(id), arguments, sizeof(arguments)));
    REQUIRE(!wz_telnet_do_parse("DO machine.model.set 128k", id,
                                sizeof(id), arguments, 4u));
    return 0;
}
