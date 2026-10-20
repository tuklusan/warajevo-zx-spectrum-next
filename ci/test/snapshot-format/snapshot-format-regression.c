/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/wz_machine.h"
#include "core/wz_state.h"

#define V3_HEADER_SIZE 86u
#define PAGE_SIZE 16384u
#define PAGE_RECORD_SIZE (3u + PAGE_SIZE)
#define SNAPSHOT_SIZE (V3_HEADER_SIZE + 8u * PAGE_RECORD_SIZE)

static void write_u16(wz_byte_t* data, size_t offset, wz_word_t value)
{
    data[offset] = (wz_byte_t)(value & 0xffu);
    data[offset + 1u] = (wz_byte_t)(value >> 8u);
}

static int make_v3_128k(wz_byte_t* data)
{
    if (data == NULL) return 0;
    memset(data, 0, SNAPSHOT_SIZE);
    write_u16(data, 30u, 54u);
    write_u16(data, 32u, 0x8123u);
    data[34u] = 4u;
    data[35u] = 0x25u;
    for (size_t index = 0u; index < 16u; ++index) {
        data[37u + index] = (wz_byte_t)(index + 1u);
    }
    data[53u] = 9u;
    for (size_t page = 0u; page < 8u; ++page) {
        const size_t offset = V3_HEADER_SIZE + page * PAGE_RECORD_SIZE;
        write_u16(data, offset, 0xffffu);
        data[offset + 2u] = (wz_byte_t)(page + 3u);
        memset(data + offset + 3u, (int)(0x30u + page), PAGE_SIZE);
    }
    return 1;
}

static int check_v3_128k_load(void)
{
    wz_byte_t* input = (wz_byte_t*)malloc(SNAPSHOT_SIZE);
    wz_snapshot_state_t snapshot;
    wz_machine_t machine;
    int passed = 0;

    memset(&machine, 0, sizeof(machine));
    if (!make_v3_128k(input)) goto done;
    wz_snapshot_state_init(&snapshot);
    if (wz_snapshot_state_load_z80_v3(&snapshot, input, SNAPSHOT_SIZE) !=
        WZ_RESULT_OK ||
        wz_machine_init(&machine, wz_machine_profile_128k_pal()) !=
        WZ_RESULT_OK ||
        wz_state_deserialize_machine(&machine, wz_snapshot_state_data(&snapshot),
                                     wz_snapshot_state_length(&snapshot)) !=
        WZ_RESULT_OK) goto done;
    if (machine.profile->kind != WZ_MACHINE_128K_PAL ||
        machine.cpu.program_counter != 0x8123u ||
        machine.paging_7ffd != 0x25u || machine.ay.selected_register != 9u) {
        goto done;
    }
    for (size_t page = 0u; page < 8u; ++page) {
        if (machine.ram_128k[page * PAGE_SIZE] != (wz_byte_t)(0x30u + page) ||
            machine.ram_128k[(page + 1u) * PAGE_SIZE - 1u] !=
                (wz_byte_t)(0x30u + page)) goto done;
    }
    for (size_t index = 0u; index < 16u; ++index) {
        if (machine.ay.registers[index] != (wz_byte_t)(index + 1u)) goto done;
    }
    passed = 1;
done:
    wz_machine_destroy(&machine);
    free(input);
    return passed;
}

static int check_v3_unsupported_mode_and_incomplete_pages(void)
{
    wz_byte_t* input = (wz_byte_t*)malloc(SNAPSHOT_SIZE);
    wz_snapshot_state_t snapshot;
    int passed = 0;

    if (!make_v3_128k(input)) goto done;
    wz_snapshot_state_init(&snapshot);
    input[34u] = 3u;
    if (wz_snapshot_state_load_z80_v3(&snapshot, input, SNAPSHOT_SIZE) !=
        WZ_RESULT_PARSE_ERROR || wz_snapshot_state_length(&snapshot) != 0u) {
        goto done;
    }
    if (!make_v3_128k(input)) goto done;
    wz_snapshot_state_init(&snapshot);
    if (wz_snapshot_state_load_z80_v3(&snapshot, input,
            SNAPSHOT_SIZE - PAGE_RECORD_SIZE) != WZ_RESULT_PARSE_ERROR ||
        wz_snapshot_state_length(&snapshot) != 0u) goto done;
    passed = 1;
done:
    free(input);
    return passed;
}

int main(void)
{
    if (!check_v3_128k_load()) {
        fputs("FAIL Z80 v3 base-128K page, paging, and AY load\n", stderr);
        return EXIT_FAILURE;
    }
    puts("PASS Z80 v3 base-128K page, paging, and AY load");
    if (!check_v3_unsupported_mode_and_incomplete_pages()) {
        fputs("FAIL Z80 v3 rejects unsupported modes and incomplete page sets\n",
              stderr);
        return EXIT_FAILURE;
    }
    puts("PASS Z80 v3 rejects unsupported modes and incomplete page sets");
    return EXIT_SUCCESS;
}
