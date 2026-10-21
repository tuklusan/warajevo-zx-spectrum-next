/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_microdrive_manager.h"
#include "core/wz_machine.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FUZZ_ITERATIONS 1024u
#define IMAGE_CAPACITY \
    (WZ_MDR_MAX_SECTORS * WZ_MDR_SECTOR_SIZE + 3u)

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Microdrive fuzz contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

typedef struct {
    size_t calls;
    size_t expected_sectors;
} flush_state_t;

static wz_dword_t next_random(wz_dword_t* state)
{
    wz_dword_t value = *state;
    value ^= value << 13u;
    value ^= value >> 17u;
    value ^= value << 5u;
    *state = value;
    return value;
}

static wz_result_t flush_sector(size_t sector, const wz_byte_t* data,
                                size_t length, void* context)
{
    flush_state_t* state = (flush_state_t*)context;
    if (state == 0 || data == 0 || length != WZ_MDR_SECTOR_SIZE ||
        sector >= state->expected_sectors) {
        return WZ_RESULT_INVALID_STATE;
    }
    ++state->calls;
    return WZ_RESULT_OK;
}

static size_t fuzz_length(size_t iteration, wz_dword_t* random_state)
{
    static const size_t boundaries[] = {
        0u,
        1u,
        WZ_MDR_SECTOR_SIZE - 1u,
        WZ_MDR_SECTOR_SIZE,
        WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE - 1u,
        WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE,
        (WZ_MDR_MAX_SECTORS - 1u) * WZ_MDR_SECTOR_SIZE,
        WZ_MDR_MAX_SECTORS * WZ_MDR_SECTOR_SIZE,
        WZ_MDR_MAX_SECTORS * WZ_MDR_SECTOR_SIZE + 1u,
        WZ_MDR_MAX_SECTORS * WZ_MDR_SECTOR_SIZE + 2u
    };

    switch (iteration % 4u) {
    case 0u:
        return (WZ_MDR_MIN_SECTORS +
                next_random(random_state) %
                    (WZ_MDR_MAX_SECTORS - WZ_MDR_MIN_SECTORS + 1u)) *
            WZ_MDR_SECTOR_SIZE;
    case 1u:
        return WZ_MDR_MAX_SECTORS * WZ_MDR_SECTOR_SIZE + 1u;
    case 2u:
        return next_random(random_state) % (IMAGE_CAPACITY + 1u);
    default:
        return boundaries[next_random(random_state) %
                          (sizeof(boundaries) / sizeof(boundaries[0]))];
    }
}

