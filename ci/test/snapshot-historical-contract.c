/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/wz_machine.h"
#include "core/wz_state.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define V1_HEADER 30u
#define V1_MEMORY 49152u
#define V1_COMPRESSED_CAPACITY (V1_HEADER + ((V1_MEMORY / 255u) + 1u) * 4u + 4u)

static void put_le16(wz_byte_t* destination, wz_word_t value)
{
    destination[0] = (wz_byte_t)(value & 0xffu);
    destination[1] = (wz_byte_t)(value >> 8u);
}

static void set_v1_header(wz_byte_t* data)
{
    memset(data, 0, V1_HEADER);
    data[0] = 0x5au;
    data[1] = 0x41u;
    data[2] = 0x34u;
    data[3] = 0x12u;
    data[4] = 0x78u;
    data[5] = 0x56u;
    put_le16(data + 6u, 0x8123u);
    put_le16(data + 8u, 0x9abcu);
    data[10] = 0x33u;
    data[11] = 0x44u;
    data[12] = 0x03u | (5u << 1u);
    data[13] = 0x66u;
    data[14] = 0x77u;
    data[15] = 0x88u;
    data[16] = 0x99u;
    data[17] = 0xaau;
    data[18] = 0xbbu;
    data[19] = 0xccu;
    data[20] = 0xddu;
    data[21] = 0xeeu;
    data[22] = 0xffu;
    put_le16(data + 23u, 0x2468u);
    put_le16(data + 25u, 0x1357u);
    data[27] = 1u;
    data[28] = 1u;
    data[29] = 2u;
}

static bool load_into_machine(const wz_byte_t* image, size_t length,
                              wz_result_t (*loader)(wz_snapshot_state_t*,
                                                    const wz_byte_t*, size_t),
                              wz_machine_t* machine)
{
    wz_snapshot_state_t snapshot;
    wz_result_t result = loader(&snapshot, image, length);
    if (result != WZ_RESULT_OK) {
        fprintf(stderr, "snapshot loader failed: %d\\n", (int)result);
        return false;
    }
    result = wz_state_deserialize_machine(machine, wz_snapshot_state_data(&snapshot),
                                          wz_snapshot_state_length(&snapshot));
    if (result != WZ_RESULT_OK) {
        fprintf(stderr, "snapshot deserialize failed: %d\\n", (int)result);
        return false;
    }
    return true;
}

static bool verify_v1_uncompressed(void)
{
    wz_byte_t* image = (wz_byte_t*)malloc(V1_HEADER + V1_MEMORY);
    wz_machine_t machine;
    bool ok = false;
    if (image == NULL || wz_machine_init(&machine, wz_machine_profile_48k_pal()) !=
                             WZ_RESULT_OK) {
        free(image);
        return false;
    }
    set_v1_header(image);
    for (size_t index = 0u; index < V1_MEMORY; ++index) {
        image[V1_HEADER + index] = (wz_byte_t)(index % 251u + 1u);
    }
    ok = load_into_machine(image, V1_HEADER + V1_MEMORY,
                           wz_snapshot_state_load_z80_v1, &machine) &&
         machine.cpu.program_counter == 0x8123u &&
         machine.cpu.stack_pointer == 0x9abcu && machine.cpu.ix == 0x1357u &&
         machine.cpu.iy == 0x2468u && machine.cpu.interrupt_mode == 2u &&
         machine.border_color == 5u && machine.memory[0x4000u] == 1u &&
         machine.memory[0xffffu] ==
             (wz_byte_t)(((V1_MEMORY - 1u) % 251u) + 1u);
    wz_machine_destroy(&machine);
    free(image);
    return ok;
}

