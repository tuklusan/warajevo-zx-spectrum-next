/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_tape.h"

#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <zlib.h>

#define WZ_TAP_PILOT_TSTATES 2168u
#define WZ_TAP_SYNC_FIRST_TSTATES 667u
#define WZ_TAP_SYNC_SECOND_TSTATES 735u
#define WZ_TAP_ZERO_TSTATES 855u
#define WZ_TAP_ONE_TSTATES 1710u
#define WZ_TAP_PAUSE_TSTATES 3500000u

static void wz_tape_write_le32(wz_byte_t bytes[4], wz_dword_t value)
{
    bytes[0] = (wz_byte_t)(value & 0xffu);
    bytes[1] = (wz_byte_t)((value >> 8u) & 0xffu);
    bytes[2] = (wz_byte_t)((value >> 16u) & 0xffu);
    bytes[3] = (wz_byte_t)(value >> 24u);
}

static wz_result_t wz_tape_tap_add_count(size_t* total, size_t amount)
{
    if (total == 0 || amount > SIZE_MAX - *total) {
        return WZ_RESULT_PARSE_ERROR;
    }
    *total += amount;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tape_tap_duration(wz_dword_t tstates,
                                        wz_dword_t ticks_per_tstate,
                                        wz_master_tick_t* duration)
{
    if (duration == 0 || ticks_per_tstate == 0u ||
        (wz_master_tick_t)tstates > UINT64_MAX / ticks_per_tstate) {
        return WZ_RESULT_PARSE_ERROR;
    }
    *duration = (wz_master_tick_t)tstates * ticks_per_tstate;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tape_tap_count_block(const wz_byte_t* block,
                                           size_t block_length,
                                           size_t* total)
{
    size_t pilot_count;
    size_t data_pulse_count;

    if (block == 0 || total == 0 || block_length < 2u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    pilot_count = block[0u] == 0u ? 8063u : 3223u;
    if (block_length > (SIZE_MAX - pilot_count - 3u) / 16u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    data_pulse_count = block_length * 16u;
    if (wz_tape_tap_add_count(total, pilot_count) != WZ_RESULT_OK ||
        wz_tape_tap_add_count(total, 2u) != WZ_RESULT_OK ||
        wz_tape_tap_add_count(total, data_pulse_count) != WZ_RESULT_OK ||
        wz_tape_tap_add_count(total, 1u) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    return WZ_RESULT_OK;
}

static wz_result_t wz_tape_tap_validate(const wz_byte_t* data,
                                        size_t length,
                                        size_t* count)
{
    size_t offset = 0u;
    size_t total = 0u;

    if (data == 0 || length == 0u || count == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    while (offset < length) {
        size_t block_length;
        wz_byte_t checksum = 0u;

        if (length - offset < 2u) {
            return WZ_RESULT_PARSE_ERROR;
        }
        block_length = (size_t)wz_read_le16(&data[offset]);
        offset += 2u;
        if (block_length < 2u || block_length > length - offset) {
            return WZ_RESULT_PARSE_ERROR;
        }
        for (size_t index = 0u; index < block_length; ++index) {
            checksum ^= data[offset + index];
        }
        if (checksum != 0u ||
            wz_tape_tap_count_block(&data[offset], block_length, &total) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
        offset += block_length;
    }
    *count = total;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tape_tap_append(wz_tape_segment_t* segments,
                                      size_t capacity,
                                      size_t* index,
                                      wz_dword_t tstates,
                                      wz_dword_t ticks_per_tstate,
                                      wz_byte_t level)
{
    wz_master_tick_t duration;

    if (wz_tape_tap_duration(tstates, ticks_per_tstate, &duration) != WZ_RESULT_OK ||
        segments == 0 || index == 0 || *index >= capacity) {
        return WZ_RESULT_PARSE_ERROR;
    }
    segments[*index].duration = duration;
    segments[*index].ear_level = level;
    *index += 1u;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tape_tap_expand_block(const wz_byte_t* block,
                                            size_t block_length,
                                            wz_dword_t ticks_per_tstate,
                                            wz_tape_segment_t* segments,
                                            size_t capacity,
                                            size_t* index)
{
    wz_byte_t level = 1u;
    size_t pilot_count = block[0u] == 0u ? 8063u : 3223u;

    for (size_t pulse = 0u; pulse < pilot_count; ++pulse) {
        if (wz_tape_tap_append(segments, capacity, index, WZ_TAP_PILOT_TSTATES,
                               ticks_per_tstate, level) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
        level ^= 1u;
    }
    if (wz_tape_tap_append(segments, capacity, index, WZ_TAP_SYNC_FIRST_TSTATES,
                           ticks_per_tstate, level) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    level ^= 1u;
    if (wz_tape_tap_append(segments, capacity, index, WZ_TAP_SYNC_SECOND_TSTATES,
                           ticks_per_tstate, level) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    level ^= 1u;
    for (size_t byte_index = 0u; byte_index < block_length; ++byte_index) {
        for (unsigned bit = 0u; bit < 8u; ++bit) {
            wz_dword_t pulse = (block[byte_index] & (wz_byte_t)(0x80u >> bit)) != 0u ?
                WZ_TAP_ONE_TSTATES : WZ_TAP_ZERO_TSTATES;
            if (wz_tape_tap_append(segments, capacity, index, pulse,
                                   ticks_per_tstate, level) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            level ^= 1u;
            if (wz_tape_tap_append(segments, capacity, index, pulse,
                                   ticks_per_tstate, level) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            level ^= 1u;
        }
    }
    return wz_tape_tap_append(segments, capacity, index, WZ_TAP_PAUSE_TSTATES,
                              ticks_per_tstate, 0u);
}

wz_result_t wz_tape_parse_standard_tap(const wz_byte_t* data,
                                       size_t length,
                                       wz_dword_t master_ticks_per_tstate,
                                       wz_tape_segment_t* segments,
                                       size_t capacity,
                                       size_t* count)
{
    size_t required;
    size_t offset = 0u;
    size_t index = 0u;

    if (master_ticks_per_tstate == 0u || count == 0 ||
        wz_tape_tap_validate(data, length, &required) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    *count = required;
    if (segments == 0 || capacity < required) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    while (offset < length) {
        size_t block_length = (size_t)wz_read_le16(&data[offset]);
        offset += 2u;
        if (wz_tape_tap_expand_block(&data[offset], block_length,
                                     master_ticks_per_tstate, segments,
                                     capacity, &index) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
        offset += block_length;
    }
    return index == required ? WZ_RESULT_OK : WZ_RESULT_PARSE_ERROR;
}

wz_result_t wz_tape_write_standard_tap(const wz_tap_block_t* blocks,
                                       size_t block_count,
                                       wz_byte_t* output,
                                       size_t capacity,
                                       size_t* length)
{
    size_t required = 0u;
    size_t offset = 0u;

    if (blocks == 0 || block_count == 0u || length == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (size_t block_index = 0u; block_index < block_count; ++block_index) {
        const wz_tap_block_t* block = &blocks[block_index];

        if (block->data == 0 || block->length == 0u || block->length > 65534u ||
            block->length > SIZE_MAX - 3u ||
            wz_tape_tap_add_count(&required, block->length + 3u) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
    }
    *length = required;
    if (output == 0 || capacity < required) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    for (size_t block_index = 0u; block_index < block_count; ++block_index) {
        const wz_tap_block_t* block = &blocks[block_index];
        wz_byte_t checksum = 0u;
        wz_word_t encoded_length = (wz_word_t)(block->length + 1u);

        wz_write_le16(&output[offset], encoded_length);
        offset += 2u;
        for (size_t byte_index = 0u; byte_index < block->length; ++byte_index) {
            output[offset + byte_index] = block->data[byte_index];
            checksum ^= block->data[byte_index];
        }
        offset += block->length;
        output[offset++] = checksum;
    }
    return WZ_RESULT_OK;
}

bool wz_tape_is_native_tap(const wz_byte_t* data, size_t length)
{
    if (data == 0 || length < 12u ||
        (data[0u] == 0xffu && data[1u] == 0xffu &&
         data[2u] == 0xffu && data[3u] == 0xffu)) {
        return false;
    }
    for (size_t offset = 4u; offset < 12u; offset += 4u) {
        if (wz_read_le16(&data[offset]) != 0xffffu ||
            wz_read_le16(&data[offset + 2u]) != 0xffffu) {
            return false;
        }
    }
    return true;
}

static wz_result_t wz_tape_native_record_header(const wz_byte_t* data,
                                                 size_t length,
                                                 size_t offset,
                                                 size_t previous_offset,
                                                 size_t* next_offset,
                                                 size_t* header_size,
                                                 wz_native_tap_record_t* record)
{
    size_t next;
    wz_word_t stored_length;

    if (data == 0 || next_offset == 0 || header_size == 0 || record == 0 ||
        offset > length || length - offset < 11u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    if ((wz_qword_t)offset > UINT32_MAX) {
        return WZ_RESULT_PARSE_ERROR;
    }
    record->offset = offset;
    record->previous_offset = (size_t)data[offset] |
        ((size_t)data[offset + 1u] << 8u) |
        ((size_t)data[offset + 2u] << 16u) |
        ((size_t)data[offset + 3u] << 24u);
    next = (size_t)data[offset + 4u] |
        ((size_t)data[offset + 5u] << 8u) |
        ((size_t)data[offset + 6u] << 16u) |
        ((size_t)data[offset + 7u] << 24u);
    stored_length = wz_read_le16(&data[offset + 8u]);
    record->next_offset = next;
    record->stored_length = stored_length;
    record->flag = data[offset + 10u];
    record->decompressed_length = 0u;
    record->compressed_length = 0u;
    record->signed_length = 0u;
    if (record->previous_offset != previous_offset ||
        (next != UINT32_MAX && (next < 12u || next >= length))) {
        return WZ_RESULT_PARSE_ERROR;
    }
    if (stored_length == 65534u || stored_length == 65535u) {
        if (length - offset < 17u) {
            return WZ_RESULT_PARSE_ERROR;
        }
        record->record_type = stored_length == 65534u ? 4u : 5u;
        record->decompressed_length = wz_read_le16(&data[offset + 11u]);
        record->compressed_length = wz_read_le16(&data[offset + 13u]);
        record->signed_length = wz_read_le16(&data[offset + 15u]);
        *header_size = 17u;
    } else {
        if (length - offset < 12u) {
            return WZ_RESULT_PARSE_ERROR;
        }
        record->record_type = data[offset + 11u];
        *header_size = 12u;
    }
    if (stored_length < 2u && *header_size == 12u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    record->payload = &data[offset + *header_size];
    if (*header_size == 17u) {
        if (next == UINT32_MAX) {
            record->payload_length = length - offset - *header_size;
        } else {
            if (next < offset + *header_size) {
                return WZ_RESULT_PARSE_ERROR;
            }
            record->payload_length = next - offset - *header_size;
        }
    } else {
        record->payload_length = (size_t)stored_length - 2u;
        if (record->payload_length > length - offset - *header_size ||
            (next != UINT32_MAX && next != offset + *header_size + record->payload_length)) {
            return WZ_RESULT_PARSE_ERROR;
        }
    }
    *next_offset = next;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_write_native_tap(const wz_native_tap_record_t* records,
                                     size_t record_count,
                                     wz_byte_t* output,
                                     size_t capacity,
                                     size_t* length)
{
    size_t required = 12u;
    size_t offset = 12u;

    if (records == 0 || record_count == 0u || length == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (size_t index = 0u; index < record_count; ++index) {
        const wz_native_tap_record_t* record = &records[index];
        size_t header_size;
        if (record->payload == 0 && record->payload_length != 0u) {
            return WZ_RESULT_INVALID_ARGUMENT;
        }
        if (record->record_type < 4u) {
            if (record->stored_length < 2u ||
                (size_t)record->stored_length - 2u != record->payload_length) {
                return WZ_RESULT_PARSE_ERROR;
            }
            header_size = 12u;
        } else if (record->record_type == 4u || record->record_type == 5u) {
            if (record->stored_length != (record->record_type == 4u ? 65534u : 65535u)) {
                return WZ_RESULT_PARSE_ERROR;
            }
            header_size = 17u;
        } else {
            return WZ_RESULT_PARSE_ERROR;
        }
        if (offset > UINT32_MAX || record->payload_length > SIZE_MAX - header_size ||
            offset > (size_t)UINT32_MAX - header_size - record->payload_length ||
            required > SIZE_MAX - header_size - record->payload_length) {
            return WZ_RESULT_PARSE_ERROR;
        }
        offset += header_size + record->payload_length;
        required += header_size + record->payload_length;
    }
    *length = required;
    if (output == 0 || capacity < required) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    wz_tape_write_le32(&output[0u], 12u);
    wz_tape_write_le32(&output[4u], UINT32_MAX);
    wz_tape_write_le32(&output[8u], UINT32_MAX);
    size_t previous = 0u;
    offset = 12u;
    for (size_t index = 0u; index < record_count; ++index) {
        const wz_native_tap_record_t* record = &records[index];
        size_t header_size = record->record_type < 4u ? 12u : 17u;
        size_t next = index + 1u < record_count ?
            offset + header_size + record->payload_length : (size_t)UINT32_MAX;
        wz_tape_write_le32(&output[offset], (wz_dword_t)previous);
        wz_tape_write_le32(&output[offset + 4u], (wz_dword_t)next);
        wz_write_le16(&output[offset + 8u], record->stored_length);
        output[offset + 10u] = record->flag;
        if (header_size == 12u) {
            output[offset + 11u] = record->record_type;
        } else {
            wz_write_le16(&output[offset + 11u], record->decompressed_length);
            wz_write_le16(&output[offset + 13u], record->compressed_length);
            wz_write_le16(&output[offset + 15u], record->signed_length);
        }
        if (record->payload_length != 0u) {
            memcpy(&output[offset + header_size], record->payload,
                   record->payload_length);
        }
        previous = offset;
        offset += header_size + record->payload_length;
    }
    return WZ_RESULT_OK;
}

static wz_dword_t wz_tzx_read_le24(const wz_byte_t* bytes)
{
    return (wz_dword_t)bytes[0] | ((wz_dword_t)bytes[1] << 8u) |
        ((wz_dword_t)bytes[2] << 16u);
}

static wz_dword_t wz_tzx_read_le32(const wz_byte_t* bytes)
{
    return (wz_dword_t)bytes[0] | ((wz_dword_t)bytes[1] << 8u) |
        ((wz_dword_t)bytes[2] << 16u) | ((wz_dword_t)bytes[3] << 24u);
}

static wz_result_t wz_tzx_block_size(const wz_byte_t* data,
                                     size_t remaining,
                                     size_t* block_length,
                                     wz_tzx_disposition_t* disposition)
{
    wz_byte_t id;
    size_t fixed_length = 0u;
    size_t variable_length = 0u;

    if (data == 0 || block_length == 0 || disposition == 0 || remaining < 1u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    id = data[0u];
    *disposition = WZ_TZX_UNSUPPORTED;
    switch (id) {
    case 0x10u:
        fixed_length = 5u;
        *disposition = WZ_TZX_SUPPORTED;
        if (remaining >= fixed_length) {
            variable_length = wz_read_le16(&data[3u]);
        }
        break;
    case 0x11u:
        fixed_length = 19u;
        *disposition = WZ_TZX_SUPPORTED;
        if (remaining >= fixed_length) {
            variable_length = (size_t)wz_tzx_read_le24(&data[16u]);
        }
        break;
    case 0x12u:
        fixed_length = 5u;
        *disposition = WZ_TZX_SUPPORTED;
        break;
    case 0x13u:
        *disposition = WZ_TZX_SUPPORTED;
        if (remaining < 2u) return WZ_RESULT_PARSE_ERROR;
        fixed_length = 2u;
        if ((size_t)data[1u] > (SIZE_MAX - fixed_length) / 2u) return WZ_RESULT_PARSE_ERROR;
        variable_length = (size_t)data[1u] * 2u;
        break;
    case 0x14u:
        fixed_length = 11u;
        *disposition = WZ_TZX_SUPPORTED;
        if (remaining >= fixed_length) variable_length = (size_t)wz_tzx_read_le24(&data[8u]);
        break;
    case 0x15u:
        fixed_length = 9u;
        *disposition = WZ_TZX_SUPPORTED;
        if (remaining >= fixed_length) variable_length = (size_t)wz_tzx_read_le24(&data[6u]);
        break;
    case 0x18u:
    case 0x19u:
        fixed_length = 5u;
        *disposition = WZ_TZX_SUPPORTED;
        if (remaining >= fixed_length) variable_length = (size_t)data[1u] |
            ((size_t)data[2u] << 8u) | ((size_t)data[3u] << 16u) |
            ((size_t)data[4u] << 24u);
        break;
    case 0x20u:
        fixed_length = 3u;
        *disposition = WZ_TZX_SUPPORTED;
        break;
    case 0x21u:
        fixed_length = 2u;
        *disposition = WZ_TZX_IGNORED;
        if (remaining >= fixed_length) variable_length = data[1u];
        break;
    case 0x22u:
        fixed_length = 1u;
        *disposition = WZ_TZX_IGNORED;
        break;
    case 0x23u:
        fixed_length = 3u;
        *disposition = WZ_TZX_SUPPORTED;
        break;
    case 0x24u:
        fixed_length = 3u;
        *disposition = WZ_TZX_SUPPORTED;
        break;
    case 0x25u:
    case 0x27u:
        fixed_length = 1u;
        *disposition = WZ_TZX_SUPPORTED;
        break;
    case 0x26u:
        *disposition = WZ_TZX_SUPPORTED;
        if (remaining < 3u) return WZ_RESULT_PARSE_ERROR;
        fixed_length = 3u;
        if ((size_t)wz_read_le16(&data[1u]) > (SIZE_MAX - fixed_length) / 2u) {
            return WZ_RESULT_PARSE_ERROR;
        }
        variable_length = (size_t)wz_read_le16(&data[1u]) * 2u;
        break;
    case 0x28u:
        fixed_length = 3u;
        *disposition = WZ_TZX_SUPPORTED;
        if (remaining >= fixed_length) variable_length = wz_read_le16(&data[1u]);
        break;
    case 0x2au:
    case 0x2bu:
        fixed_length = 5u;
        *disposition = WZ_TZX_SUPPORTED;
        if (remaining >= fixed_length) variable_length = (size_t)data[1u] |
            ((size_t)data[2u] << 8u) | ((size_t)data[3u] << 16u) |
            ((size_t)data[4u] << 24u);
        break;
    case 0x30u:
        fixed_length = 2u;
        *disposition = WZ_TZX_IGNORED;
        if (remaining >= fixed_length) variable_length = data[1u];
        break;
    case 0x31u:
        fixed_length = 3u;
        *disposition = WZ_TZX_IGNORED;
        if (remaining >= fixed_length) variable_length = data[2u];
        break;
    case 0x32u:
        fixed_length = 3u;
        *disposition = WZ_TZX_IGNORED;
        if (remaining >= fixed_length) variable_length = wz_read_le16(&data[1u]);
        break;
    case 0x33u:
        *disposition = WZ_TZX_IGNORED;
        if (remaining < 2u) {
            return WZ_RESULT_PARSE_ERROR;
        }
        fixed_length = 2u;
        variable_length = (size_t)data[1u] * 3u;
        break;
    case 0x35u:
        fixed_length = 21u;
        *disposition = WZ_TZX_IGNORED;
        if (remaining >= fixed_length) variable_length = (size_t)data[17u] |
            ((size_t)data[18u] << 8u) | ((size_t)data[19u] << 16u) |
            ((size_t)data[20u] << 24u);
        break;
    case 0x40u:
        fixed_length = 3u;
        *disposition = WZ_TZX_IGNORED;
        if (remaining >= fixed_length) variable_length = wz_read_le16(&data[1u]);
        break;
    case 0x5au:
        fixed_length = 10u;
        *disposition = WZ_TZX_IGNORED;
        break;
    default:
        return WZ_RESULT_UNSUPPORTED_OPERATION;
    }
    if (fixed_length > remaining || variable_length > remaining - fixed_length) {
        return WZ_RESULT_PARSE_ERROR;
    }
    *block_length = fixed_length + variable_length;
    return WZ_RESULT_OK;
}

static bool wz_tzx_target_is_valid(int32_t current,
                                   int16_t relative,
                                   size_t block_count)
{
    int32_t target = current + (int32_t)relative;
    return target >= 0 && (size_t)target < block_count;
}

static wz_result_t wz_tzx_jump_target(size_t current,
                                      const wz_tzx_block_t* block,
                                      size_t block_count,
                                      size_t* target)
{
    int32_t resolved;

    if (block == 0 || target == 0 || block->data == 0 ||
        block->data_length < 2u || current > (size_t)INT32_MAX ||
        block_count > (size_t)INT32_MAX ||
        !wz_tzx_target_is_valid((int32_t)current,
                                (int16_t)wz_read_le16(block->data),
                                block_count)) {
        return WZ_RESULT_PARSE_ERROR;
    }
    resolved = (int32_t)current + (int16_t)wz_read_le16(block->data);
    *target = (size_t)resolved;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tzx_call_target(size_t current,
                                      const wz_tzx_block_t* block,
                                      size_t call_index,
                                      size_t block_count,
                                      size_t* target)
{
    size_t calls;
    int16_t relative;
    int32_t resolved;

    if (block == 0 || target == 0 || block->data == 0 ||
        block->data_length < 2u) return WZ_RESULT_PARSE_ERROR;
    calls = wz_read_le16(block->data);
    if (calls == 0u || call_index >= calls || calls > (SIZE_MAX - 2u) / 2u ||
        block->data_length < 2u + calls * 2u || current > (size_t)INT32_MAX ||
        block_count > (size_t)INT32_MAX) return WZ_RESULT_PARSE_ERROR;
    relative = (int16_t)wz_read_le16(block->data + 2u + call_index * 2u);
    if (!wz_tzx_target_is_valid((int32_t)current, relative, block_count)) {
        return WZ_RESULT_PARSE_ERROR;
    }
    resolved = (int32_t)current + relative;
    *target = (size_t)resolved;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tzx_select_target(size_t current,
                                        const wz_tzx_block_t* block,
                                        size_t block_count,
                                        size_t* target)
{
    size_t offset = 3u;
    size_t selections;
    size_t selected_target = 0u;

    if (block == 0 || target == 0 || block->data == 0 ||
        block->data_length < 3u) return WZ_RESULT_PARSE_ERROR;
    if ((size_t)wz_read_le16(block->data) != block->data_length - 2u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    selections = block->data[2u];
    if (selections == 0u || current > (size_t)INT32_MAX ||
        block_count > (size_t)INT32_MAX) return WZ_RESULT_PARSE_ERROR;
    for (size_t selection = 0u; selection < selections; ++selection) {
        int16_t relative;
        int32_t resolved;
        size_t text_length;

        if (offset > block->data_length || block->data_length - offset < 3u) {
            return WZ_RESULT_PARSE_ERROR;
        }
        relative = (int16_t)wz_read_le16(block->data + offset);
        text_length = (size_t)block->data[offset + 2u];
        offset += 3u;
        if (text_length > block->data_length - offset) {
            return WZ_RESULT_PARSE_ERROR;
        }
        if (!wz_tzx_target_is_valid((int32_t)current, relative, block_count)) {
            return WZ_RESULT_PARSE_ERROR;
        }
        resolved = (int32_t)current + relative;
        if (selection == 0u) selected_target = (size_t)resolved;
        offset += text_length;
    }
    if (offset != block->data_length) return WZ_RESULT_PARSE_ERROR;
    *target = selected_target;
    return WZ_RESULT_OK;
}

enum { WZ_TZX_MAX_LOOP_DEPTH = 64u };

wz_result_t wz_tape_parse_tzx(const wz_byte_t* data,
                              size_t length,
                              wz_tzx_block_t* blocks,
                              size_t capacity,
                              size_t* count)
{
    size_t offset = 10u;
    size_t found = 0u;
    size_t index;
    int loop_depth = 0;

    if (data == 0 || count == 0 || length < 10u ||
        memcmp(data, "ZXTape!\x1a", 8u) != 0 || data[8u] != 1u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    while (offset < length) {
        size_t block_length;
        wz_tzx_disposition_t disposition;
        wz_result_t result = wz_tzx_block_size(&data[offset], length - offset,
                                                &block_length, &disposition);
        if (result != WZ_RESULT_OK) return result;
        if (found == SIZE_MAX) return WZ_RESULT_PARSE_ERROR;
        ++found;
        offset += block_length;
    }
    if (offset != length) return WZ_RESULT_PARSE_ERROR;
    *count = found;
    if (blocks == 0 || capacity < found) return WZ_RESULT_BUFFER_TOO_SMALL;

    offset = 10u;
    for (index = 0u; index < found; ++index) {
        size_t block_length;
        wz_tzx_disposition_t disposition;
        if (wz_tzx_block_size(&data[offset], length - offset, &block_length,
                              &disposition) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
        blocks[index].offset = offset;
        blocks[index].block_length = block_length;
        blocks[index].block_id = data[offset];
        blocks[index].disposition = disposition;
        blocks[index].data = &data[offset + 1u];
        blocks[index].data_length = block_length - 1u;
        if (data[offset] == 0x23u && !wz_tzx_target_is_valid((int32_t)index,
                (int16_t)wz_read_le16(&data[offset + 1u]), found)) {
            return WZ_RESULT_PARSE_ERROR;
        }
        if (data[offset] == 0x24u) ++loop_depth;
        if (data[offset] == 0x25u && loop_depth-- == 0) return WZ_RESULT_PARSE_ERROR;
        offset += block_length;
    }
    if (loop_depth != 0) return WZ_RESULT_PARSE_ERROR;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tzx_add_segments(size_t* total, size_t amount)
{
    if (total == 0 || amount > SIZE_MAX - *total) {
        return WZ_RESULT_PARSE_ERROR;
    }
    *total += amount;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tzx_append_segment(wz_tape_segment_t* segments,
                                         size_t capacity,
                                         size_t* index,
                                         wz_dword_t tstates,
                                         wz_dword_t ticks_per_tstate,
                                         wz_byte_t level)
{
    wz_master_tick_t duration;
    if (segments == 0 || index == 0 || *index >= capacity ||
        wz_tape_tap_duration(tstates, ticks_per_tstate, &duration) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    segments[*index].duration = duration;
    segments[*index].ear_level = level;
    ++*index;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tzx_csw_decode(const wz_tzx_block_t* block,
                                     const wz_byte_t** encoded,
                                     size_t* encoded_length,
                                     wz_byte_t** owned,
                                     size_t* pulse_count)
{
    size_t offset = 14u;
    size_t found = 0u;
    wz_dword_t declared;
    wz_byte_t* expanded = 0;

    if (block == 0 || encoded == 0 || encoded_length == 0 || owned == 0 ||
        pulse_count == 0 || block->data == 0 || block->data_length < offset ||
        (block->data[9u] != 1u && block->data[9u] != 2u) ||
        wz_tzx_read_le24(block->data + 6u) == 0u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    declared = wz_tzx_read_le32(block->data + 10u);
    if (declared == 0u) return WZ_RESULT_PARSE_ERROR;
    if (block->data[9u] == 1u) {
        *encoded = block->data + offset;
        *encoded_length = block->data_length - offset;
        *owned = 0;
    } else {
        size_t maximum;
        size_t capacity;
        uint64_t maximum64;
        z_stream stream;
        int status;

        maximum64 = (uint64_t)declared * 5u;
        if (maximum64 > SIZE_MAX) {
            return WZ_RESULT_PARSE_ERROR;
        }
        maximum = (size_t)maximum64;
        capacity = maximum < 4096u ? maximum : 4096u;
        expanded = (wz_byte_t*)malloc(capacity);
        if (expanded == 0) return WZ_RESULT_OUT_OF_MEMORY;
        memset(&stream, 0, sizeof(stream));
        if (block->data_length - offset > UINT_MAX) {
            free(expanded);
            return WZ_RESULT_PARSE_ERROR;
        }
        stream.next_in = (Bytef*)(block->data + offset);
        stream.avail_in = (uInt)(block->data_length - offset);
        if (inflateInit2(&stream, 15 + 32) != Z_OK) {
            free(expanded);
            return WZ_RESULT_PARSE_ERROR;
        }
        for (;;) {
            stream.next_out = expanded + stream.total_out;
            stream.avail_out = (uInt)(capacity - stream.total_out);
            status = inflate(&stream, Z_FINISH);
            if (status == Z_STREAM_END) break;
            if (status != Z_OK && status != Z_BUF_ERROR) {
                inflateEnd(&stream);
                free(expanded);
                return WZ_RESULT_PARSE_ERROR;
            }
            if (stream.avail_out != 0u || stream.total_out == maximum) {
                inflateEnd(&stream);
                free(expanded);
                return WZ_RESULT_PARSE_ERROR;
            }
            capacity = capacity > maximum / 2u ? maximum : capacity * 2u;
            {
                wz_byte_t* resized = (wz_byte_t*)realloc(expanded, capacity);
                if (resized == 0) {
                    inflateEnd(&stream);
                    free(expanded);
                    return WZ_RESULT_OUT_OF_MEMORY;
                }
                expanded = resized;
            }
        }
        if (stream.total_out == 0u || stream.avail_in != 0u ||
            stream.total_out > maximum) {
            inflateEnd(&stream);
            free(expanded);
            return WZ_RESULT_PARSE_ERROR;
        }
        inflateEnd(&stream);
        *encoded = expanded;
        *encoded_length = stream.total_out;
        *owned = expanded;
    }
    for (offset = 0u; offset < *encoded_length && found < (size_t)declared;) {
        size_t run;
        if ((*encoded)[offset++] != 0u) {
            run = 1u;
        } else {
            if (offset > *encoded_length - 4u) {
                free(*owned);
                *owned = 0;
                return WZ_RESULT_PARSE_ERROR;
            }
            run = (size_t)(*encoded)[offset] |
                ((size_t)(*encoded)[offset + 1u] << 8u) |
                ((size_t)(*encoded)[offset + 2u] << 16u) |
                ((size_t)(*encoded)[offset + 3u] << 24u);
            offset += 4u;
            if (run == 0u) {
                free(*owned);
                *owned = 0;
                return WZ_RESULT_PARSE_ERROR;
            }
        }
        ++found;
    }
    if (found != (size_t)declared || offset != *encoded_length) {
        free(*owned);
        *owned = 0;
        return WZ_RESULT_PARSE_ERROR;
    }
    *pulse_count = found;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tzx_count_csw(const wz_tzx_block_t* block,
                                    size_t* amount)
{
    const wz_byte_t* encoded;
    wz_byte_t* owned;
    size_t pulses;

    size_t encoded_length;

    if (amount == 0 || wz_tzx_csw_decode(block, &encoded, &encoded_length,
            &owned, &pulses) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    (void)encoded;
    free(owned);
    if (pulses == SIZE_MAX) return WZ_RESULT_PARSE_ERROR;
    *amount = pulses + (wz_read_le16(block->data + 4u) == 0u ? 0u : 1u);
    return WZ_RESULT_OK;
}

static wz_result_t wz_tzx_expand_csw(const wz_tzx_block_t* block,
                                     wz_dword_t ticks_per_tstate,
                                     wz_tape_segment_t* segments,
                                     size_t capacity,
                                     size_t* index,
                                     wz_byte_t* level)
{
    const wz_byte_t* encoded;
    wz_byte_t* owned;
    size_t encoded_length;
    size_t offset = 0u;
    size_t pulses;
    wz_dword_t sample_rate;
    wz_result_t result = WZ_RESULT_OK;

    if (level == 0 || wz_tzx_csw_decode(block, &encoded, &encoded_length,
            &owned, &pulses) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    sample_rate = wz_tzx_read_le24(block->data + 6u);
    for (size_t pulse = 0u; pulse < pulses; ++pulse) {
        uint64_t samples;
        wz_dword_t tstates;
        if (encoded[offset] != 0u) {
            samples = encoded[offset++];
        } else {
            ++offset;
            samples = (uint64_t)encoded[offset] |
                ((uint64_t)encoded[offset + 1u] << 8u) |
                ((uint64_t)encoded[offset + 2u] << 16u) |
                ((uint64_t)encoded[offset + 3u] << 24u);
            offset += 4u;
        }
        samples = (samples * 3500000u + sample_rate / 2u) / sample_rate;
        if (samples == 0u || samples > UINT32_MAX) {
            result = WZ_RESULT_PARSE_ERROR;
            goto cleanup;
        }
        tstates = (wz_dword_t)samples;
        if (wz_tzx_append_segment(segments, capacity, index, tstates,
                                  ticks_per_tstate, *level) != WZ_RESULT_OK) {
            result = WZ_RESULT_PARSE_ERROR;
            goto cleanup;
        }
        *level ^= 1u;
    }
    if (wz_read_le16(block->data + 4u) != 0u) {
        if (wz_tzx_append_segment(segments, capacity, index,
                (wz_dword_t)wz_read_le16(block->data + 4u) * 3500u,
                ticks_per_tstate, 0u) != WZ_RESULT_OK) {
            result = WZ_RESULT_PARSE_ERROR;
            goto cleanup;
        }
        *level = 0u;
    }
cleanup:
    free(owned);
    return result;
}

typedef struct {
    size_t pilot_count;
    size_t pilot_pulses;
    size_t pilot_alphabet;
    size_t data_count;
    size_t data_pulses;
    size_t data_alphabet;
    size_t pilot_defs;
    size_t pilot_prle;
    size_t data_defs;
    size_t data_stream;
    size_t data_stream_bytes;
} wz_tzx_generalized_info_t;

static wz_result_t wz_tzx_generalized_layout(const wz_tzx_block_t* block,
                                             wz_tzx_generalized_info_t* info)
{
    size_t pilot_symbol_size;
    size_t data_symbol_size;
    size_t offset;
    size_t bits_per_symbol = 0u;
    size_t value;

    if (block == 0 || info == 0 || block->data == 0 ||
        block->data_length < 18u ||
        (size_t)wz_tzx_read_le32(block->data) != block->data_length - 4u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    info->pilot_count = (size_t)wz_tzx_read_le32(block->data + 6u);
    info->pilot_pulses = block->data[10u];
    info->pilot_alphabet = block->data[11u] == 0u ? 256u : block->data[11u];
    info->data_count = (size_t)wz_tzx_read_le32(block->data + 12u);
    info->data_pulses = block->data[16u];
    info->data_alphabet = block->data[17u] == 0u ? 256u : block->data[17u];
    if ((info->pilot_count != 0u &&
            (info->pilot_pulses == 0u || info->pilot_alphabet == 0u)) ||
        (info->data_count != 0u &&
            (info->data_pulses == 0u || info->data_alphabet == 0u))) {
        return WZ_RESULT_PARSE_ERROR;
    }
    pilot_symbol_size = (size_t)info->pilot_pulses * 2u + 1u;
    data_symbol_size = (size_t)info->data_pulses * 2u + 1u;
    if (info->pilot_alphabet > SIZE_MAX / pilot_symbol_size) {
        return WZ_RESULT_PARSE_ERROR;
    }
    info->pilot_defs = 18u;
    offset = info->pilot_defs;
    if (info->pilot_count != 0u) {
        if (info->pilot_alphabet > (SIZE_MAX - offset) / pilot_symbol_size) {
            return WZ_RESULT_PARSE_ERROR;
        }
        offset += info->pilot_alphabet * pilot_symbol_size;
        if (info->pilot_count > (SIZE_MAX - offset) / 3u) {
            return WZ_RESULT_PARSE_ERROR;
        }
        info->pilot_prle = offset;
        offset += info->pilot_count * 3u;
    } else {
        info->pilot_prle = offset;
    }
    if (info->data_alphabet > SIZE_MAX / data_symbol_size) {
        return WZ_RESULT_PARSE_ERROR;
    }
    info->data_defs = offset;
    if (info->data_alphabet > (SIZE_MAX - offset) / data_symbol_size) {
        return WZ_RESULT_PARSE_ERROR;
    }
    offset += info->data_alphabet * data_symbol_size;
    value = info->data_alphabet - 1u;
    while (value != 0u) {
        ++bits_per_symbol;
        value >>= 1u;
    }
    if (bits_per_symbol != 0u &&
        info->data_count > (SIZE_MAX - 7u) / bits_per_symbol) {
        return WZ_RESULT_PARSE_ERROR;
    }
    value = (info->data_count * bits_per_symbol + 7u) / 8u;
    if (offset > block->data_length || value > block->data_length - offset) {
        return WZ_RESULT_PARSE_ERROR;
    }
    info->data_stream = offset;
    info->data_stream_bytes = value;
    if (offset + value != block->data_length) return WZ_RESULT_PARSE_ERROR;
    return WZ_RESULT_OK;
}

static size_t wz_tzx_generalized_data_symbol(const wz_tzx_block_t* block,
                                             const wz_tzx_generalized_info_t* info,
                                             size_t index)
{
    size_t bits = 0u;
    size_t value = info->data_alphabet - 1u;
    size_t bit_offset;
    size_t symbol = 0u;

    while (value != 0u) {
        ++bits;
        value >>= 1u;
    }
    if (bits == 0u) return 0u;
    bit_offset = index * bits;
    for (size_t bit = 0u; bit < bits; ++bit) {
        size_t absolute = bit_offset + bit;
        symbol = (symbol << 1u) |
            ((block->data[info->data_stream + absolute / 8u] >>
              (7u - absolute % 8u)) & 1u);
    }
    return symbol;
}

static wz_result_t wz_tzx_generalized_symbol_pulses(
    const wz_tzx_block_t* block, size_t definitions_offset, size_t definition,
    size_t max_pulses, size_t alphabet, size_t* pulses)
{
    size_t size = max_pulses * 2u + 1u;
    size_t offset;

    if (pulses == 0 || definition >= alphabet ||
        alphabet > SIZE_MAX / size) return WZ_RESULT_PARSE_ERROR;
    if (definition > (SIZE_MAX - definitions_offset) / size) {
        return WZ_RESULT_PARSE_ERROR;
    }
    offset = definitions_offset + definition * size;
    *pulses = 0u;
    while (*pulses < max_pulses &&
           wz_read_le16(block->data + offset + 1u + *pulses * 2u) != 0u) {
        ++*pulses;
    }
    return WZ_RESULT_OK;
}

static wz_result_t wz_tzx_count_generalized(const wz_tzx_block_t* block,
                                            size_t* amount)
{
    wz_tzx_generalized_info_t info;
    size_t total = 0u;

    if (amount == 0 || wz_tzx_generalized_layout(block, &info) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    for (size_t index = 0u; index < info.pilot_count; ++index) {
        size_t symbol = block->data[info.pilot_prle + index * 3u];
        size_t repetitions = (size_t)wz_read_le16(
            block->data + info.pilot_prle + index * 3u + 1u);
        size_t pulses = 0u;
        if (repetitions == 0u || wz_tzx_generalized_symbol_pulses(block,
                info.pilot_defs, symbol, info.pilot_pulses,
                info.pilot_alphabet, &pulses) != WZ_RESULT_OK ||
            (pulses != 0u && repetitions > (SIZE_MAX - total) / pulses)) {
            return WZ_RESULT_PARSE_ERROR;
        }
        total += pulses * repetitions;
    }
    for (size_t index = 0u; index < info.data_count; ++index) {
        size_t pulses = 0u;
        if (wz_tzx_generalized_symbol_pulses(block, info.data_defs,
                wz_tzx_generalized_data_symbol(block, &info, index),
                info.data_pulses, info.data_alphabet, &pulses) != WZ_RESULT_OK ||
            pulses > SIZE_MAX - total) return WZ_RESULT_PARSE_ERROR;
        total += pulses;
    }
    if (wz_read_le16(block->data + 4u) != 0u) {
        if (total == SIZE_MAX) return WZ_RESULT_PARSE_ERROR;
        ++total;
    }
    *amount = total;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tzx_expand_generalized(const wz_tzx_block_t* block,
                                             wz_dword_t ticks_per_tstate,
                                             wz_tape_segment_t* segments,
                                             size_t capacity,
                                             size_t* index,
                                             wz_byte_t* level)
{
    wz_tzx_generalized_info_t info;

    if (level == 0 || wz_tzx_generalized_layout(block, &info) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    for (size_t stream_index = 0u; stream_index < info.pilot_count; ++stream_index) {
        size_t symbol = block->data[info.pilot_prle + stream_index * 3u];
        size_t repetitions = (size_t)wz_read_le16(
            block->data + info.pilot_prle + stream_index * 3u + 1u);
        for (size_t repetition = 0u; repetition < repetitions; ++repetition) {
            size_t symbol_size = (size_t)info.pilot_pulses * 2u + 1u;
            size_t symbol_offset = info.pilot_defs + symbol * symbol_size;
            wz_byte_t flags = block->data[symbol_offset] & 3u;
            if (flags == 0u) *level ^= 1u;
            else if (flags == 2u) *level = 0u;
            else if (flags == 3u) *level = 1u;
            for (size_t pulse = 0u; pulse < info.pilot_pulses; ++pulse) {
                wz_word_t duration = wz_read_le16(block->data + symbol_offset + 1u + pulse * 2u);
                if (duration == 0u) break;
                if (wz_tzx_append_segment(segments, capacity, index, duration,
                                          ticks_per_tstate, *level) != WZ_RESULT_OK) {
                    return WZ_RESULT_PARSE_ERROR;
                }
                *level ^= 1u;
            }
        }
    }
    for (size_t stream_index = 0u; stream_index < info.data_count; ++stream_index) {
        size_t symbol = wz_tzx_generalized_data_symbol(block, &info, stream_index);
        size_t symbol_size = (size_t)info.data_pulses * 2u + 1u;
        size_t symbol_offset = info.data_defs + symbol * symbol_size;
        wz_byte_t flags = block->data[symbol_offset] & 3u;
        if (flags == 0u) *level ^= 1u;
        else if (flags == 2u) *level = 0u;
        else if (flags == 3u) *level = 1u;
        for (size_t pulse = 0u; pulse < info.data_pulses; ++pulse) {
            wz_word_t duration = wz_read_le16(block->data + symbol_offset + 1u + pulse * 2u);
            if (duration == 0u) break;
            if (wz_tzx_append_segment(segments, capacity, index, duration,
                                      ticks_per_tstate, *level) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            *level ^= 1u;
        }
    }
    if (wz_read_le16(block->data + 4u) != 0u) {
        if (wz_tzx_append_segment(segments, capacity, index,
                (wz_dword_t)wz_read_le16(block->data + 4u) * 3500u,
                ticks_per_tstate, 0u) != WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
        *level = 0u;
    }
    return WZ_RESULT_OK;
}

static bool wz_tzx_is_ignored_metadata(wz_byte_t block_id)
{
    return block_id == 0x21u || block_id == 0x22u || block_id == 0x30u ||
           block_id == 0x31u || block_id == 0x32u || block_id == 0x33u ||
           block_id == 0x35u || block_id == 0x40u || block_id == 0x5au;
}

static wz_result_t wz_tzx_count_standard_speed(const wz_tzx_block_t* block,
                                               size_t* amount)
{
    size_t data_length;
    size_t pilot_count;
    size_t pulses;

    if (block == 0 || amount == 0 || block->data == 0 || block->data_length < 4u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    data_length = wz_read_le16(block->data + 2u);
    if (data_length > block->data_length - 4u) return WZ_RESULT_PARSE_ERROR;
    pilot_count = block->data[4u] == 0u ? 8063u : 3223u;
    if (data_length > (SIZE_MAX - pilot_count - 3u) / 16u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    pulses = pilot_count + 2u + data_length * 16u;
    if (wz_read_le16(block->data) != 0u) ++pulses;
    *amount = pulses;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tzx_expand_standard_speed(const wz_tzx_block_t* block,
                                                wz_dword_t ticks_per_tstate,
                                                wz_tape_segment_t* segments,
                                                size_t capacity,
                                                size_t* index,
                                                wz_byte_t* level)
{
    size_t data_length;
    size_t pilot_count;
    wz_byte_t* current_level = level;

    if (block == 0 || block->data == 0 || index == 0 || current_level == 0 ||
        block->data_length < 4u) return WZ_RESULT_PARSE_ERROR;
    data_length = wz_read_le16(block->data + 2u);
    if (data_length > block->data_length - 4u) return WZ_RESULT_PARSE_ERROR;
    pilot_count = block->data[4u] == 0u ? 8063u : 3223u;
    for (size_t pulse = 0u; pulse < pilot_count; ++pulse) {
        if (wz_tzx_append_segment(segments, capacity, index, 2168u,
                                  ticks_per_tstate, *current_level) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
        *current_level ^= 1u;
    }
    if (wz_tzx_append_segment(segments, capacity, index, 667u, ticks_per_tstate,
                              *current_level) != WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
    *current_level ^= 1u;
    if (wz_tzx_append_segment(segments, capacity, index, 735u, ticks_per_tstate,
                              *current_level) != WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
    *current_level ^= 1u;
    for (size_t byte_index = 0u; byte_index < data_length; ++byte_index) {
        for (unsigned bit = 0u; bit < 8u; ++bit) {
            wz_dword_t duration = (block->data[4u + byte_index] &
                                   (wz_byte_t)(0x80u >> bit)) != 0u ? 1710u : 855u;
            if (wz_tzx_append_segment(segments, capacity, index, duration,
                                      ticks_per_tstate, *current_level) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            *current_level ^= 1u;
            if (wz_tzx_append_segment(segments, capacity, index, duration,
                                      ticks_per_tstate, *current_level) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            *current_level ^= 1u;
        }
    }
    if (wz_read_le16(block->data) != 0u &&
        wz_tzx_append_segment(segments, capacity, index,
                              (wz_dword_t)wz_read_le16(block->data) * 3500u,
                              ticks_per_tstate, 0u) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    *current_level = 0u;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tzx_count_turbo(const wz_tzx_block_t* block, size_t* amount)
{
    size_t data_length;
    size_t used_bits;
    size_t pilot_count;

    if (block == 0 || amount == 0 || block->data == 0 || block->data_length < 18u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    data_length = (size_t)wz_tzx_read_le24(block->data + 15u);
    if (data_length == 0u || data_length > block->data_length - 18u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    used_bits = block->data[12u] == 0u ? 8u : block->data[12u];
    if (used_bits > 8u) return WZ_RESULT_PARSE_ERROR;
    pilot_count = (size_t)wz_read_le16(block->data + 10u);
    if (data_length > (SIZE_MAX - used_bits) / 8u) return WZ_RESULT_PARSE_ERROR;
    *amount = pilot_count + 2u + (data_length - 1u) * 16u + used_bits * 2u;
    if (wz_read_le16(block->data + 13u) != 0u) ++*amount;
    return WZ_RESULT_OK;
}

static wz_result_t wz_tzx_expand_turbo(const wz_tzx_block_t* block,
                                       wz_dword_t ticks_per_tstate,
                                       wz_tape_segment_t* segments,
                                       size_t capacity,
                                       size_t* index,
                                       wz_byte_t* level)
{
    size_t data_length = (size_t)wz_tzx_read_le24(block->data + 15u);
    size_t used_bits = block->data[12u] == 0u ? 8u : block->data[12u];
    size_t pilot_count = (size_t)wz_read_le16(block->data + 10u);
    wz_byte_t* current_level = level;

    for (size_t pulse = 0u; pulse < pilot_count; ++pulse) {
        if (wz_tzx_append_segment(segments, capacity, index,
                                  wz_read_le16(block->data), ticks_per_tstate,
                                  *current_level) != WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
        *current_level ^= 1u;
    }
    if (wz_tzx_append_segment(segments, capacity, index, wz_read_le16(block->data + 2u),
                              ticks_per_tstate, *current_level) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    *current_level ^= 1u;
    if (wz_tzx_append_segment(segments, capacity, index, wz_read_le16(block->data + 4u),
                              ticks_per_tstate, *current_level) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    *current_level ^= 1u;
    for (size_t byte_index = 0u; byte_index < data_length; ++byte_index) {
        unsigned bits = byte_index + 1u == data_length ? (unsigned)used_bits : 8u;
        for (unsigned bit = 0u; bit < bits; ++bit) {
            wz_dword_t duration = (block->data[18u + byte_index] &
                (wz_byte_t)(0x80u >> bit)) != 0u ? wz_read_le16(block->data + 8u) :
                wz_read_le16(block->data + 6u);
            if (wz_tzx_append_segment(segments, capacity, index, duration,
                                      ticks_per_tstate, *current_level) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            *current_level ^= 1u;
            if (wz_tzx_append_segment(segments, capacity, index, duration,
                                      ticks_per_tstate, *current_level) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            *current_level ^= 1u;
        }
    }
    if (wz_read_le16(block->data + 13u) != 0u &&
        wz_tzx_append_segment(segments, capacity, index,
            (wz_dword_t)wz_read_le16(block->data + 13u) * 3500u,
            ticks_per_tstate, 0u) != WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
    *current_level = 0u;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_expand_tzx_timing(const wz_tzx_block_t* blocks,
                                      size_t block_count,
                                      wz_dword_t master_ticks_per_tstate,
                                      wz_tape_segment_t* segments,
                                      size_t capacity,
                                      size_t* count)
{
    size_t required = 0u;
    size_t index = 0u;
    size_t steps = 0u;
    size_t loop_depth = 0u;
    size_t loop_starts[WZ_TZX_MAX_LOOP_DEPTH];
    wz_word_t loop_remaining[WZ_TZX_MAX_LOOP_DEPTH];
    size_t call_origins[WZ_TZX_MAX_LOOP_DEPTH];
    size_t call_returns[WZ_TZX_MAX_LOOP_DEPTH];
    size_t call_next[WZ_TZX_MAX_LOOP_DEPTH];
    size_t call_depth = 0u;
    size_t step_limit = block_count > SIZE_MAX / 65535u ? SIZE_MAX :
        block_count * 65535u;
    wz_byte_t level = 1u;

    if (blocks == 0 || block_count == 0u || count == 0 ||
        master_ticks_per_tstate == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (size_t block_index = 0u; block_index < block_count; ++block_index) {
        const wz_tzx_block_t* block = &blocks[block_index];
        size_t amount = 0u;
        size_t target;
        if (++steps > step_limit) return WZ_RESULT_PARSE_ERROR;
        if (block->data == 0 && block->data_length != 0u) {
            return WZ_RESULT_INVALID_ARGUMENT;
        }
        switch (block->block_id) {
        case 0x10u:
            if (wz_tzx_count_standard_speed(block, &amount) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            break;
        case 0x11u:
            if (wz_tzx_count_turbo(block, &amount) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            break;
        case 0x12u:
            if (block->data_length < 4u) return WZ_RESULT_PARSE_ERROR;
            amount = (size_t)wz_read_le16(block->data + 2u);
            break;
        case 0x13u:
            if (block->data_length < 1u) return WZ_RESULT_PARSE_ERROR;
            amount = (size_t)block->data[0u];
            if (amount > (SIZE_MAX - 1u) / 2u ||
                block->data_length < 1u + amount * 2u) return WZ_RESULT_PARSE_ERROR;
            break;
        case 0x14u: {
            size_t data_length;
            size_t used_bits;
            if (block->data_length < 10u) return WZ_RESULT_PARSE_ERROR;
            data_length = (size_t)wz_tzx_read_le24(block->data + 7u);
            if (data_length == 0u || data_length > block->data_length - 10u) {
                return WZ_RESULT_PARSE_ERROR;
            }
            used_bits = block->data[4u] == 0u ? 8u : block->data[4u];
            if (used_bits > 8u || data_length > (SIZE_MAX - used_bits) / 8u) {
                return WZ_RESULT_PARSE_ERROR;
            }
            amount = (data_length - 1u) * 16u + used_bits * 2u;
            if (wz_read_le16(block->data + 5u) != 0u) ++amount;
            break;
        }
        case 0x15u: {
            size_t data_length;
            size_t used_bits;
            if (block->data_length < 8u) return WZ_RESULT_PARSE_ERROR;
            data_length = (size_t)wz_tzx_read_le24(block->data + 5u);
            if (data_length == 0u || data_length > block->data_length - 8u) {
                return WZ_RESULT_PARSE_ERROR;
            }
            used_bits = block->data[4u] == 0u ? 8u : block->data[4u];
            if (used_bits > 8u || data_length > (SIZE_MAX - used_bits) / 8u ||
                wz_read_le16(block->data) == 0u) return WZ_RESULT_PARSE_ERROR;
            amount = (data_length - 1u) * 8u + used_bits;
            if (wz_read_le16(block->data + 2u) != 0u) ++amount;
            break;
        }
        case 0x18u:
            if (wz_tzx_count_csw(block, &amount) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            break;
        case 0x19u:
            if (wz_tzx_count_generalized(block, &amount) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            break;
        case 0x20u:
            if (block->data_length < 2u) return WZ_RESULT_PARSE_ERROR;
            amount = wz_read_le16(block->data) == 0u ? 0u : 1u;
            break;
        case 0x23u:
            if (wz_tzx_jump_target(block_index, block, block_count, &target) !=
                    WZ_RESULT_OK || target <= block_index) {
                return WZ_RESULT_PARSE_ERROR;
            }
            block_index = target - 1u;
            break;
        case 0x24u:
            if (block->data_length < 2u || wz_read_le16(block->data) == 0u ||
                loop_depth >= WZ_TZX_MAX_LOOP_DEPTH) return WZ_RESULT_PARSE_ERROR;
            loop_starts[loop_depth] = block_index + 1u;
            loop_remaining[loop_depth] = wz_read_le16(block->data);
            ++loop_depth;
            break;
        case 0x25u:
            if (block->data_length != 0u || loop_depth == 0u) {
                return WZ_RESULT_PARSE_ERROR;
            }
            if (--loop_remaining[loop_depth - 1u] != 0u) {
                block_index = loop_starts[loop_depth - 1u] - 1u;
            } else {
                --loop_depth;
            }
            break;
        case 0x26u:
            if (call_depth >= WZ_TZX_MAX_LOOP_DEPTH || block->data_length < 2u ||
                wz_read_le16(block->data) == 0u ||
                wz_tzx_call_target(block_index, block, 0u, block_count, &target) !=
                    WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
            call_origins[call_depth] = block_index;
            call_returns[call_depth] = block_index + 1u;
            call_next[call_depth] = 1u;
            ++call_depth;
            block_index = target - 1u;
            break;
        case 0x28u:
            if (wz_tzx_select_target(block_index, block, block_count, &target) !=
                    WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
            block_index = target == 0u ? block_count - 1u : target - 1u;
            break;
        case 0x27u:
            if (block->data_length != 0u || call_depth == 0u) {
                return WZ_RESULT_PARSE_ERROR;
            }
            if (call_next[call_depth - 1u] < (size_t)wz_read_le16(
                    blocks[call_origins[call_depth - 1u]].data)) {
                if (wz_tzx_call_target(call_origins[call_depth - 1u],
                        &blocks[call_origins[call_depth - 1u]],
                        call_next[call_depth - 1u], block_count, &target) !=
                        WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
                ++call_next[call_depth - 1u];
                block_index = target - 1u;
            } else {
                block_index = call_returns[call_depth - 1u] - 1u;
                --call_depth;
            }
            break;
        case 0x2au: {
            size_t declared_length;
            if (block->data_length < 4u) return WZ_RESULT_PARSE_ERROR;
            declared_length = (size_t)block->data[0u] |
                ((size_t)block->data[1u] << 8u) |
                ((size_t)block->data[2u] << 16u) |
                ((size_t)block->data[3u] << 24u);
            if (declared_length > block->data_length - 4u) {
                return WZ_RESULT_PARSE_ERROR;
            }
            break;
        }
        case 0x2bu:
            if (block->data_length < 5u) return WZ_RESULT_PARSE_ERROR;
            if ((size_t)wz_tzx_read_le24(block->data) > block->data_length - 4u) {
                return WZ_RESULT_PARSE_ERROR;
            }
            break;
        default:
            if (!wz_tzx_is_ignored_metadata(block->block_id)) {
                return WZ_RESULT_UNSUPPORTED_OPERATION;
            }
            break;
        }
        if (wz_tzx_add_segments(&required, amount) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
        if (block->block_id == 0x2au) break;
    }
    if (loop_depth != 0u || call_depth != 0u) return WZ_RESULT_PARSE_ERROR;
    *count = required;
    if (segments == 0 || capacity < required) return WZ_RESULT_BUFFER_TOO_SMALL;
    steps = 0u;
    loop_depth = 0u;
    call_depth = 0u;
    for (size_t block_index = 0u; block_index < block_count; ++block_index) {
        const wz_tzx_block_t* block = &blocks[block_index];
        size_t target;
        if (++steps > step_limit) return WZ_RESULT_PARSE_ERROR;
        if (block->block_id == 0x10u) {
            if (wz_tzx_expand_standard_speed(block, master_ticks_per_tstate,
                                             segments, capacity, &index, &level) !=
                    WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
        } else if (block->block_id == 0x11u) {
            if (wz_tzx_expand_turbo(block, master_ticks_per_tstate, segments,
                                    capacity, &index, &level) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
        } else if (block->block_id == 0x12u) {
            wz_word_t duration = wz_read_le16(block->data);
            wz_word_t repetitions = wz_read_le16(block->data + 2u);
            for (wz_word_t repetition = 0u; repetition < repetitions; ++repetition) {
                if (wz_tzx_append_segment(segments, capacity, &index, duration,
                                          master_ticks_per_tstate, level) != WZ_RESULT_OK) {
                    return WZ_RESULT_PARSE_ERROR;
                }
                level ^= 1u;
            }
        } else if (block->block_id == 0x13u) {
            for (size_t pulse = 0u; pulse < (size_t)block->data[0u]; ++pulse) {
                wz_word_t duration = wz_read_le16(block->data + 1u + pulse * 2u);
                if (wz_tzx_append_segment(segments, capacity, &index, duration,
                                          master_ticks_per_tstate, level) != WZ_RESULT_OK) {
                    return WZ_RESULT_PARSE_ERROR;
                }
                level ^= 1u;
            }
        } else if (block->block_id == 0x14u) {
            size_t data_length = (size_t)wz_tzx_read_le24(block->data + 7u);
            size_t used_bits = block->data[4u] == 0u ? 8u : block->data[4u];
            wz_word_t zero_duration = wz_read_le16(block->data);
            wz_word_t one_duration = wz_read_le16(block->data + 2u);
            for (size_t byte_index = 0u; byte_index < data_length; ++byte_index) {
                unsigned bits = byte_index + 1u == data_length ? (unsigned)used_bits : 8u;
                for (unsigned bit = 0u; bit < bits; ++bit) {
                    wz_dword_t duration = (block->data[10u + byte_index] &
                        (wz_byte_t)(0x80u >> bit)) != 0u ? one_duration : zero_duration;
                    if (wz_tzx_append_segment(segments, capacity, &index, duration,
                                              master_ticks_per_tstate, level) != WZ_RESULT_OK) {
                        return WZ_RESULT_PARSE_ERROR;
                    }
                    level ^= 1u;
                    if (wz_tzx_append_segment(segments, capacity, &index, duration,
                                              master_ticks_per_tstate, level) != WZ_RESULT_OK) {
                        return WZ_RESULT_PARSE_ERROR;
                    }
                    level ^= 1u;
                }
            }
            if (wz_read_le16(block->data + 5u) != 0u) {
                if (wz_tzx_append_segment(segments, capacity, &index,
                    (wz_dword_t)wz_read_le16(block->data + 5u) * 3500u,
                    master_ticks_per_tstate, 0u) != WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
                level = 0u;
            }
        } else if (block->block_id == 0x15u) {
            size_t data_length = (size_t)wz_tzx_read_le24(block->data + 5u);
            size_t used_bits = block->data[4u] == 0u ? 8u : block->data[4u];
            wz_word_t duration = wz_read_le16(block->data);
            for (size_t byte_index = 0u; byte_index < data_length; ++byte_index) {
                unsigned bits = byte_index + 1u == data_length ? (unsigned)used_bits : 8u;
                for (unsigned bit = 0u; bit < bits; ++bit) {
                    level = (block->data[8u + byte_index] &
                             (wz_byte_t)(0x80u >> bit)) != 0u ? 1u : 0u;
                    if (wz_tzx_append_segment(segments, capacity, &index, duration,
                                              master_ticks_per_tstate, level) != WZ_RESULT_OK) {
                        return WZ_RESULT_PARSE_ERROR;
                    }
                }
            }
            if (wz_read_le16(block->data + 2u) != 0u) {
                if (wz_tzx_append_segment(segments, capacity, &index,
                    (wz_dword_t)wz_read_le16(block->data + 2u) * 3500u,
                    master_ticks_per_tstate, 0u) != WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
                level = 0u;
            }
        } else if (block->block_id == 0x18u) {
            if (wz_tzx_expand_csw(block, master_ticks_per_tstate, segments,
                                  capacity, &index, &level) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
        } else if (block->block_id == 0x19u) {
            if (wz_tzx_expand_generalized(block, master_ticks_per_tstate, segments,
                                           capacity, &index, &level) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
        } else if (block->block_id == 0x2bu) {
            level = block->data[4u] == 0u ? 0u : 1u;
        } else if (block->block_id == 0x23u) {
            if (wz_tzx_jump_target(block_index, block, block_count, &target) !=
                    WZ_RESULT_OK || target <= block_index) {
                return WZ_RESULT_PARSE_ERROR;
            }
            block_index = target - 1u;
        } else if (block->block_id == 0x24u) {
            if (block->data_length < 2u || wz_read_le16(block->data) == 0u ||
                loop_depth >= WZ_TZX_MAX_LOOP_DEPTH) return WZ_RESULT_PARSE_ERROR;
            loop_starts[loop_depth] = block_index + 1u;
            loop_remaining[loop_depth] = wz_read_le16(block->data);
            ++loop_depth;
        } else if (block->block_id == 0x25u) {
            if (block->data_length != 0u || loop_depth == 0u) {
                return WZ_RESULT_PARSE_ERROR;
            }
            if (--loop_remaining[loop_depth - 1u] != 0u) {
                block_index = loop_starts[loop_depth - 1u] - 1u;
            } else {
                --loop_depth;
            }
        } else if (block->block_id == 0x26u) {
            if (call_depth >= WZ_TZX_MAX_LOOP_DEPTH || block->data_length < 2u ||
                wz_read_le16(block->data) == 0u ||
                wz_tzx_call_target(block_index, block, 0u, block_count, &target) !=
                    WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
            call_origins[call_depth] = block_index;
            call_returns[call_depth] = block_index + 1u;
            call_next[call_depth] = 1u;
            ++call_depth;
            block_index = target - 1u;
        } else if (block->block_id == 0x28u) {
            if (wz_tzx_select_target(block_index, block, block_count, &target) !=
                    WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
            block_index = target == 0u ? block_count - 1u : target - 1u;
        } else if (block->block_id == 0x27u) {
            if (block->data_length != 0u || call_depth == 0u) {
                return WZ_RESULT_PARSE_ERROR;
            }
            if (call_next[call_depth - 1u] < (size_t)wz_read_le16(
                    blocks[call_origins[call_depth - 1u]].data)) {
                if (wz_tzx_call_target(call_origins[call_depth - 1u],
                        &blocks[call_origins[call_depth - 1u]],
                        call_next[call_depth - 1u], block_count, &target) !=
                        WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
                ++call_next[call_depth - 1u];
                block_index = target - 1u;
            } else {
                block_index = call_returns[call_depth - 1u] - 1u;
                --call_depth;
            }
        } else if (block->block_id == 0x20u) {
            wz_dword_t tstates = (wz_dword_t)wz_read_le16(block->data) * 3500u;
            if (wz_tzx_append_segment(segments, capacity, &index, tstates,
                                      master_ticks_per_tstate, 0u) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            level = 0u;
        } else if (block->block_id == 0x2au) {
            break;
        } else if (!wz_tzx_is_ignored_metadata(block->block_id)) {
            return WZ_RESULT_UNSUPPORTED_OPERATION;
        }
    }
    if (loop_depth != 0u || call_depth != 0u) return WZ_RESULT_PARSE_ERROR;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_parse_native_tap(const wz_byte_t* data,
                                     size_t length,
                                     wz_native_tap_record_t* records,
                                     size_t capacity,
                                     size_t* count)
{
    size_t offset;
    size_t previous = 0u;
    size_t found = 0u;
    size_t guard = 0u;

    if (count == 0 || !wz_tape_is_native_tap(data, length)) {
        return WZ_RESULT_PARSE_ERROR;
    }
    offset = (size_t)data[0u] | ((size_t)data[1u] << 8u) |
        ((size_t)data[2u] << 16u) | ((size_t)data[3u] << 24u);
    if (offset < 12u || offset >= length) {
        return WZ_RESULT_PARSE_ERROR;
    }
    while (offset != UINT32_MAX) {
        wz_native_tap_record_t record;
        size_t next;
        size_t header_size;

        if (++guard > length ||
            wz_tape_native_record_header(data, length, offset, previous,
                                         &next, &header_size, &record) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
        (void)header_size;
        if (found == SIZE_MAX) {
            return WZ_RESULT_PARSE_ERROR;
        }
        ++found;
        previous = offset;
        offset = next;
    }
    *count = found;
    if (records == 0 || capacity < found) {
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    offset = (size_t)data[0u] | ((size_t)data[1u] << 8u) |
        ((size_t)data[2u] << 16u) | ((size_t)data[3u] << 24u);
    previous = 0u;
    for (size_t index = 0u; index < found; ++index) {
        size_t next;
        size_t header_size;
        if (wz_tape_native_record_header(data, length, offset, previous,
                                         &next, &header_size, &records[index]) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
        previous = offset;
        offset = next;
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_validate(const wz_tape_segment_t* segments,
                             size_t segment_count)
{
    size_t index;

    if (segments == 0 || segment_count == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (index = 0u; index < segment_count; ++index) {
        if (segments[index].duration == 0u || segments[index].ear_level > 1u) {
            return WZ_RESULT_INVALID_ARGUMENT;
        }
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_mount(wz_tape_t* tape,
                          const wz_tape_segment_t* segments,
                          size_t segment_count)
{
    if (tape == 0 || wz_tape_validate(segments, segment_count) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    tape->segments = segments;
    tape->segment_count = segment_count;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_state_init(wz_tape_state_t* state,
                               const wz_tape_t* tape)
{
    if (state == 0 || tape == 0 || tape->segments == 0 ||
        tape->segment_count == 0u ||
        wz_tape_validate(tape->segments, tape->segment_count) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    state->tape = tape;
    state->segment_index = 0u;
    state->segment_elapsed = 0u;
    state->ear_level = tape->segments[0u].ear_level;
    state->motor_on = false;
    state->at_end = false;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_state_advance(wz_tape_state_t* state,
                                  wz_master_tick_t ticks)
{
    wz_master_tick_t remaining = ticks;

    if (state == 0 || state->tape == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (!state->motor_on || state->at_end || remaining == 0u) {
        return WZ_RESULT_OK;
    }
    while (remaining != 0u && !state->at_end) {
        const wz_tape_segment_t* segment =
            &state->tape->segments[state->segment_index];
        wz_master_tick_t available = segment->duration - state->segment_elapsed;

        if (remaining < available) {
            state->segment_elapsed += remaining;
            remaining = 0u;
            continue;
        }
        remaining -= available;
        ++state->segment_index;
        state->segment_elapsed = 0u;
        if (state->segment_index >= state->tape->segment_count) {
            state->segment_index = state->tape->segment_count - 1u;
            state->ear_level = segment->ear_level;
            state->at_end = true;
        } else {
            state->ear_level = state->tape->segments[state->segment_index].ear_level;
        }
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_state_rewind(wz_tape_state_t* state)
{
    if (state == 0 || state->tape == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    state->segment_index = 0u;
    state->segment_elapsed = 0u;
    state->ear_level = state->tape->segments[0u].ear_level;
    state->at_end = false;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_state_set_motor(wz_tape_state_t* state, bool motor_on)
{
    if (state == 0 || state->tape == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    state->motor_on = motor_on && !state->at_end;
    return WZ_RESULT_OK;
}

wz_byte_t wz_tape_state_ear_level(const wz_tape_state_t* state)
{
    return state == 0 ? 0u : state->ear_level;
}

bool wz_tape_state_at_end(const wz_tape_state_t* state)
{
    return state != 0 && state->at_end;
}

static wz_dword_t wz_wav_le32(const wz_byte_t* value)
{
    return (wz_dword_t)value[0] | ((wz_dword_t)value[1] << 8u) |
        ((wz_dword_t)value[2] << 16u) | ((wz_dword_t)value[3] << 24u);
}

static wz_result_t wz_wav_scan(const wz_byte_t* data, size_t length,
                               wz_dword_t* sample_rate, wz_word_t* channels,
                               wz_word_t* bits, size_t* sample_offset,
                               size_t* sample_bytes)
{
    size_t offset = 12u;
    bool format_found = false;
    bool data_found = false;

    if (data == 0 || length < 12u || memcmp(data, "RIFF", 4u) != 0 ||
        memcmp(data + 8u, "WAVE", 4u) != 0 ||
        (size_t)wz_wav_le32(data + 4u) > length - 8u || sample_rate == 0 ||
        channels == 0 || bits == 0 || sample_offset == 0 || sample_bytes == 0) {
        return WZ_RESULT_PARSE_ERROR;
    }
    while (offset <= length && length - offset >= 8u) {
        size_t chunk_size = (size_t)wz_wav_le32(data + offset + 4u);
        size_t chunk_data = offset + 8u;
        size_t padded;
        if (chunk_size > length - chunk_data || chunk_size > SIZE_MAX - 1u) {
            return WZ_RESULT_PARSE_ERROR;
        }
        padded = chunk_size + (chunk_size & 1u);
        if (padded > length - chunk_data) {
            return WZ_RESULT_PARSE_ERROR;
        }
        if (memcmp(data + offset, "fmt ", 4u) == 0) {
            if (chunk_size < 16u || wz_read_le16(data + chunk_data) != 1u) {
                return WZ_RESULT_UNSUPPORTED_OPERATION;
            }
            *channels = wz_read_le16(data + chunk_data + 2u);
            *sample_rate = wz_wav_le32(data + chunk_data + 4u);
            *bits = wz_read_le16(data + chunk_data + 14u);
            size_t format_frame_bytes = (size_t)*channels * (*bits / 8u);
            wz_qword_t expected_byte_rate = (wz_qword_t)*sample_rate * format_frame_bytes;
            if (*channels == 0u || *sample_rate == 0u ||
                (*bits != 8u && *bits != 16u) || format_frame_bytes == 0u ||
                format_frame_bytes > UINT16_MAX || expected_byte_rate > UINT32_MAX ||
                wz_read_le16(data + chunk_data + 12u) != format_frame_bytes ||
                wz_wav_le32(data + chunk_data + 8u) != expected_byte_rate) {
                return WZ_RESULT_UNSUPPORTED_OPERATION;
            }
            format_found = true;
        } else if (memcmp(data + offset, "data", 4u) == 0) {
            *sample_offset = chunk_data;
            *sample_bytes = chunk_size;
            data_found = true;
        }
        if (format_found && data_found) return WZ_RESULT_OK;
        if (padded > length - chunk_data) return WZ_RESULT_PARSE_ERROR;
        offset = chunk_data + padded;
    }
    return format_found && data_found ? WZ_RESULT_OK : WZ_RESULT_PARSE_ERROR;
}

static int wz_wav_sample_level(const wz_byte_t* sample, wz_word_t bits,
                               wz_word_t channels, wz_byte_t threshold,
                               wz_byte_t hysteresis, int previous)
{
    int64_t value = 0;
    for (wz_word_t channel = 0u; channel < channels; ++channel) {
        int sample_value = bits == 8u ? (int)sample[channel] - 128 :
            (int)(int16_t)wz_read_le16(sample + channel * 2u);
        value += sample_value;
    }
    value /= (int)channels;
    if (bits == 8u) {
        int64_t high = (int)threshold + (int)hysteresis;
        int64_t low = (int)threshold - (int)hysteresis;
        if (previous == 0 && value > high - 128) return 1;
        if (previous == 1 && value < low - 128) return 0;
    } else {
        int64_t high = ((int64_t)threshold - 128) * 256 + (int64_t)hysteresis * 256;
        int64_t low = ((int64_t)threshold - 128) * 256 - (int64_t)hysteresis * 256;
        if (previous == 0 && value > high) return 1;
        if (previous == 1 && value < low) return 0;
    }
    return previous;
}

wz_result_t wz_tape_parse_wav_pcm(const wz_byte_t* data, size_t length,
                                  wz_dword_t master_ticks_per_second,
                                  wz_byte_t threshold, wz_byte_t hysteresis,
                                  wz_tape_segment_t* segments, size_t capacity,
                                  size_t* count)
{
    wz_dword_t sample_rate;
    wz_word_t channels;
    wz_word_t bits;
    size_t sample_offset;
    size_t sample_bytes;
    size_t frame_bytes;
    size_t frames;
    size_t required = 0u;
    int level = 0;
    size_t segment_start = 0u;

    if (count == 0 || master_ticks_per_second == 0u ||
        wz_wav_scan(data, length, &sample_rate, &channels, &bits,
                    &sample_offset, &sample_bytes) != WZ_RESULT_OK) {
        return WZ_RESULT_PARSE_ERROR;
    }
    frame_bytes = (size_t)channels * (bits / 8u);
    if (frame_bytes == 0u || sample_bytes % frame_bytes != 0u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    frames = sample_bytes / frame_bytes;
    if (frames == 0u || frames > (SIZE_MAX - 1u) / 2u ||
        (wz_qword_t)frames > UINT64_MAX / master_ticks_per_second) {
        return WZ_RESULT_PARSE_ERROR;
    }
    for (size_t frame = 0u; frame < frames; ++frame) {
        size_t tick = (size_t)(((wz_qword_t)frame * master_ticks_per_second) / sample_rate);
        int next = wz_wav_sample_level(data + sample_offset + frame * frame_bytes,
                                       bits, channels, threshold, hysteresis, level);
        if (next != level && tick > segment_start) {
            if (required == SIZE_MAX) return WZ_RESULT_PARSE_ERROR;
            ++required;
            level = next;
            segment_start = tick;
        }
    }
    if (((wz_qword_t)frames * master_ticks_per_second) / sample_rate > segment_start) {
        if (required == SIZE_MAX) return WZ_RESULT_PARSE_ERROR;
        ++required;
    }
    if (required == 0u) return WZ_RESULT_PARSE_ERROR;
    *count = required;
    if (segments == 0 || capacity < required) return WZ_RESULT_BUFFER_TOO_SMALL;
    level = 0;
    segment_start = 0u;
    size_t output_index = 0u;
    for (size_t frame = 0u; frame <= frames; ++frame) {
        size_t tick = (size_t)(((wz_qword_t)frame * master_ticks_per_second) / sample_rate);
        if (frame < frames) {
            int next = wz_wav_sample_level(data + sample_offset + frame * frame_bytes,
                                           bits, channels, threshold, hysteresis, level);
            if (next == level) continue;
            if (tick > segment_start) {
                segments[output_index].duration = tick - segment_start;
                segments[output_index].ear_level = (wz_byte_t)level;
                ++output_index;
            }
            level = next;
            segment_start = tick;
        } else if (tick > segment_start) {
            segments[output_index].duration = tick - segment_start;
            segments[output_index].ear_level = (wz_byte_t)level;
            ++output_index;
        }
    }
    return output_index == required ? WZ_RESULT_OK : WZ_RESULT_PARSE_ERROR;
}
