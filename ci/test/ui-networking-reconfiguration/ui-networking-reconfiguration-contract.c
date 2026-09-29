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

static unsigned cases;

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Networking reconfiguration contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        return 1; \
    } \
    ++cases; \
} while (0)

static int seed_emulated_state(wz_machine_t* machine,
                               int* observer_context,
                               int* input_context,
                               int* source_context)
{
    static const wz_tape_segment_t tape_segments[] = {{32u, 1u, 0u}};

    wz_machine_memory_write(machine, 0x4000u, 0xa5u);
    machine->cpu.program_counter = 0x2345u;
    machine->master_tick = 1234u;
    REQUIRE(wz_machine_mount_tape(
                machine, tape_segments,
                sizeof(tape_segments) / sizeof(tape_segments[0])) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_set_keyboard_key(machine, 0u, 1u, true) == WZ_RESULT_OK);
    REQUIRE(wz_machine_set_kempston_control(
                machine, WZ_KEMPSTON_RIGHT, true) == WZ_RESULT_OK);
    machine->bus_observer.context = observer_context;
    machine->bus_input.context = input_context;
    machine->bus_data_source.context = source_context;
    REQUIRE(machine->tape_mounted != 0u);
    REQUIRE(wz_machine_memory_read(machine, 0x4000u) == 0xa5u);
    REQUIRE(machine->cpu.program_counter == 0x2345u &&
            machine->master_tick == 1234u);
    return 0;
}

int main(void)
{
    wz_machine_t machine;
    wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE] = {0};
    wz_networking_command_context_t context;
    wz_command_registry_t registry;
    wz_command_metadata_t storage[1];
    wz_command_result_t result;
    wz_ui_layout_state_t ui;
    int observer_context = 1;
    int input_context = 2;
    int source_context = 3;

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

    wz_ui_layout_state_init(&ui);
    ui.paused = true;
    REQUIRE(seed_emulated_state(&machine, &observer_context, &input_context,
                                &source_context) == 0);
    REQUIRE(wz_ui_layout_select_networking(
                &ui, WZ_NETWORKING_INTERFACE1));
    REQUIRE(wz_ui_layout_activate_networking(
                &registry, WZ_NETWORKING_INTERFACE1, &result) == WZ_RESULT_OK);
    REQUIRE(ui.paused && ui.networking_selection == WZ_NETWORKING_INTERFACE1);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_INTERFACE1);
    REQUIRE(machine.profile == wz_machine_profile_48k_pal());
    REQUIRE(machine.has_interface1_rom != 0u);
    REQUIRE(machine.cpu.program_counter == 0u && machine.master_tick == 0u);
    REQUIRE(wz_machine_memory_read(&machine, 0x4000u) == 0u);
    REQUIRE(machine.tape_mounted == 0u && machine.tape.segments == NULL);
    REQUIRE(machine.keyboard_rows[0] == 0x1fu);
    REQUIRE(!machine.kempston.pressed[WZ_KEMPSTON_RIGHT]);
    REQUIRE(machine.bus_observer.context == NULL &&
            machine.bus_input.context == NULL &&
            machine.bus_data_source.context == NULL);

    ui.paused = false;
    REQUIRE(seed_emulated_state(&machine, &observer_context, &input_context,
                                &source_context) == 0);
    REQUIRE(wz_ui_layout_select_networking(&ui, WZ_NETWORKING_NONE));
    REQUIRE(wz_ui_layout_activate_networking(
                &registry, WZ_NETWORKING_NONE, &result) == WZ_RESULT_OK);
    REQUIRE(!ui.paused && ui.networking_selection == WZ_NETWORKING_NONE);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_NONE);
    REQUIRE(machine.has_interface1_rom != 0u);
    REQUIRE(machine.cpu.program_counter == 0u && machine.master_tick == 0u);
    REQUIRE(wz_machine_memory_read(&machine, 0x4000u) == 0u);
    REQUIRE(machine.tape_mounted == 0u && machine.tape.segments == NULL);
    REQUIRE(machine.bus_observer.context == NULL &&
            machine.bus_input.context == NULL &&
            machine.bus_data_source.context == NULL);

    wz_machine_destroy(&machine);
    (void)printf("ui-networking-reconfiguration cases=%u status=pass\n", cases);
    return 0;
}