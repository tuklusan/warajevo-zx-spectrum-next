/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_tape_manager.h"

static wz_result_t prepare_output(size_t required,
                                  wz_tape_manager_block_t* output,
                                  size_t capacity, size_t* count)
{
    if (count == 0 || required == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *count = required;
    return output != 0 && capacity >= required ? WZ_RESULT_OK :
        WZ_RESULT_BUFFER_TOO_SMALL;
}

static void initialize_block(wz_tape_manager_block_t* block, size_t ordinal,
                             const char* type, size_t logical_length,
                             size_t stored_length, unsigned int flags,
                             const char* metadata, size_t selected_block)
{
    block->ordinal = ordinal;
    block->type = type;
    block->logical_length = logical_length;
    block->stored_length = stored_length;
    block->flags = flags;
    block->metadata = metadata;
    block->selected = ordinal == selected_block;
}

wz_result_t wz_tape_manager_view_init(
    wz_tape_manager_view_t* view, const char* source_identity,
    wz_tape_manager_format_t format, const char* loading_mode,
    size_t current_block, size_t current_position,
    wz_tape_manager_state_t state, size_t selected_block)
{
    if (view == 0 || source_identity == 0 || source_identity[0] == '\0' ||
        loading_mode == 0 || loading_mode[0] == '\0' ||
        format > WZ_TAPE_MANAGER_FORMAT_TZX ||
        state > WZ_TAPE_MANAGER_STATE_PLAYING) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    view->source_identity = source_identity;
    view->format = format;
    view->loading_mode = loading_mode;
    view->current_block = current_block;
    view->current_position = current_position;
    view->state = state;
    view->selected_block = selected_block;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_manager_blocks_from_tap(
    const wz_tap_block_t* blocks, size_t block_count, size_t selected_block,
    wz_tape_manager_block_t* output, size_t capacity, size_t* count)
{
    if (blocks == 0 || block_count == 0u) return WZ_RESULT_INVALID_ARGUMENT;
    if (prepare_output(block_count, output, capacity, count) != WZ_RESULT_OK)
        return WZ_RESULT_BUFFER_TOO_SMALL;
    for (size_t index = 0u; index < block_count; ++index) {
        if (blocks[index].data == 0 || blocks[index].length == 0u)
            return WZ_RESULT_INVALID_ARGUMENT;
        initialize_block(&output[index], index, "TAP data block",
                         blocks[index].length, blocks[index].length + 2u,
                         0u, 0, selected_block);
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_manager_blocks_from_native(
    const wz_native_tap_record_t* records, size_t record_count,
    size_t selected_block, wz_tape_manager_block_t* output,
    size_t capacity, size_t* count)
{
    if (records == 0 || record_count == 0u) return WZ_RESULT_INVALID_ARGUMENT;
    if (prepare_output(record_count, output, capacity, count) != WZ_RESULT_OK)
        return WZ_RESULT_BUFFER_TOO_SMALL;
    for (size_t index = 0u; index < record_count; ++index) {
        const wz_native_tap_record_t* record = &records[index];
        if (record->payload == 0 || record->payload_length == 0u)
            return WZ_RESULT_INVALID_ARGUMENT;
        initialize_block(&output[index], index, "Warajevo native record",
                         record->decompressed_length != 0u ?
                             record->decompressed_length : record->payload_length,
                         record->stored_length, record->flag,
                         record->record_type == 0u ? "data" : "typed",
                         selected_block);
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_manager_blocks_from_tzx(
    const wz_tzx_block_t* blocks, size_t block_count, size_t selected_block,
    wz_tape_manager_block_t* output, size_t capacity, size_t* count)
{
    if (blocks == 0 || block_count == 0u) return WZ_RESULT_INVALID_ARGUMENT;
    if (prepare_output(block_count, output, capacity, count) != WZ_RESULT_OK)
        return WZ_RESULT_BUFFER_TOO_SMALL;
    for (size_t index = 0u; index < block_count; ++index) {
        const wz_tzx_block_t* block = &blocks[index];
        if (block->data == 0 || block->data_length == 0u)
            return WZ_RESULT_INVALID_ARGUMENT;
        initialize_block(&output[index], index, "TZX block",
                         block->data_length, block->block_length,
                         (unsigned int)block->block_id,
                         block->disposition == WZ_TZX_SUPPORTED ?
                             "supported" : "not-expanded", selected_block);
    }
    return WZ_RESULT_OK;
}