static bool verify_v1_compressed(void)
{
    wz_byte_t* image = (wz_byte_t*)malloc(V1_COMPRESSED_CAPACITY);
    wz_machine_t machine;
    size_t offset = V1_HEADER;
    size_t remaining = V1_MEMORY;
    bool ok = false;
    if (image == NULL || wz_machine_init(&machine, wz_machine_profile_48k_pal()) !=
                             WZ_RESULT_OK) {
        free(image);
        return false;
    }
    set_v1_header(image);
    image[12u] = (wz_byte_t)(image[12u] | 0x20u);
    while (remaining != 0u) {
        const wz_byte_t count = (wz_byte_t)(remaining > 255u ? 255u : remaining);
        image[offset++] = 0xedu;
        image[offset++] = 0xedu;
        image[offset++] = count;
        image[offset++] = (wz_byte_t)(remaining ^ 0x5au);
        remaining -= count;
    }
    image[offset++] = 0u;
    image[offset++] = 0xedu;
    image[offset++] = 0xedu;
    image[offset++] = 0u;
    ok = load_into_machine(image, offset, wz_snapshot_state_load_z80_v1, &machine) &&
         machine.memory[0x4000u] == 0x5au &&
         machine.memory[0x40ffu] == 0x5au && machine.memory[0xffffu] == 0x9au;
    wz_machine_destroy(&machine);
    free(image);
    return ok;
}

static bool verify_v2_writer_round_trip(void)
{
    wz_byte_t* image = (wz_byte_t*)malloc(WZ_Z80_V2_LENGTH);
    wz_machine_t source;
    wz_machine_t restored;
    wz_snapshot_state_t snapshot;
    bool ok = false;
    if (image == NULL || wz_machine_init(&source, wz_machine_profile_48k_pal()) !=
                              WZ_RESULT_OK ||
        wz_machine_init(&restored, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(image);
        return false;
    }
    source.cpu.main.a = 0xc3u;
    source.cpu.main.f = 0x52u;
    source.cpu.program_counter = 0x4567u;
    source.cpu.stack_pointer = 0x89abu;
    source.cpu.r = 0x91u;
    source.border_color = 6u;
    source.memory[0x4000u] = 0x11u;
    source.memory[0x7fffu] = 0x22u;
    source.memory[0x8000u] = 0x33u;
    source.memory[0xbfffu] = 0x44u;
    source.memory[0xc000u] = 0x55u;
    source.memory[0xffffu] = 0x66u;
    ok = wz_state_save_z80_v2_48k(&source, image, WZ_Z80_V2_LENGTH) ==
             WZ_RESULT_OK &&
         wz_snapshot_state_load_z80_v2(&snapshot, image, WZ_Z80_V2_LENGTH) ==
             WZ_RESULT_OK &&
         wz_state_deserialize_machine(&restored, wz_snapshot_state_data(&snapshot),
                                      wz_snapshot_state_length(&snapshot)) ==
             WZ_RESULT_OK &&
         restored.cpu.main.a == source.cpu.main.a &&
         restored.cpu.main.f == source.cpu.main.f &&
         restored.cpu.program_counter == source.cpu.program_counter &&
         restored.cpu.stack_pointer == source.cpu.stack_pointer &&
         restored.cpu.r == source.cpu.r && restored.border_color == 6u &&
         restored.memory[0x4000u] == 0x11u && restored.memory[0x7fffu] == 0x22u &&
         restored.memory[0x8000u] == 0x33u && restored.memory[0xbfffu] == 0x44u &&
         restored.memory[0xc000u] == 0x55u && restored.memory[0xffffu] == 0x66u;
    wz_machine_destroy(&source);
    wz_machine_destroy(&restored);
    free(image);
    return ok;
}

int main(void)
{
    if (!verify_v1_uncompressed()) {
        fputs("FAIL Z80 v1 uncompressed load\n", stderr);
        return EXIT_FAILURE;
    }
    if (!verify_v1_compressed()) {
        fputs("FAIL Z80 v1 compressed load\n", stderr);
        return EXIT_FAILURE;
    }
    if (!verify_v2_writer_round_trip()) {
        fputs("FAIL Z80 v2 writer/load round trip\n", stderr);
        return EXIT_FAILURE;
    }
    puts("snapshot-historical-contract cases=3 status=pass");
    return EXIT_SUCCESS;
}
