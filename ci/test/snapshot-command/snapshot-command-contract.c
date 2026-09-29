/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_file_open_run.h"
#include "app/wz_snapshot_save_workflow.h"
#include "core/wz_machine.h"
#include "core/wz_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "failed: %s\n", #expression); \
        return false; \
    } \
} while (0)

static bool verify_native_format_route(void)
{
    wz_open_run_route_t route = WZ_OPEN_RUN_UNSUPPORTED;
    REQUIRE(wz_file_open_run_route("saves/current.SNA", &route) ==
            WZ_OPEN_RUN_OK);
    REQUIRE(route == WZ_OPEN_RUN_SNAPSHOT);
    REQUIRE(wz_file_open_run_route("saves/current.Z80", &route) ==
            WZ_OPEN_RUN_OK);
    REQUIRE(route == WZ_OPEN_RUN_SNAPSHOT);
    return true;
}

static bool verify_save_destination_semantics(void)
{
    wz_snapshot_save_workflow_t workflow;
    wz_snapshot_save_workflow_init(&workflow);
    REQUIRE(wz_snapshot_save(&workflow) == WZ_SNAPSHOT_SAVE_NEEDS_DESTINATION);
    REQUIRE(wz_snapshot_save_current_destination(&workflow) == NULL);
    REQUIRE(wz_snapshot_save_as(&workflow, "saves/first.z80") ==
            WZ_SNAPSHOT_SAVE_OK);
    REQUIRE(strcmp(wz_snapshot_save_current_destination(&workflow),
                   "saves/first.z80") == 0);
    REQUIRE(wz_snapshot_save(&workflow) == WZ_SNAPSHOT_SAVE_OK);
    REQUIRE(wz_snapshot_save_as(&workflow, "saves/second.sna") ==
            WZ_SNAPSHOT_SAVE_OK);
    REQUIRE(strcmp(wz_snapshot_save_current_destination(&workflow),
                   "saves/second.sna") == 0);
    REQUIRE(wz_snapshot_save_as(&workflow, "") ==
            WZ_SNAPSHOT_SAVE_INVALID_DESTINATION);
    REQUIRE(strcmp(wz_snapshot_save_current_destination(&workflow),
                   "saves/second.sna") == 0);
    return true;
}

static bool verify_native_round_trip_and_rejection(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    wz_snapshot_state_t saved;
    wz_snapshot_state_t loaded;
    wz_byte_t* historical;
    wz_qword_t initial_hash;
    wz_qword_t restored_hash;
    wz_qword_t unchanged_hash;
    bool ok = false;
    REQUIRE(machine != NULL);
    if (wz_machine_init(machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        free(machine);
        return false;
    }
    machine->cpu.main.a = 0x5au;
    if (wz_state_hash_machine(machine, &initial_hash) != WZ_RESULT_OK ||
        wz_snapshot_state_capture(&saved, machine) != WZ_RESULT_OK) goto cleanup;

    historical = (wz_byte_t*)malloc(WZ_Z80_V2_LENGTH);
    if (historical == NULL) goto cleanup;
    if (wz_state_validate_historical_representability(
            machine, WZ_HISTORICAL_FORMAT_Z80) != WZ_RESULT_OK ||
        wz_state_save_z80_v2_48k(machine, historical, WZ_Z80_V2_LENGTH) !=
            WZ_RESULT_OK ||
        wz_snapshot_state_load_z80_v2(&loaded, historical,
                                      WZ_Z80_V2_LENGTH) != WZ_RESULT_OK) {
        free(historical);
        goto cleanup;
    }
    free(historical);

    machine->cpu.main.a = 0xa5u;
    if (wz_state_deserialize_machine(machine, wz_snapshot_state_data(&loaded),
                                     wz_snapshot_state_length(&loaded)) !=
            WZ_RESULT_OK || machine->cpu.main.a != 0x5au) goto cleanup;

    machine->cpu.main.a = 0x5au;
    if (wz_snapshot_state_load(&loaded, wz_snapshot_state_data(&saved),
                               wz_snapshot_state_length(&saved)) != WZ_RESULT_OK) {
        goto cleanup;
    }
    machine->cpu.main.a = 0xa5u;
    if (wz_state_deserialize_machine(machine, wz_snapshot_state_data(&loaded),
                                     wz_snapshot_state_length(&loaded)) !=
            WZ_RESULT_OK ||
        wz_state_hash_machine(machine, &restored_hash) != WZ_RESULT_OK ||
        restored_hash != initial_hash) goto cleanup;

    if (wz_state_hash_machine(machine, &unchanged_hash) != WZ_RESULT_OK) {
        goto cleanup;
    }
    if (wz_snapshot_state_load(&loaded, wz_snapshot_state_data(&saved),
                               wz_snapshot_state_length(&saved) - 1u) ==
        WZ_RESULT_OK ||
        wz_state_hash_machine(machine, &restored_hash) != WZ_RESULT_OK ||
        restored_hash != unchanged_hash) goto cleanup;
    ok = true;
cleanup:
    wz_machine_destroy(machine);
    free(machine);
    REQUIRE(ok);
    return true;
}

static bool verify_128k_writers(void)
{
    wz_machine_t* machine = (wz_machine_t*)calloc(1u, sizeof(*machine));
    const size_t capacity = WZ_SNA_128K_LENGTH > WZ_Z80_128K_V2_LENGTH ?
        WZ_SNA_128K_LENGTH : WZ_Z80_128K_V2_LENGTH;
    wz_byte_t* image = (wz_byte_t*)malloc(capacity);
    bool ok = false;
    REQUIRE(machine != NULL && image != NULL);
    if (wz_machine_init(machine, wz_machine_profile_128k_pal()) != WZ_RESULT_OK) {
        free(image);
        free(machine);
        return false;
    }
    if (wz_state_validate_historical_representability(
            machine, WZ_HISTORICAL_FORMAT_SNA) != WZ_RESULT_OK ||
        wz_state_save_sna_128k(machine, image, WZ_SNA_128K_LENGTH) !=
            WZ_RESULT_OK ||
        wz_state_validate_historical_representability(
            machine, WZ_HISTORICAL_FORMAT_Z80) != WZ_RESULT_OK ||
        wz_state_save_z80_v2_128k(machine, image, WZ_Z80_128K_V2_LENGTH) !=
            WZ_RESULT_OK) goto cleanup;
    ok = true;
cleanup:
    wz_machine_destroy(machine);
    free(image);
    free(machine);
    REQUIRE(ok);
    return true;
}

int main(void)
{
    unsigned passed = 0u;
    if (verify_native_format_route()) ++passed;
    if (verify_save_destination_semantics()) ++passed;
    if (verify_native_round_trip_and_rejection()) ++passed;
    if (verify_128k_writers()) ++passed;
    if (passed != 4u) return 1;
    printf("snapshot-command-contract cases=%u status=pass\n", passed);
    return 0;
}
