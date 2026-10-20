/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"
#include "core/wz_trace.h"
#include "diagnostics/wz_trace_file.h"

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Interface 1 trace fixture failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

static wz_result_t request_io(wz_machine_t* machine, wz_bus_cycle_t cycle,
                              wz_word_t address, wz_byte_t value,
                              wz_master_tick_t tick, wz_byte_t* result)
{
    wz_bus_request_t request;
    wz_bus_request_init(&request, cycle, tick, address, value, 1u);
    if (wz_machine_bus_request(machine, &request) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (result != NULL) {
        *result = request.value;
    }
    return WZ_RESULT_OK;
}

int main(int argc, char** argv)
{
    wz_machine_t machine = {0};
    wz_trace_file_t trace_file = {0};
    wz_trace_sink_t trace_sink = {0};
    wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE] = {0};
    wz_byte_t image_bytes[WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    wz_mdr_image_t image = {0};
    wz_byte_t value = 0u;
    const wz_dword_t event_mask =
        UINT32_C(1) << WZ_TRACE_INTERFACE1_IO;
    int result = 1;

    REQUIRE(argc == 2);
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_load_interface1_rom(&machine, interface1_rom,
            sizeof(interface1_rom), WZ_INTERFACE1_ROM_NEW) == WZ_RESULT_OK);
    REQUIRE(wz_machine_set_networking_mode(&machine,
            WZ_NETWORKING_INTERFACE1) == WZ_RESULT_OK);
    machine.hardware_io_decode_enabled = 1u;
    for (size_t index = 0u; index < sizeof(image_bytes); ++index) {
        image_bytes[index] = (wz_byte_t)(index & 0xffu);
    }
    REQUIRE(wz_mdr_image_init(&image, image_bytes, sizeof(image_bytes)) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_mount_microdrive(&machine, 1u, &image) ==
            WZ_RESULT_OK);
    REQUIRE(wz_trace_file_create(&trace_file, argv[1], 1u,
            (wz_dword_t)WZ_MACHINE_48K_PAL, 0u, event_mask) == WZ_RESULT_OK);
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    wz_machine_set_timing_trace(&machine, &trace_sink);

    REQUIRE(request_io(&machine, WZ_BUS_IO_WRITE, 0x00efu, 0x06u, 10u,
                       NULL) == WZ_RESULT_OK);
    REQUIRE(request_io(&machine, WZ_BUS_IO_WRITE, 0xa0efu, 0x04u, 20u,
                       NULL) == WZ_RESULT_OK);
    REQUIRE(machine.interface1_active_motor == 0u);
    REQUIRE(request_io(&machine, WZ_BUS_IO_READ, 0x80e7u, 0u, 30u,
                       &value) == WZ_RESULT_OK);
    REQUIRE(value == image_bytes[WZ_MDR_HEADER_OFFSET]);
    REQUIRE(request_io(&machine, WZ_BUS_IO_WRITE, 0x00f7u, 0x01u, 40u,
                       NULL) == WZ_RESULT_OK);
    REQUIRE(request_io(&machine, WZ_BUS_IO_READ, 0x80f7u, 0u, 50u,
                       &value) == WZ_RESULT_OK);
    REQUIRE(value == 0xffu);

    wz_machine_set_timing_trace(&machine, NULL);
    REQUIRE(wz_trace_file_freeze(&trace_file) == WZ_RESULT_OK);
    result = 0;

cleanup:
    wz_machine_set_timing_trace(&machine, NULL);
    wz_trace_file_close(&trace_file);
    wz_machine_destroy(&machine);
    return result;
}
