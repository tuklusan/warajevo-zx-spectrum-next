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
#include <stdlib.h>
#include <string.h>

#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"
#include "core/wz_state.h"

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "microdrive bank contract failed at line %d: %s\n", \
                __LINE__, #condition); \
        goto failure; \
    } \
} while (0)

typedef struct {
    size_t calls;
    size_t slots[WZ_MACHINE_MICRODRIVE_COUNT];
    size_t fail_slot;
} flush_log_t;

static wz_result_t record_flush(size_t slot, size_t sector,
    const wz_byte_t* data, size_t length, void* opaque)
{
    flush_log_t* log = (flush_log_t*)opaque;
    if (log == NULL || slot >= WZ_MACHINE_MICRODRIVE_COUNT ||
        sector >= WZ_MDR_MAX_SECTORS || data == NULL ||
        length != WZ_MDR_SECTOR_SIZE) return WZ_RESULT_INVALID_ARGUMENT;
    ++log->calls;
    ++log->slots[slot];
    return slot == log->fail_slot ? WZ_RESULT_INVALID_STATE : WZ_RESULT_OK;
}

static void select_motor(wz_machine_t* machine, size_t slot,
                         wz_master_tick_t* tick)
{
    for (size_t step = 0u; step < WZ_MACHINE_MICRODRIVE_COUNT; ++step) {
        const wz_byte_t data = step == WZ_MACHINE_MICRODRIVE_COUNT - 1u - slot
            ? 0u : 1u;
        (void)wz_machine_interface1_control_write(
            machine, (wz_byte_t)(0x06u | data), (*tick)++);
        (void)wz_machine_interface1_control_write(
            machine, (wz_byte_t)(0x04u | data), (*tick)++);
    }
}

int main(void)
{
    wz_machine_t machine = {0};
    wz_machine_t restored = {0};
    wz_byte_t image_bytes[WZ_MACHINE_MICRODRIVE_COUNT]
        [WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    wz_mdr_image_t images[WZ_MACHINE_MICRODRIVE_COUNT];
    wz_byte_t* state_bytes = NULL;
    wz_state_writer_t writer;
    flush_log_t flush_log = {0};
    size_t slot;
    wz_master_tick_t tick = 0u;
    int exit_code = 1;

    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_init(&restored, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_microdrive_at(&machine,
                WZ_MACHINE_MICRODRIVE_COUNT) == NULL);
    REQUIRE(wz_machine_mount_microdrive(&machine, 0u, &images[0]) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_machine_eject_microdrive(&machine,
                WZ_MACHINE_MICRODRIVE_COUNT + 1u, false) ==
            WZ_RESULT_INVALID_ARGUMENT);
    for (slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
        memset(image_bytes[slot], (int)(0x30u + slot), sizeof(image_bytes[slot]));
        REQUIRE(wz_mdr_image_init(&images[slot], image_bytes[slot],
                                  sizeof(image_bytes[slot])) == WZ_RESULT_OK);
        REQUIRE(wz_machine_mount_microdrive(&machine, slot + 1u,
                                            &images[slot]) == WZ_RESULT_OK);
        REQUIRE(wz_machine_microdrive_at(&machine, slot)->image_present == 1u);
        REQUIRE(wz_machine_microdrive_at(&machine, slot)->image_identity ==
                images[slot].identity);
        REQUIRE(wz_mdr_transport_select_motor(
                    wz_machine_microdrive_at(&machine, slot),
                    (wz_byte_t)slot) == WZ_RESULT_OK);
        REQUIRE(wz_mdr_transport_set_write_mode(
                    wz_machine_microdrive_at(&machine, slot), 1u) ==
                WZ_RESULT_OK);
        REQUIRE(wz_mdr_transport_write(
                    wz_machine_microdrive_at(&machine, slot),
                    (wz_byte_t)(0xa0u + slot)) == WZ_RESULT_OK);
    }

    REQUIRE(wz_machine_reset(&machine) == WZ_RESULT_OK);
    for (slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
        const wz_mdr_transport_t* transport =
            wz_machine_microdrive_at_const(&machine, slot);
        REQUIRE(transport->image_identity == images[slot].identity);
        REQUIRE(transport->dirty == 1u);
        REQUIRE(transport->offset == WZ_MDR_HEADER_OFFSET + 1u);
    }

    state_bytes = (wz_byte_t*)malloc(WZ_STATE_MACHINE_LENGTH);
    REQUIRE(state_bytes != NULL);
    wz_state_writer_init(&writer, state_bytes, WZ_STATE_MACHINE_LENGTH);
    REQUIRE(wz_state_serialize_machine(&machine, &writer) == WZ_RESULT_OK);
    REQUIRE(writer.length == WZ_STATE_MACHINE_LENGTH);
    REQUIRE(wz_state_deserialize_machine(&restored, state_bytes,
                                         writer.length) == WZ_RESULT_OK);
    for (slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
        const wz_mdr_transport_t* transport =
            wz_machine_microdrive_at_const(&restored, slot);
        REQUIRE(transport->image_identity == images[slot].identity);
        REQUIRE(transport->dirty == 1u);
        REQUIRE(transport->offset == WZ_MDR_HEADER_OFFSET + 1u);
    }

    machine.networking_mode = WZ_NETWORKING_INTERFACE1;
    for (slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
        wz_bus_request_t request;
        select_motor(&machine, slot, &tick);
        REQUIRE(wz_machine_interface1_active_motor(&machine) == slot);
        wz_bus_request_init(&request, WZ_BUS_IO_READ, tick++, 0x12e7u,
                            0xffu, 4u);
        REQUIRE(wz_machine_bus_request(&machine, &request) == WZ_RESULT_OK);
        REQUIRE(request.source == WZ_BUS_SOURCE_INPUT);
        REQUIRE(request.value == (wz_byte_t)(0x30u + slot));
    }

    flush_log.fail_slot = 3u;
    REQUIRE(wz_machine_reconfigure_networking_mode_with_mdr_bank_resolution(
                &machine, WZ_NETWORKING_NONE, record_flush, &flush_log,
                false) == WZ_RESULT_INVALID_STATE);
    REQUIRE(machine.networking_mode == WZ_NETWORKING_INTERFACE1);
    REQUIRE(wz_machine_microdrives_are_dirty(&machine));
    memset(&flush_log, 0, sizeof(flush_log));
    flush_log.fail_slot = SIZE_MAX;
    REQUIRE(wz_machine_reconfigure_networking_mode_with_mdr_bank_resolution(
                &machine, WZ_NETWORKING_NONE, record_flush, &flush_log,
                false) == WZ_RESULT_OK);
    REQUIRE(flush_log.calls == WZ_MACHINE_MICRODRIVE_COUNT);
    for (slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
        REQUIRE(flush_log.slots[slot] == 1u);
    }

    exit_code = 0;
    printf("microdrive-bank slots=%u state=pass\n",
           (unsigned)WZ_MACHINE_MICRODRIVE_COUNT);

failure:
    free(state_bytes);
    wz_machine_destroy(&machine);
    wz_machine_destroy(&restored);
    return exit_code;
}
