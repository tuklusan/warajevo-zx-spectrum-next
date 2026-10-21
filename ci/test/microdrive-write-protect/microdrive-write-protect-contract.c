/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "core/wz_machine.h"
#include "core/wz_state.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Microdrive write-protection contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

typedef struct {
    wz_byte_t* image;
    size_t calls;
    size_t sector;
    wz_byte_t captured[WZ_MDR_SECTOR_SIZE];
    bool fail;
} flush_context_t;

static wz_result_t persist_sector(size_t sector, const wz_byte_t* data,
                                  size_t length, void* opaque)
{
    flush_context_t* context = (flush_context_t*)opaque;

    if (context == NULL || data == NULL || length != WZ_MDR_SECTOR_SIZE) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    ++context->calls;
    context->sector = sector;
    memcpy(context->captured, data, length);
    if (context->fail) {
        return WZ_RESULT_INVALID_STATE;
    }
    memcpy(context->image + sector * WZ_MDR_SECTOR_SIZE, data, length);
    return WZ_RESULT_OK;
}

static bool select_motor_zero(wz_machine_t* machine,
                              wz_master_tick_t* tick)
{
    size_t step;

    for (step = 0u; step < WZ_MACHINE_MICRODRIVE_COUNT; ++step) {
        const wz_byte_t data =
            step == WZ_MACHINE_MICRODRIVE_COUNT - 1u ? 0u : 1u;
        if (wz_machine_interface1_control_write(
                machine, (wz_byte_t)(0x06u | data), (*tick)++) !=
                WZ_RESULT_OK ||
            wz_machine_interface1_control_write(
                machine, (wz_byte_t)(0x04u | data), (*tick)++) !=
                WZ_RESULT_OK) {
            return false;
        }
    }
    return wz_machine_interface1_active_motor(machine) == 0u;
}

static bool io_request(wz_machine_t* machine, wz_bus_cycle_t cycle,
                       wz_master_tick_t* tick, wz_word_t address,
                       wz_byte_t* value)
{
    wz_bus_request_t request;

    wz_bus_request_init(&request, cycle, *tick, address,
                        value == NULL ? 0u : *value, 4u);
    *tick += 4u;
    if (wz_machine_bus_request(machine, &request) != WZ_RESULT_OK) {
        return false;
    }
    if (value != NULL) {
        *value = request.value;
    }
    return true;
}

