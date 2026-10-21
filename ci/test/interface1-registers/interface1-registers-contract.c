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
        (void)fprintf(stderr, "Interface 1 register contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
    ++checks; \
} while (0)

static wz_result_t io(wz_machine_t* machine, wz_bus_cycle_t cycle,
                      wz_word_t address, wz_byte_t* value)
{
    wz_bus_request_t request;
    if (machine == 0 || value == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    wz_bus_request_init(&request, cycle, machine->master_tick, address,
                        cycle == WZ_BUS_IO_WRITE ? *value : 0u, 4u);
    if (wz_machine_bus_request(machine, &request) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    *value = request.value;
    return WZ_RESULT_OK;
}

int main(void)
{
    static wz_machine_t machine;
    static wz_byte_t rom[WZ_INTERFACE1_ROM_SIZE];
    static wz_byte_t media[WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    wz_mdr_image_t image;
    wz_byte_t value = 0u;
    unsigned checks = 0u;
    int result = 1;

    memset(rom, 0x5au, sizeof(rom));
    memset(media, 0x33u, sizeof(media));
    REQUIRE(io(0, WZ_BUS_IO_READ, 0x00efu, &value) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_load_interface1_rom(&machine, rom, sizeof(rom),
            WZ_INTERFACE1_ROM_OLD) == WZ_RESULT_OK);
    REQUIRE(wz_machine_set_networking_mode(&machine,
            WZ_NETWORKING_INTERFACE1) == WZ_RESULT_OK);
    REQUIRE(io(&machine, WZ_BUS_IO_READ, 0x00efu, &value) == WZ_RESULT_OK);
    REQUIRE((value & 0x01u) == 0u);
    REQUIRE(wz_mdr_image_init(&image, media, sizeof(media)) == WZ_RESULT_OK);
    REQUIRE(wz_machine_mount_microdrive(&machine, 1u, &image) == WZ_RESULT_OK);
    REQUIRE(wz_machine_interface1_control_write(&machine, 0x06u, 1u) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_interface1_control_write(&machine, 0x04u, 2u) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_interface1_active_motor(&machine) == 0u);

    /* EF GAP/SYNC counters follow the specified 15 status-read profile. */
    value = 0u;
    for (unsigned index = 0u; index < 15u; ++index) {
        REQUIRE(io(&machine, WZ_BUS_IO_READ, 0xa0efu, &value) == WZ_RESULT_OK);
        REQUIRE((value & 0x06u) == 0x06u);
    }
    REQUIRE(io(&machine, WZ_BUS_IO_READ, 0xa0efu, &value) == WZ_RESULT_OK);
    REQUIRE((value & 0x06u) == 0u);
    REQUIRE((value & 0x01u) == 0u);
    REQUIRE((value & 0x18u) == 0u);

    /* Network reads expose one LSB per mirrored F7 bus access. */
    {
        wz_byte_t block[WZ_ZXNET_DATA_CAPACITY];
        memset(block, 0xa5, sizeof(block));
        REQUIRE(wz_zxnet_feed_read_block(&machine.zxnet, 7u, block,
                sizeof(block)) == WZ_RESULT_OK);
        REQUIRE(wz_zxnet_start_read_collection(&machine.zxnet) == WZ_RESULT_OK);
        REQUIRE(wz_machine_interface1_control_write(&machine, 0x04u, 3u) ==
                WZ_RESULT_OK);
        value = 0u;
        REQUIRE(io(&machine, WZ_BUS_IO_READ, 0x80f7u, &value) == WZ_RESULT_OK);
        REQUIRE(value == 0xffu);
        REQUIRE(io(&machine, WZ_BUS_IO_READ, 0x80f7u, &value) == WZ_RESULT_OK);
        REQUIRE(value == 0xfeu);
    }

    /* Network output writes bits into the active deterministic collection. */
    wz_zxnet_init(&machine.zxnet);
    REQUIRE(wz_zxnet_begin_claim(&machine.zxnet, 0x42u) == WZ_RESULT_OK);
    REQUIRE(wz_zxnet_accept_claim(&machine.zxnet, true) == WZ_RESULT_OK);
    value = 0x01u;
    REQUIRE(io(&machine, WZ_BUS_IO_WRITE, 0x00f7u, &value) == WZ_RESULT_OK);
    REQUIRE(machine.zxnet.state.buffer[0] == 0x80u);
    REQUIRE(machine.zxnet.state.bit_count == 8u);

    /* The serial selection stays electrically idle without host redirection. */
    REQUIRE(wz_machine_interface1_control_write(&machine, 0x05u, 4u) ==
            WZ_RESULT_OK);
    value = 0u;
    REQUIRE(io(&machine, WZ_BUS_IO_READ, 0x00f7u, &value) == WZ_RESULT_OK);
    REQUIRE(value == 0xffu);

    (void)printf("PASS Interface 1 bus registers (%u checks)\n", checks);
    result = 0;

cleanup:
    wz_machine_destroy(&machine);
    return result;
}
