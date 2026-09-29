/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/wz_bus.h"
#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"

static bool check_delay(const wz_machine_t* machine,
                        wz_bus_cycle_t cycle,
                        wz_word_t address,
                        wz_dword_t tstate,
                        wz_byte_t t_states,
                        wz_byte_t expected)
{
    wz_master_tick_t tick = (wz_master_tick_t)tstate * 2u;
    return wz_machine_contention_delay(machine, cycle, address, tick,
                                       t_states) == expected;
}

static bool check_delay_after_long_runtime(const wz_machine_t* machine)
{
    wz_master_tick_t tstate = (wz_master_tick_t)UINT32_MAX + 1u + 14435u;
    return wz_machine_contention_delay(machine, WZ_BUS_MEMORY_READ, 0x4000u,
                                       tstate * 2u, 3u) == 0u;
}

static bool check_io_bus_advance(const wz_machine_profile_t* profile,
                                 wz_word_t port,
                                 wz_byte_t expected_delay)
{
    wz_machine_t machine;
    wz_bus_request_t request;
    wz_master_tick_t start = (wz_master_tick_t)14335u * 2u;

    memset(&machine, 0, sizeof(machine));
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        return false;
    }
    machine.master_tick = start;
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, start, port, 0x5au, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        request.contention_delay != expected_delay ||
        request.master_tick != start + (wz_master_tick_t)expected_delay * 2u ||
        machine.master_tick != start + (wz_master_tick_t)expected_delay * 2u) {
        wz_machine_destroy(&machine);
        return false;
    }
    wz_machine_destroy(&machine);
    return true;
}

static bool check_oversized_io_request_rejected(
    const wz_machine_profile_t* profile)
{
    wz_machine_t machine;
    wz_bus_request_t request;
    wz_master_tick_t start = (wz_master_tick_t)14335u * 2u;

    memset(&machine, 0, sizeof(machine));
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        return false;
    }
    machine.master_tick = start;
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, start, 0x40ffu, 0x5au, 5u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_INVALID_ARGUMENT ||
        machine.master_tick != start) {
        wz_machine_destroy(&machine);
        return false;
    }
    wz_machine_destroy(&machine);
    return true;
}

static bool check_48k_contract(void)
{
    static const wz_byte_t delay_pattern[8] = {6u, 5u, 4u, 3u, 2u, 1u, 0u, 0u};
    const wz_machine_profile_t* profile = wz_machine_profile_48k_pal();
    wz_machine_t machine;

    memset(&machine, 0, sizeof(machine));
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        return false;
    }
    for (wz_dword_t phase = 0u; phase < 8u; ++phase) {
        if (!check_delay(&machine, WZ_BUS_MEMORY_READ, 0x4000u,
                         14335u + phase, 3u, delay_pattern[phase])) {
            wz_machine_destroy(&machine);
            return false;
        }
    }
    if (!check_delay(&machine, WZ_BUS_MEMORY_WRITE, 0x7fffu,
                     14335u, 3u, 6u) ||
        !check_delay(&machine, WZ_BUS_MEMORY_READ, 0x3fffu,
                     14335u, 3u, 0u) ||
        !check_delay(&machine, WZ_BUS_MEMORY_READ, 0x8000u,
                     14335u, 3u, 0u) ||
        !check_delay(&machine, WZ_BUS_MEMORY_READ, 0x4000u,
                     14463u, 3u, 0u) ||
        !check_delay_after_long_runtime(&machine) ||
        !check_delay(&machine, WZ_BUS_IO_READ, 0x00feu,
                     14335u, 4u, 5u) ||
        !check_delay(&machine, WZ_BUS_IO_WRITE, 0x40feu,
                     14335u, 4u, 6u) ||
        !check_delay(&machine, WZ_BUS_IO_WRITE, 0x40ffu,
                     14335u, 4u, 12u) ||
        !check_delay(&machine, WZ_BUS_IO_WRITE, 0x00ffu,
                     14335u, 4u, 0u) ||
        !check_delay(&machine, WZ_BUS_IO_WRITE, 0x40ffu,
                     14335u, 5u, 0u) ||
        !check_io_bus_advance(profile, 0x40ffu, 12u) ||
        !check_io_bus_advance(profile, 0x00feu, 5u) ||
        !check_oversized_io_request_rejected(profile)) {
        wz_machine_destroy(&machine);
        return false;
    }
    wz_machine_destroy(&machine);
    return true;
}

static bool check_128k_contract(void)
{
    const wz_machine_profile_t* profile = wz_machine_profile_128k_pal();
    wz_machine_t machine;
    static const wz_byte_t contended_banks[4u] = {1u, 3u, 5u, 7u};

    memset(&machine, 0, sizeof(machine));
    if (wz_machine_init(&machine, profile) != WZ_RESULT_OK) {
        return false;
    }
    if (!check_delay(&machine, WZ_BUS_MEMORY_READ, 0x4000u,
                     14361u, 3u, 6u) ||
        !check_delay(&machine, WZ_BUS_MEMORY_READ, 0x8000u,
                     14361u, 3u, 0u) ||
        !check_delay(&machine, WZ_BUS_MEMORY_READ, 0xc000u,
                     14361u, 3u, 0u) ||
        !check_delay(&machine, WZ_BUS_IO_READ, 0x00feu,
                     14361u, 4u, 5u) ||
        !check_delay(&machine, WZ_BUS_MEMORY_READ, 0x4000u,
                     14361u + 128u, 3u, 0u) ||
        !check_delay(&machine, WZ_BUS_MEMORY_READ, 0x4000u,
                     14361u + 228u, 3u, 6u) ||
        !check_delay(&machine, WZ_BUS_MEMORY_READ, 0x4000u,
                     70908u + 14361u, 3u, 6u)) {
        wz_machine_destroy(&machine);
        return false;
    }
    for (size_t index = 0u; index < sizeof(contended_banks); ++index) {
        machine.paging_7ffd = contended_banks[index];
        if (!check_delay(&machine, WZ_BUS_MEMORY_READ, 0xc000u,
                         14361u, 3u, 6u)) {
            wz_machine_destroy(&machine);
            return false;
        }
    }
    machine.paging_7ffd = 2u;
    if (!check_delay(&machine, WZ_BUS_MEMORY_READ, 0xc000u,
                     14361u, 3u, 0u)) {
        wz_machine_destroy(&machine);
        return false;
    }
    wz_machine_destroy(&machine);
    return true;
}

int main(void)
{
    if (!check_48k_contract()) {
        fputs("48K PAL contention contract failed.\n", stderr);
        return 1;
    }
    if (!check_128k_contract()) {
        fputs("128K PAL bank-dependent contention contract failed.\n", stderr);
        return 1;
    }
    puts("48K and 128K PAL contention contracts passed.");
    return 0;
}
