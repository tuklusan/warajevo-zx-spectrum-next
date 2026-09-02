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
                                      ticks_per_tstate, *current_level) != WZ_RESULT_OK ||
                wz_tzx_append_segment(segments, capacity, index, duration,
                                      ticks_per_tstate, (wz_byte_t)(*current_level ^ 1u)) !=
                    WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
            *current_level ^= 1u;
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
                              ticks_per_tstate, *current_level) != WZ_RESULT_OK ||
        wz_tzx_append_segment(segments, capacity, index, wz_read_le16(block->data + 4u),
                              ticks_per_tstate, (wz_byte_t)(*current_level ^ 1u)) !=
            WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
    *current_level ^= 1u;
    *current_level ^= 1u;
    for (size_t byte_index = 0u; byte_index < data_length; ++byte_index) {
        unsigned bits = byte_index + 1u == data_length ? (unsigned)used_bits : 8u;
        for (unsigned bit = 0u; bit < bits; ++bit) {
            wz_dword_t duration = (block->data[18u + byte_index] &
                (wz_byte_t)(0x80u >> bit)) != 0u ? wz_read_le16(block->data + 8u) :
                wz_read_le16(block->data + 6u);
            if (wz_tzx_append_segment(segments, capacity, index, duration,
                                      ticks_per_tstate, *current_level) != WZ_RESULT_OK ||
                wz_tzx_append_segment(segments, capacity, index, duration,
                                      ticks_per_tstate, (wz_byte_t)(*current_level ^ 1u)) !=
                    WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
            *current_level ^= 1u;
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
    wz_byte_t level = 1u;

    if (blocks == 0 || block_count == 0u || count == 0 ||
        master_ticks_per_tstate == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (size_t block_index = 0u; block_index < block_count; ++block_index) {
        const wz_tzx_block_t* block = &blocks[block_index];
        size_t amount = 0u;
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
        case 0x20u:
            if (block->data_length < 2u) return WZ_RESULT_PARSE_ERROR;
            amount = wz_read_le16(block->data) == 0u ? 0u : 1u;
            break;
        default:
            return WZ_RESULT_UNSUPPORTED_OPERATION;
        }
        if (wz_tzx_add_segments(&required, amount) != WZ_RESULT_OK) {
            return WZ_RESULT_PARSE_ERROR;
        }
    }
    *count = required;
    if (segments == 0 || capacity < required) return WZ_RESULT_BUFFER_TOO_SMALL;
    for (size_t block_index = 0u; block_index < block_count; ++block_index) {
        const wz_tzx_block_t* block = &blocks[block_index];
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
                                              master_ticks_per_tstate, level) != WZ_RESULT_OK ||
                        wz_tzx_append_segment(segments, capacity, &index, duration,
                                              master_ticks_per_tstate, (wz_byte_t)(level ^ 1u)) !=
                            WZ_RESULT_OK) return WZ_RESULT_PARSE_ERROR;
                    level ^= 1u;
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
        } else {
            wz_dword_t tstates = (wz_dword_t)wz_read_le16(block->data) * 3500u;
            if (wz_tzx_append_segment(segments, capacity, &index, tstates,
                                      master_ticks_per_tstate, 0u) != WZ_RESULT_OK) {
                return WZ_RESULT_PARSE_ERROR;
            }
            level = 0u;
        }
    }
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
