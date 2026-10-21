/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/wz_machine.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Interface 1 paging contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

static wz_result_t opcode_fetch(wz_machine_t* machine, wz_word_t address,
                                wz_byte_t* value)
{
    wz_bus_request_t request;
    wz_bus_request_init(&request, WZ_BUS_M1_OPCODE_FETCH, 0u, address, 0u, 4u);
    if (wz_machine_bus_request(machine, &request) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    *value = request.value;
    return WZ_RESULT_OK;
}

static wz_result_t memory_read(wz_machine_t* machine, wz_word_t address,
                               wz_byte_t* value)
{
    wz_bus_request_t request;
    wz_bus_request_init(&request, WZ_BUS_MEMORY_READ, 0u, address, 0u, 3u);
    if (wz_machine_bus_request(machine, &request) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    *value = request.value;
    return WZ_RESULT_OK;
}

int main(void)
{
    static wz_machine_t machine;
    static wz_byte_t main_rom[WZ_48K_ROM_SIZE];
    static wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE];
    static wz_byte_t oversized_rom[WZ_48K_ROM_SIZE];
    wz_byte_t value = 0u;
    wz_qword_t identity;
    int result = 1;

    memset(main_rom, 0x3cu, sizeof(main_rom));
    memset(interface1_rom, 0x5au, sizeof(interface1_rom));
    memset(oversized_rom, 0xa5u, sizeof(oversized_rom));
    interface1_rom[0x0008u] = 0x08u;
    interface1_rom[0x0700u] = 0xc9u;
    interface1_rom[0x1708u] = 0x17u;

    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    memcpy(machine.memory, main_rom, sizeof(main_rom));
    REQUIRE(wz_machine_load_interface1_rom(&machine, interface1_rom,
            WZ_INTERFACE1_ROM_SIZE - 1u, WZ_INTERFACE1_ROM_OLD) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(!wz_machine_has_interface1_rom(&machine));
    REQUIRE(wz_machine_load_interface1_rom(&machine, oversized_rom,
            sizeof(oversized_rom), WZ_INTERFACE1_ROM_OLD) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(!wz_machine_has_interface1_rom(&machine));
    REQUIRE(wz_machine_load_interface1_rom(&machine, interface1_rom,
            sizeof(interface1_rom), WZ_INTERFACE1_ROM_OLD) == WZ_RESULT_OK);
    identity = wz_machine_interface1_rom_identity(&machine);
    REQUIRE(identity != 0u);
    REQUIRE(wz_machine_set_networking_mode(&machine,
            WZ_NETWORKING_INTERFACE1) == WZ_RESULT_OK);
    REQUIRE(wz_machine_interface1_rom_page(&machine) == 0u);

    /* Data reads at trap addresses do not change ROM selection. */
    REQUIRE(memory_read(&machine, 0x0008u, &value) == WZ_RESULT_OK);
    REQUIRE(value == 0x3cu);
    REQUIRE(wz_machine_interface1_rom_page(&machine) == 0u);

    /* The 48K ROM's error and CLOSE# entry fetches select the shadow ROM. */
    REQUIRE(opcode_fetch(&machine, 0x0008u, &value) == WZ_RESULT_OK);
    REQUIRE(value == 0x08u);
    REQUIRE(wz_machine_interface1_rom_page(&machine) == 1u);
    REQUIRE(wz_machine_memory_read(&machine, 0x1fffu) == 0x5au);

    REQUIRE(opcode_fetch(&machine, 0x0700u, &value) == WZ_RESULT_OK);
    REQUIRE(value == 0xc9u);
    REQUIRE(wz_machine_interface1_rom_page(&machine) == 0u);
    REQUIRE(wz_machine_memory_read(&machine, 0x0700u) == 0x3cu);

    REQUIRE(opcode_fetch(&machine, 0x1708u, &value) == WZ_RESULT_OK);
    REQUIRE(value == 0x17u);
    REQUIRE(wz_machine_interface1_rom_page(&machine) == 1u);
    REQUIRE(opcode_fetch(&machine, 0x0700u, &value) == WZ_RESULT_OK);
    REQUIRE(value == 0xc9u);
    REQUIRE(wz_machine_interface1_rom_page(&machine) == 0u);

    REQUIRE(wz_machine_load_interface1_rom(&machine, oversized_rom,
            sizeof(oversized_rom), WZ_INTERFACE1_ROM_NEW) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_machine_interface1_rom_identity(&machine) == identity);
    REQUIRE(wz_machine_memory_read(&machine, 0x0008u) == 0x3cu);
    REQUIRE(wz_machine_set_networking_mode(&machine, WZ_NETWORKING_NONE) ==
            WZ_RESULT_OK);
    REQUIRE(opcode_fetch(&machine, 0x0008u, &value) == WZ_RESULT_OK);
    REQUIRE(value == 0x3cu);
    REQUIRE(wz_machine_interface1_rom_page(&machine) == 0u);

    (void)puts("PASS Interface 1 8 KiB ROM trap paging and unpaging");
    result = 0;

cleanup:
    wz_machine_destroy(&machine);
    return result;
}