int main(void)
{
    static wz_machine_t machine;
    static wz_machine_t restored;
    static wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE];
    static wz_byte_t image_bytes[WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    static wz_byte_t state_bytes[WZ_STATE_MACHINE_LENGTH];
    wz_mdr_image_t image;
    wz_state_writer_t writer;
    flush_context_t flush = {0};
    wz_mdr_transport_t* transport;
    const wz_mdr_transport_t* restored_transport;
    wz_byte_t value = 0u;
    wz_master_tick_t tick = 0u;
    bool machine_initialized = false;
    bool restored_initialized = false;
    int result = 1;

    memset(image_bytes, 0x35u, sizeof(image_bytes));
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    machine_initialized = true;
    REQUIRE(wz_machine_init(&restored, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    restored_initialized = true;
    REQUIRE(wz_machine_load_interface1_rom(&machine, interface1_rom,
            sizeof(interface1_rom), WZ_INTERFACE1_ROM_OLD) == WZ_RESULT_OK);
    REQUIRE(wz_machine_set_networking_mode(&machine,
            WZ_NETWORKING_INTERFACE1) == WZ_RESULT_OK);
    REQUIRE(wz_mdr_image_init(&image, image_bytes, sizeof(image_bytes)) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_mount_microdrive(&machine, 1u, &image) == WZ_RESULT_OK);
    REQUIRE(select_motor_zero(&machine, &tick));

    /* EF status reports the mounted cartridge's effective protection state. */
    REQUIRE(io_request(&machine, WZ_BUS_IO_READ, &tick, 0x12efu, &value));
    REQUIRE((value & 0x01u) == 0u);
    transport = wz_machine_microdrive_at(&machine, 0u);
    REQUIRE(transport != NULL);
    REQUIRE(wz_mdr_transport_set_host_write_protected(transport, 1u) ==
            WZ_RESULT_OK);
    REQUIRE(io_request(&machine, WZ_BUS_IO_READ, &tick, 0x12efu, &value));
    REQUIRE((value & 0x01u) != 0u);
    REQUIRE(wz_machine_interface1_control_write(&machine, 0x08u, tick++) ==
            WZ_RESULT_OK);
    value = 0xa5u;
    REQUIRE(!io_request(&machine, WZ_BUS_IO_WRITE, &tick, 0x12e7u, &value));
    REQUIRE(wz_mdr_transport_is_dirty(transport) == 0u);
    REQUIRE(image_bytes[WZ_MDR_HEADER_OFFSET] == 0x35u);

    /* A permitted write stays staged until the application callback succeeds. */
    REQUIRE(wz_mdr_transport_set_host_write_protected(transport, 0u) ==
            WZ_RESULT_OK);
    REQUIRE(io_request(&machine, WZ_BUS_IO_WRITE, &tick, 0x12e7u, &value));
    REQUIRE(wz_mdr_transport_is_dirty(transport) != 0u);
    REQUIRE(image_bytes[WZ_MDR_HEADER_OFFSET] == 0x35u);
    flush.image = image_bytes;
    flush.fail = true;
    REQUIRE(wz_mdr_transport_flush(transport, persist_sector, &flush) ==
            WZ_RESULT_INVALID_STATE);
    REQUIRE(flush.calls == 1u && flush.sector == 0u);
    REQUIRE(flush.captured[WZ_MDR_HEADER_OFFSET] == 0xa5u);
    REQUIRE(wz_mdr_transport_is_dirty(transport) != 0u);
    REQUIRE(image_bytes[WZ_MDR_HEADER_OFFSET] == 0x35u);

    /* A host protection change blocks flush without losing staged data. */
    REQUIRE(wz_mdr_transport_set_host_write_protected(transport, 1u) ==
            WZ_RESULT_OK);
    REQUIRE(wz_mdr_transport_flush(transport, persist_sector, &flush) ==
            WZ_RESULT_INVALID_STATE);
    REQUIRE(flush.calls == 1u && wz_mdr_transport_is_dirty(transport) != 0u);
    REQUIRE(wz_mdr_transport_set_host_write_protected(transport, 0u) ==
            WZ_RESULT_OK);
    flush.fail = false;
    REQUIRE(wz_mdr_transport_flush(transport, persist_sector, &flush) ==
            WZ_RESULT_OK);
    REQUIRE(flush.calls == 2u &&
            wz_mdr_transport_is_dirty(transport) == 0u);
    REQUIRE(image_bytes[WZ_MDR_HEADER_OFFSET] == 0xa5u);

    /* Host write-protection is part of canonical machine state. */
    REQUIRE(wz_mdr_transport_set_host_write_protected(transport, 1u) ==
            WZ_RESULT_OK);
    wz_state_writer_init(&writer, state_bytes, sizeof(state_bytes));
    REQUIRE(wz_state_serialize_machine(&machine, &writer) == WZ_RESULT_OK);
    REQUIRE(wz_state_deserialize_machine(&restored, state_bytes,
                                         writer.length) == WZ_RESULT_OK);
    restored_transport = wz_machine_microdrive_at_const(&restored, 0u);
    REQUIRE(restored_transport != NULL &&
            restored_transport->host_write_protected == 1u);

    result = 0;

cleanup:
    if (restored_initialized) wz_machine_destroy(&restored);
    if (machine_initialized) wz_machine_destroy(&machine);
    if (result == 0) {
        (void)puts("PASS Microdrive write protection and application-controlled flush");
    }
    return result;
}
