/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_networking_commands.h"
#include "app/wz_ui_layout.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "UI networking contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_machine_t machine;
    wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE] = {0};
    wz_networking_command_context_t context;
    wz_command_registry_t registry;
    wz_command_metadata_t storage[1];
    wz_command_result_t result;
    wz_ui_layout_state_t ui;
    const wz_ui_networking_option_t* option;
    const wz_command_metadata_t* command;
    const uint8_t interface1 = (uint8_t)WZ_NETWORKING_INTERFACE1;
    const char* reason = NULL;
    unsigned cases = 0u;

    (void)memset(&machine, 0, sizeof(machine));
    (void)memset(&context, 0, sizeof(context));
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_load_interface1_rom(
                &machine, interface1_rom, sizeof(interface1_rom),
                WZ_INTERFACE1_ROM_OLD) == WZ_RESULT_OK);
    context.machine = &machine;
    REQUIRE(wz_command_registry_init(&registry, storage, 1u) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_bind_owner_thread(&registry) == WZ_RESULT_OK);
    REQUIRE(wz_networking_commands_register(&registry, &context) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_finalize(&registry) == WZ_RESULT_OK);
    REQUIRE(wz_ui_layout_networking_option_count() == 3u);
    ++cases;

    option = wz_ui_layout_networking_option_at(0u);
    REQUIRE(option != NULL && option->mode == WZ_NETWORKING_NONE &&
            strcmp(option->label, "None") == 0 && option->available &&
            strcmp(option->command_id, WZ_NETWORKING_COMMAND_ID) == 0);
    option = wz_ui_layout_networking_option_at(1u);
    REQUIRE(option != NULL && option->mode == WZ_NETWORKING_INTERFACE1 &&
            strcmp(option->label, "Interface-1") == 0 && option->available &&
            strcmp(option->command_id, WZ_NETWORKING_COMMAND_ID) == 0);
    option = wz_ui_layout_networking_option_at(2u);
    REQUIRE(option != NULL && option->mode == WZ_NETWORKING_EAR_MIC &&
            strcmp(option->label, "Ear+Mic") == 0 && !option->available &&
            strcmp(option->command_id, WZ_NETWORKING_COMMAND_ID) == 0 &&
            strcmp(option->disabled_reason,
                   WZ_UI_NETWORKING_EAR_MIC_DISABLED_REASON) == 0);
    REQUIRE(wz_ui_layout_networking_option_at(3u) == NULL);
    cases += 3u;

    command = wz_command_registry_find(&registry, WZ_NETWORKING_COMMAND_ID);
    REQUIRE(command != NULL && command->affects_machine_state &&
            command->recordable &&
            strcmp(command->parameter_acquisition,
                   "networking-radio-group") == 0);
    REQUIRE(wz_command_registry_remote_permission(
                &registry, WZ_NETWORKING_COMMAND_ID,
                (wz_command_arguments_t){&interface1, sizeof(interface1)}) ==
            WZ_COMMAND_REMOTE_SAFE);
    cases += 2u;

    wz_ui_layout_state_init(&ui);
    REQUIRE(wz_ui_layout_select_networking(&ui, WZ_NETWORKING_INTERFACE1));
    REQUIRE(ui.networking_selection == WZ_NETWORKING_INTERFACE1 &&
            strcmp(ui.networking_mode, "Interface-1") == 0);
    machine.master_tick = 91u;
    machine.memory[0x4000u] = 0xa5u;
    REQUIRE(wz_ui_layout_activate_networking(
                &registry, WZ_NETWORKING_INTERFACE1, &result) == WZ_RESULT_OK);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_INTERFACE1 &&
            machine.master_tick == 0u && machine.memory[0x4000u] == 0u &&
            machine.has_interface1_rom != 0u);
    cases += 3u;

    machine.master_tick = 37u;
    machine.memory[0x4001u] = 0x5au;
    REQUIRE(wz_ui_layout_select_networking(&ui, WZ_NETWORKING_NONE));
    REQUIRE(wz_ui_layout_activate_networking(
                &registry, WZ_NETWORKING_NONE, &result) == WZ_RESULT_OK);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_NONE &&
            machine.master_tick == 0u && machine.memory[0x4001u] == 0u);
    cases += 3u;

    machine.memory[0x4002u] = 0x3cu;
    REQUIRE(!wz_ui_layout_select_networking(&ui, WZ_NETWORKING_EAR_MIC));
    REQUIRE(wz_ui_layout_activate_networking(
                &registry, WZ_NETWORKING_EAR_MIC, &result) ==
            WZ_RESULT_UNSUPPORTED_OPERATION);
    REQUIRE(result.status == WZ_COMMAND_RESULT_UNAVAILABLE &&
            strcmp(result.reason,
                   WZ_UI_NETWORKING_EAR_MIC_DISABLED_REASON) == 0 &&
            machine.networking_mode == WZ_NETWORKING_NONE &&
            machine.memory[0x4002u] == 0x3cu);
    REQUIRE(wz_command_registry_state(&registry, WZ_NETWORKING_COMMAND_ID,
                                     &reason) == WZ_COMMAND_AVAILABLE);
    cases += 4u;

    wz_machine_destroy(&machine);
    (void)printf("ui-networking-mode cases=%u status=pass\n", cases);
    return 0;
}
