/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_snapshot_inspector.h"
#include "core/wz_machine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "failed: %s\n", #expression); \
        return false; \
    } \
} while (0)

static bool verify_48k_inspection(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_snapshot_inspector_t inspector;
    const wz_snapshot_inspector_t* metadata;
    const wz_debugger_snapshot_t* snapshot;
    char output[1024];
    bool ok = false;
    if (machine == NULL) return false;
    if (wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return false;
    }
    machine->cpu.main.a = 0x5au;
    machine->cpu.program_counter = 0x4321u;
    machine->ay.selected_register = 13u;
    machine->ay.registers[13u] = 0x0bu;
    wz_snapshot_inspector_init(&inspector);
    if (wz_snapshot_inspector_open(&inspector, machine) !=
            WZ_SNAPSHOT_INSPECTOR_OK) goto cleanup;
    metadata = wz_snapshot_inspector_metadata(&inspector);
    snapshot = wz_snapshot_inspector_machine(&inspector);
    if (metadata == NULL || snapshot == NULL ||
        strcmp(metadata->format_name, "live-machine") != 0 ||
        strcmp(metadata->format_version, "runtime") != 0 ||
        metadata->model_kind != WZ_MACHINE_48K_PAL ||
        (metadata->model_name == NULL || machine->profile->name == NULL ||
         strcmp(metadata->model_name, machine->profile->name) != 0) ||
        metadata->ay_selected_register != 13u ||
        metadata->ay_registers[13u] != 0x0bu ||
        metadata->memory_page_count != 3u ||
        snapshot->cpu.main.a != 0x5au ||
        snapshot->cpu.program_counter != 0x4321u ||
        wz_snapshot_inspector_format(&inspector, output, sizeof(output)) !=
            WZ_RESULT_OK ||
        strstr(output, "Format: live-machine/runtime") == NULL ||
        strstr(output, "Model: ") == NULL ||
        strstr(output, "Registers: A=5A") == NULL ||
        strstr(output, "Paging: ") == NULL ||
        strstr(output, "AY: selected=13") == NULL ||
        strstr(output, "Memory pages: count=3") == NULL ||
        strstr(output, "Warnings: 0") == NULL) goto cleanup;
    ok = true;
cleanup:
    wz_snapshot_inspector_close(&inspector);
    wz_machine_destroy(machine);
    free(machine);
    REQUIRE(ok);
    return true;
}

static bool verify_128k_inspection(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_snapshot_inspector_t inspector;
    const wz_snapshot_inspector_t* metadata;
    const wz_debugger_page_info_t* paging;
    bool ok = false;
    if (machine == NULL) return false;
    if (wz_machine_init(machine, wz_machine_profile_128k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return false;
    }
    wz_snapshot_inspector_init(&inspector);
    if (wz_snapshot_inspector_open(&inspector, machine) !=
            WZ_SNAPSHOT_INSPECTOR_OK) goto cleanup;
    metadata = wz_snapshot_inspector_metadata(&inspector);
    paging = wz_snapshot_inspector_paging(&inspector);
    if (metadata == NULL || paging == NULL ||
        metadata->model_kind != WZ_MACHINE_128K_PAL ||
        (metadata->model_name == NULL || machine->profile->name == NULL ||
         strcmp(metadata->model_name, machine->profile->name) != 0) ||
        metadata->memory_page_count != WZ_128K_RAM_BANK_COUNT ||
        paging->paging_value != wz_machine_128k_paging_value(machine)) {
        goto cleanup;
    }
    for (size_t index = 0u; index < metadata->memory_page_count; ++index) {
        if (metadata->memory_pages[index] == 0u) goto cleanup;
    }
    ok = true;
cleanup:
    wz_snapshot_inspector_close(&inspector);
    wz_machine_destroy(machine);
    free(machine);
    REQUIRE(ok);
    return true;
}

static bool verify_invalid_arguments_and_close(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_snapshot_inspector_t inspector;
    char output[64];
    bool ok = false;
    if (machine == NULL) return false;
    if (wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return false;
    }
    wz_snapshot_inspector_init(&inspector);
    if (wz_snapshot_inspector_open(NULL, machine) !=
            WZ_SNAPSHOT_INSPECTOR_INVALID_ARGUMENT ||
        wz_snapshot_inspector_open(&inspector, NULL) !=
            WZ_SNAPSHOT_INSPECTOR_INVALID_ARGUMENT ||
        wz_snapshot_inspector_machine(&inspector) != NULL ||
        wz_snapshot_inspector_format(&inspector, output, sizeof(output)) !=
            WZ_RESULT_INVALID_ARGUMENT ||
        wz_snapshot_inspector_open(&inspector, machine) !=
            WZ_SNAPSHOT_INSPECTOR_OK) goto cleanup;
    wz_snapshot_inspector_close(&inspector);
    if (wz_snapshot_inspector_is_open(&inspector) ||
        wz_snapshot_inspector_metadata(&inspector) != NULL) goto cleanup;
    ok = true;
cleanup:
    wz_machine_destroy(machine);
    free(machine);
    REQUIRE(ok);
    return true;
}

int main(void)
{
    unsigned passed = 0u;
    if (verify_48k_inspection()) ++passed;
    if (verify_128k_inspection()) ++passed;
    if (verify_invalid_arguments_and_close()) ++passed;
    if (passed != 3u) return 1;
    printf("snapshot-inspector-contract cases=%u status=pass\n", passed);
    return 0;
}
