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
        fprintf(stderr, "reset-retention regression failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_machine_t machine;
    wz_tape_segment_t tape_segments[] = {{32u, 1u}, {64u, 0u}};
    wz_byte_t image_bytes[WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    wz_mdr_image_t image;
    wz_trace_sink_t trace;
    int observer_context = 1;
    int input_context = 2;
    int source_context = 3;

    memset(&machine, 0, sizeof(machine));
    memset(image_bytes, 0x5au, sizeof(image_bytes));
    wz_trace_sink_init(&trace, NULL, NULL);
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_mount_tape(&machine, tape_segments, 2u) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_image_init(&image, image_bytes, sizeof(image_bytes)) ==
            WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_mount(&machine.microdrive, &image) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_select_motor(&machine.microdrive, 0u) ==
            WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_set_write_mode(&machine.microdrive, 1u) ==
            WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_write(&machine.microdrive, 0xa5u) == WZ_RESULT_OK);
    REQUIRE(wz_machine_set_printer_mode(&machine, WZ_PRINTER_MODE_HP) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_printer_write(&machine, 0x00fbu, 0x00u, 17u) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_printer_write(&machine, 0x00fbu, 0x80u, 18u) ==
            WZ_RESULT_OK);

    machine.bus_observer.context = &observer_context;
    machine.bus_input.context = &input_context;
    machine.bus_data_source.context = &source_context;
    machine.timing_trace = &trace;
    machine.networking_mode = WZ_NETWORKING_INTERFACE1;
    machine.master_tick = 1234u;
    machine.cpu.program_counter = 0x1234u;

    REQUIRE(wz_machine_reset(&machine) == WZ_RESULT_OK);
    REQUIRE(machine.master_tick == 0u);
    REQUIRE(machine.cpu.program_counter == 0u);
    REQUIRE(machine.tape_mounted == 1u);
    REQUIRE(machine.tape.segments == tape_segments);
    REQUIRE(machine.tape_state.tape == &machine.tape);
    REQUIRE(wz_tape_state_ear_level(&machine.tape_state) == 1u);
    REQUIRE(machine.microdrive.image == &image);
    REQUIRE(machine.microdrive.image_present == 1u);
    REQUIRE(machine.microdrive.dirty == 1u);
    REQUIRE(machine.microdrive.buffer[WZ_MDR_HEADER_OFFSET] == 0xa5u);
    REQUIRE(wz_printer_mode(&machine.printer) == WZ_PRINTER_MODE_HP);
    REQUIRE(machine.printer.motor_on == 1u);
    REQUIRE(machine.printer.hp_bits == 1u);
    REQUIRE(machine.printer.hp_accumulator == 1u);
    REQUIRE(machine.bus_observer.context == &observer_context);
    REQUIRE(machine.bus_input.context == &input_context);
    REQUIRE(machine.bus_data_source.context == &source_context);
    REQUIRE(machine.timing_trace == &trace);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_INTERFACE1);

    wz_machine_destroy(&machine);
    puts("reset-retention regression passed");
    return 0;
}