int main(void)
{
    static wz_byte_t media[IMAGE_CAPACITY];
    static wz_byte_t edited[IMAGE_CAPACITY];
    static wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE];
    static wz_byte_t raw_sector[WZ_MDR_SECTOR_SIZE + 3u];
    static wz_byte_t edit_bytes[16];
    static wz_machine_t machine;
    wz_mdr_image_t image;
    wz_byte_t before_image_bytes[sizeof(wz_mdr_image_t)];
    wz_mdr_image_t mounted_image;
    wz_mdr_transport_t transport;
    wz_microdrive_manager_file_t files[WZ_MICRODRIVE_MANAGER_MAX_FILES];
    wz_microdrive_manager_allocation_t allocation;
    flush_state_t flush = {0u, 0u};
    wz_dword_t random_state = UINT32_C(0x6d647231);
    size_t accepted = 0u;
    size_t rejected = 0u;
    bool machine_initialized = false;
    int result = 1;

    memset(media, 0, sizeof(media));
    memset(interface1_rom, 0, sizeof(interface1_rom));
    memset(raw_sector, 0, sizeof(raw_sector));
    memset(edit_bytes, 0, sizeof(edit_bytes));

    wz_mdr_transport_init(&transport);
    REQUIRE(wz_mdr_transport_mount(&transport, 0) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_mdr_transport_select_motor(0, 0u) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_mdr_transport_set_write_mode(0, 0u) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_mdr_transport_set_erase(0, 0u) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_mdr_transport_set_host_write_protected(0, 0u) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_mdr_transport_read(0, edit_bytes) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_mdr_transport_write(0, 0u) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_mdr_transport_flush(0, flush_sector, &flush) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_mdr_transport_flush(&transport, 0, &flush) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_mdr_transport_select_motor(&transport, 8u) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_mdr_transport_set_write_mode(&transport, 2u) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_mdr_transport_set_erase(&transport, 2u) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_mdr_transport_set_host_write_protected(&transport, 2u) ==
            WZ_RESULT_INVALID_ARGUMENT);
    wz_mdr_transport_discard(0);

    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    machine_initialized = true;
    REQUIRE(wz_machine_interface1_control_write(&machine, 0u, 0u) ==
            WZ_RESULT_INVALID_STATE);
    REQUIRE(wz_machine_set_networking_mode(
                &machine, (wz_networking_mode_t)0xffu) ==
            WZ_RESULT_INVALID_ARGUMENT);
    REQUIRE(wz_machine_set_networking_mode(
                &machine, WZ_NETWORKING_EAR_MIC) ==
            WZ_RESULT_UNSUPPORTED_OPERATION);
    REQUIRE(wz_machine_load_interface1_rom(&machine, interface1_rom,
                sizeof(interface1_rom), WZ_INTERFACE1_ROM_OLD) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_set_networking_mode(
                &machine, WZ_NETWORKING_INTERFACE1) == WZ_RESULT_OK);

    for (size_t iteration = 0u; iteration < FUZZ_ITERATIONS; ++iteration) {
        const size_t length = fuzz_length(iteration, &random_state);
        size_t sector_index;
        size_t offset;
        size_t byte_count;
        wz_byte_t value;
        const wz_byte_t* sector_data = 0;
        wz_result_t parse_result;

        REQUIRE(length <= sizeof(media));
        for (size_t index = 0u; index < length; ++index) {
            media[index] = (wz_byte_t)next_random(&random_state);
        }
        memset(&image, 0xa5, sizeof(image));
        memcpy(before_image_bytes, &image, sizeof(image));
        parse_result = wz_mdr_image_init(&image, media, length);
        if (parse_result != WZ_RESULT_OK) {
            REQUIRE(parse_result == WZ_RESULT_PARSE_ERROR);
            REQUIRE(memcmp(&image, before_image_bytes, sizeof(image)) == 0);
            ++rejected;
            continue;
        }

        ++accepted;
        REQUIRE(image.sector_count >= WZ_MDR_MIN_SECTORS &&
                image.sector_count <= WZ_MDR_MAX_SECTORS);
        REQUIRE(wz_mdr_image_sector(
                    &image, image.sector_count, &sector_data) ==
                WZ_RESULT_INVALID_STATE);
        (void)wz_microdrive_manager_validate(&image);
        (void)wz_microdrive_manager_catalog(&image, files,
            WZ_MICRODRIVE_MANAGER_MAX_FILES, &sector_index, &allocation);
        (void)wz_microdrive_manager_sector_verify(
            &image, next_random(&random_state) % (image.sector_count + 2u));

        memcpy(edited, media, length);
        sector_index = next_random(&random_state) % (image.sector_count + 2u);
        offset = next_random(&random_state) % 1024u;
        byte_count = next_random(&random_state) % sizeof(edit_bytes);
        (void)wz_microdrive_manager_sector_repair(
            edited, length, sector_index);
        (void)wz_microdrive_manager_sector_edit_data(
            edited, length, sector_index, offset, edit_bytes, byte_count);
        (void)wz_microdrive_manager_sector_edit_raw(
            edited, length, sector_index, raw_sector,
            next_random(&random_state) % sizeof(raw_sector));
        (void)wz_microdrive_manager_format(edited, length, "FUZZ");

        wz_mdr_transport_init(&transport);
        REQUIRE(wz_mdr_transport_read(&transport, &value) ==
                WZ_RESULT_INVALID_STATE);
        REQUIRE(wz_mdr_transport_write(&transport, 0u) ==
                WZ_RESULT_INVALID_STATE);
        REQUIRE(wz_mdr_transport_mount(&transport, &image) == WZ_RESULT_OK);
        flush.expected_sectors = image.sector_count;
        flush.calls = 0u;
        REQUIRE(wz_mdr_transport_read(&transport, &value) ==
                WZ_RESULT_INVALID_STATE);
        REQUIRE(wz_mdr_transport_select_motor(
                    &transport, (wz_byte_t)(next_random(&random_state) % 8u)) ==
                WZ_RESULT_OK);
        REQUIRE(wz_mdr_transport_read(&transport, &value) == WZ_RESULT_OK);
        REQUIRE(wz_mdr_transport_set_write_mode(&transport, 1u) ==
                WZ_RESULT_OK);
        REQUIRE(wz_mdr_transport_set_erase(&transport, 1u) ==
                WZ_RESULT_OK);
        REQUIRE(wz_mdr_transport_write(&transport, (wz_byte_t)next_random(
                    &random_state)) == WZ_RESULT_INVALID_STATE);
        REQUIRE(wz_mdr_transport_is_dirty(&transport) == 0u);
        REQUIRE(wz_mdr_transport_set_erase(&transport, 0u) ==
                WZ_RESULT_OK);
        REQUIRE(wz_mdr_transport_set_host_write_protected(&transport, 1u) ==
                WZ_RESULT_OK);
        REQUIRE(wz_mdr_transport_write(&transport, (wz_byte_t)next_random(
                    &random_state)) == WZ_RESULT_INVALID_STATE);
        REQUIRE(wz_mdr_transport_set_host_write_protected(&transport, 0u) ==
                WZ_RESULT_OK);
        if (wz_mdr_image_is_write_protected(&image)) {
            REQUIRE(wz_mdr_transport_write(&transport, 0xa5u) ==
                    WZ_RESULT_INVALID_STATE);
        } else {
            REQUIRE(wz_mdr_transport_write(&transport, 0xa5u) ==
                    WZ_RESULT_OK);
            REQUIRE(wz_mdr_transport_is_dirty(&transport) != 0u);
            REQUIRE(wz_mdr_transport_flush(
                        &transport, flush_sector, &flush) == WZ_RESULT_OK);
            REQUIRE(flush.calls == 1u &&
                    wz_mdr_transport_is_dirty(&transport) == 0u);
        }
        wz_mdr_transport_discard(&transport);

        mounted_image = image;
        REQUIRE(wz_machine_mount_microdrive(&machine, 1u, &mounted_image) ==
                WZ_RESULT_OK);
        for (size_t transition = 0u; transition < 16u; ++transition) {
            const wz_byte_t control = (wz_byte_t)next_random(&random_state);
            REQUIRE(wz_machine_interface1_control_write(
                        &machine, control, (wz_master_tick_t)transition) ==
                    WZ_RESULT_OK);
            if ((control & 0x04u) != 0u) {
                const wz_result_t read_result =
                    wz_machine_interface1_data_read(&machine, &value);
                REQUIRE(read_result == WZ_RESULT_OK ||
                        read_result == WZ_RESULT_INVALID_STATE);
            } else {
                const wz_result_t write_result =
                    wz_machine_interface1_data_write(&machine, control);
                REQUIRE(write_result == WZ_RESULT_OK ||
                        write_result == WZ_RESULT_INVALID_STATE);
            }
        }
        for (size_t slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
            wz_mdr_transport_discard(
                wz_machine_microdrive_at(&machine, slot));
        }
        REQUIRE(!wz_machine_microdrives_are_dirty(&machine));
    }

    REQUIRE(accepted > 0u && rejected > 0u);
    result = 0;

cleanup:
    if (machine_initialized) {
        wz_machine_destroy(&machine);
    }
    if (result == 0) {
        (void)printf("PASS MDR malformed-image and invalid-transition fuzz; accepted=%zu rejected=%zu\n",
                     accepted, rejected);
    }
    return result;
}
