/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdio.h>
#include <string.h>

#include "core/wz_machine.h"
#include "core/wz_state.h"

static size_t cases;

#define CHECK(test) do { \
    ++cases; \
    if (!(test)) { \
        (void)fprintf(stderr, "ZX Net canonical state failed at line %d: %s\n", \
                      __LINE__, #test); \
        goto cleanup; \
    } \
} while (0)

static wz_result_t unused_write_callback(wz_word_t block_id,
                                         const wz_byte_t* data,
                                         size_t length,
                                         void* context)
{
    (void)block_id;
    (void)data;
    (void)length;
    (void)context;
    return WZ_RESULT_OK;
}

static wz_qword_t machine_hash(const wz_machine_t* machine)
{
    wz_qword_t hash = 0u;
    return wz_state_hash_machine(machine, &hash) == WZ_RESULT_OK ? hash : 0u;
}

int main(void)
{
    static wz_machine_t machine;
    static wz_machine_t restored;
    static wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE];
    static wz_byte_t image_bytes[WZ_MDR_MIN_SECTORS * WZ_MDR_SECTOR_SIZE];
    static wz_byte_t state_bytes[WZ_STATE_MACHINE_LENGTH];
    static wz_byte_t state_repeat[WZ_STATE_MACHINE_LENGTH];
    static wz_byte_t malformed[WZ_STATE_MACHINE_LENGTH];
    wz_mdr_image_t image;
    wz_state_writer_t writer;
    wz_state_writer_t repeat_writer;
    wz_qword_t source_hash;
    wz_qword_t restored_hash;
    wz_qword_t altered_hash;
    bool machine_initialized = false;
    bool restored_initialized = false;
    int result = 1;

    for (size_t index = 0u; index < sizeof(interface1_rom); ++index) {
        interface1_rom[index] = (wz_byte_t)(index * 7u + 3u);
    }
    for (size_t index = 0u; index < sizeof(image_bytes); ++index) {
        image_bytes[index] = (wz_byte_t)(index * 13u + 9u);
    }

    CHECK(wz_machine_init(&machine, wz_machine_profile_48k_pal()) == WZ_RESULT_OK);
    machine_initialized = true;
    CHECK(wz_machine_init(&restored, wz_machine_profile_48k_pal()) == WZ_RESULT_OK);
    restored_initialized = true;
    CHECK(wz_machine_load_interface1_rom(&machine, interface1_rom,
          sizeof(interface1_rom), WZ_INTERFACE1_ROM_NEW) == WZ_RESULT_OK);
    CHECK(wz_machine_load_interface1_rom(&restored, interface1_rom,
          sizeof(interface1_rom), WZ_INTERFACE1_ROM_NEW) == WZ_RESULT_OK);
    CHECK(wz_machine_set_networking_mode(&machine,
          WZ_NETWORKING_INTERFACE1) == WZ_RESULT_OK);
    CHECK(wz_machine_set_networking_mode(&restored,
          WZ_NETWORKING_INTERFACE1) == WZ_RESULT_OK);
    CHECK(wz_machine_set_interface1_rom_page(&machine, 2u) == WZ_RESULT_OK);
    CHECK(wz_machine_interface1_control_write(&machine, 0x06u, 7001u) ==
          WZ_RESULT_OK);
    CHECK(wz_machine_interface1_control_write(&machine, 0x04u, 7002u) ==
          WZ_RESULT_OK);
    machine.master_tick = 7002u;
    wz_zxnet_set_master_tick(&machine.zxnet, machine.master_tick);

    CHECK(wz_mdr_image_init(&image, image_bytes, sizeof(image_bytes)) ==
          WZ_RESULT_OK);
    for (size_t slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
        wz_mdr_transport_t* transport = wz_machine_microdrive_at(&machine, slot);
        CHECK(transport != NULL);
        CHECK(wz_machine_mount_microdrive(&machine, slot + 1u, &image) ==
              WZ_RESULT_OK);
        transport = wz_machine_microdrive_at(&machine, slot);
        CHECK(wz_mdr_transport_select_motor(transport, (wz_byte_t)slot) ==
              WZ_RESULT_OK);
        transport->sector = slot;
        transport->offset = 40u + slot;
        transport->phase = slot % 2u == 0u ? WZ_MDR_PHASE_HEADER :
                                              WZ_MDR_PHASE_DATA;
        transport->write_enabled = (wz_byte_t)(slot % 2u);
        transport->erase_enabled = (wz_byte_t)(slot == 6u);
        transport->host_write_protected = (wz_byte_t)(slot == 4u);
        if (slot % 2u != 0u) {
            transport->write_enabled = 1u;
            transport->erase_enabled = 0u;
            transport->host_write_protected = 0u;
            CHECK(wz_mdr_transport_write(transport,
                  (wz_byte_t)(0xa0u + slot)) == WZ_RESULT_OK);
        }
    }

    machine.zxnet.state.state = WZ_ZXNET_COLLREAD;
    machine.zxnet.state.claim_byte = 0x5au;
    machine.zxnet.state.block_id = 0x1234u;
    machine.zxnet.state.last_block_id = 0x4321u;
    machine.zxnet.state.network_delay = 37u;
    machine.zxnet.state.busy_length = 11u;
    machine.zxnet.state.free_length = 13u;
    machine.zxnet.state.network_count = 19u;
    machine.zxnet.state.network_byte = 0xc3u;
    machine.zxnet.state.bit_count = 7u;
    machine.zxnet.state.buffer_position = 29u;
    machine.zxnet.state.buffer_length = WZ_ZXNET_DATA_CAPACITY;
    machine.zxnet.state.read_block_ready = true;
    for (size_t index = 0u; index < sizeof(machine.zxnet.state.buffer); ++index) {
        machine.zxnet.state.buffer[index] = (wz_byte_t)(index * 5u + 1u);
    }
    wz_zxnet_set_write_callback(&machine.zxnet, unused_write_callback, &machine);

    wz_state_writer_init(&writer, state_bytes, sizeof(state_bytes));
    wz_state_writer_init(&repeat_writer, state_repeat, sizeof(state_repeat));
    CHECK(wz_state_serialize_machine(&machine, &writer) == WZ_RESULT_OK);
    CHECK(wz_state_serialize_machine(&machine, &repeat_writer) == WZ_RESULT_OK);
    CHECK(writer.length == repeat_writer.length);
    CHECK(memcmp(state_bytes, state_repeat, writer.length) == 0);
    CHECK(wz_state_hash_machine(&machine, &source_hash) == WZ_RESULT_OK);
    CHECK(wz_state_hash_machine(&machine, &altered_hash) == WZ_RESULT_OK &&
          source_hash == altered_hash);

    {
        wz_mdr_transport_t* clean_drive = wz_machine_microdrive_at(&machine, 0u);
        const wz_byte_t original = clean_drive->buffer[0];
        clean_drive->buffer[0] ^= 0xffu;
        CHECK(wz_state_hash_machine(&machine, &altered_hash) == WZ_RESULT_OK &&
              source_hash == altered_hash);
        clean_drive->buffer[0] = original;
    }
    machine.interface1_control_latch_tick += 1u;
    CHECK(machine_hash(&machine) != source_hash);
    machine.interface1_control_latch_tick -= 1u;
    machine.networking_mode = WZ_NETWORKING_NONE;
    CHECK(machine_hash(&machine) != source_hash);
    machine.networking_mode = WZ_NETWORKING_INTERFACE1;
    machine.zxnet.state.block_id ^= 1u;
    CHECK(machine_hash(&machine) != source_hash);
    machine.zxnet.state.block_id ^= 1u;
    wz_machine_microdrive_at(&machine, 0u)->host_write_protected = 1u;
    CHECK(machine_hash(&machine) != source_hash);
    wz_machine_microdrive_at(&machine, 0u)->host_write_protected = 0u;

    CHECK(wz_state_deserialize_machine(&restored, state_bytes,
          writer.length) == WZ_RESULT_OK);
    CHECK(wz_state_hash_machine(&restored, &restored_hash) == WZ_RESULT_OK &&
          source_hash == restored_hash);
    CHECK(restored.networking_mode == WZ_NETWORKING_INTERFACE1);
    CHECK(restored.has_interface1_rom == machine.has_interface1_rom &&
          restored.interface1_rom_variant == machine.interface1_rom_variant &&
          restored.interface1_rom_page == machine.interface1_rom_page &&
          restored.interface1_rom_identity == machine.interface1_rom_identity);
    CHECK(restored.interface1_control_latch == machine.interface1_control_latch &&
          restored.interface1_previous_control_latch ==
              machine.interface1_previous_control_latch &&
          restored.interface1_motor_shift == machine.interface1_motor_shift &&
          restored.interface1_active_motor == machine.interface1_active_motor &&
          restored.interface1_control_latch_tick ==
              machine.interface1_control_latch_tick);
    for (size_t slot = 0u; slot < WZ_MACHINE_MICRODRIVE_COUNT; ++slot) {
        const wz_mdr_transport_t* source =
            wz_machine_microdrive_at_const(&machine, slot);
        const wz_mdr_transport_t* target =
            wz_machine_microdrive_at_const(&restored, slot);
        CHECK(target->image_present == source->image_present &&
              target->image_identity == source->image_identity &&
              target->image_length == source->image_length &&
              target->image_sector_count == source->image_sector_count &&
              target->sector == source->sector && target->offset == source->offset &&
              target->active_motor == source->active_motor &&
              target->write_enabled == source->write_enabled &&
              target->erase_enabled == source->erase_enabled &&
              target->host_write_protected == source->host_write_protected &&
              target->dirty == source->dirty && target->phase == source->phase);
        CHECK(target->image == NULL);
        if (source->dirty != 0u) {
            CHECK(memcmp(target->buffer, source->buffer,
                         WZ_MDR_SECTOR_SIZE) == 0);
        }
    }
    CHECK(restored.zxnet.state.state == machine.zxnet.state.state &&
          restored.zxnet.state.claim_byte == machine.zxnet.state.claim_byte &&
          restored.zxnet.state.block_id == machine.zxnet.state.block_id &&
          restored.zxnet.state.last_block_id == machine.zxnet.state.last_block_id &&
          restored.zxnet.state.network_delay == machine.zxnet.state.network_delay &&
          restored.zxnet.state.busy_length == machine.zxnet.state.busy_length &&
          restored.zxnet.state.free_length == machine.zxnet.state.free_length &&
          restored.zxnet.state.network_count == machine.zxnet.state.network_count &&
          restored.zxnet.state.network_byte == machine.zxnet.state.network_byte &&
          restored.zxnet.state.bit_count == machine.zxnet.state.bit_count &&
          restored.zxnet.state.buffer_position ==
              machine.zxnet.state.buffer_position &&
          restored.zxnet.state.buffer_length == machine.zxnet.state.buffer_length &&
          restored.zxnet.state.read_block_ready ==
              machine.zxnet.state.read_block_ready);
    CHECK(memcmp(restored.zxnet.state.buffer, machine.zxnet.state.buffer,
                 WZ_ZXNET_DATA_CAPACITY) == 0);
    CHECK(restored.zxnet.write_callback == NULL &&
          restored.zxnet.write_context == NULL);

    {
        wz_mdr_transport_t* preserved_drive =
            wz_machine_microdrive_at(&restored, 0u);
        const size_t q_offset = WZ_STATE_MACHINE_MEMORY_OFFSET + 65536u +
            20u + 17u + 31u * WZ_MACHINE_MICRODRIVE_COUNT + 281u;
        CHECK(preserved_drive != NULL);
        preserved_drive->image = &image;
        wz_zxnet_set_write_callback(&restored.zxnet,
                                    unused_write_callback, &restored);
        CHECK(wz_state_hash_machine(&restored, &restored_hash) == WZ_RESULT_OK &&
              source_hash == restored_hash);
        memcpy(malformed, state_bytes, writer.length);
        CHECK(q_offset < writer.length && q_offset < sizeof(malformed));
        malformed[q_offset] = 2u;
        CHECK(wz_state_hash_machine(&restored, &restored_hash) == WZ_RESULT_OK);
        CHECK(wz_state_deserialize_machine(&restored, malformed,
              writer.length) == WZ_RESULT_INVALID_STATE);
        CHECK(wz_state_hash_machine(&restored, &altered_hash) == WZ_RESULT_OK &&
              restored_hash == altered_hash);
        CHECK(preserved_drive->image == &image);
        CHECK(restored.zxnet.write_callback == unused_write_callback &&
              restored.zxnet.write_context == &restored);
    }

    result = 0;

cleanup:
    if (restored_initialized) wz_machine_destroy(&restored);
    if (machine_initialized) wz_machine_destroy(&machine);
    if (result == 0) {
        (void)printf("PASS ZX Net canonical state fields and transactional restore; "
                     "cases=%zu canonical_hash=%016llx\n", cases,
                     (unsigned long long)source_hash);
    }
    return result;
}
