/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_tape_manager.h"

#include <stdarg.h>
#include <stdio.h>

static const wz_tape_manager_maintenance_operation_t maintenance_operations[] = {
    {WZ_TAPE_MANAGER_MAINTENANCE_EXCLUDE, "media.tape.native.exclude",
     "Exclude", false, "requires-native-tape"},
    {WZ_TAPE_MANAGER_MAINTENANCE_LINEARIZE, "media.tape.native.linearize",
     "Linearize", false, "requires-native-tape"},
    {WZ_TAPE_MANAGER_MAINTENANCE_IMPLODE, "media.tape.native.implode",
     "Implode", false, "requires-native-tape"},
    {WZ_TAPE_MANAGER_MAINTENANCE_DECOMPRESS, "media.tape.native.decompress",
     "Decompress", false, "requires-native-tape"},
    {WZ_TAPE_MANAGER_MAINTENANCE_EFFICIENCY, "media.tape.native.efficiency",
     "Compression Efficiency", false, "requires-native-tape"}
};

#include <stdint.h>

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

static bool valid_block(const wz_tap_block_t* block)
{
    return block != 0 && block->data != 0 && block->length != 0u;
}

wz_result_t wz_tape_manager_edit_init(wz_tape_manager_edit_t* edit,
                                      wz_tap_block_t* blocks,
                                      size_t count, size_t capacity)
{
    if (edit == 0 || blocks == 0 || count == 0u || count > capacity) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (size_t index = 0u; index < count; ++index) {
        if (!valid_block(&blocks[index])) return WZ_RESULT_INVALID_ARGUMENT;
    }
    edit->blocks = blocks;
    edit->count = count;
    edit->capacity = capacity;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_manager_reorder(wz_tape_manager_edit_t* edit,
                                    size_t from, size_t to)
{
    wz_tap_block_t moved;
    if (edit == 0 || edit->blocks == 0 || from >= edit->count ||
        to >= edit->count) return WZ_RESULT_INVALID_ARGUMENT;
    if (from == to) return WZ_RESULT_OK;
    moved = edit->blocks[from];
    if (from < to) {
        for (size_t index = from; index < to; ++index)
            edit->blocks[index] = edit->blocks[index + 1u];
    } else {
        for (size_t index = from; index > to; --index)
            edit->blocks[index] = edit->blocks[index - 1u];
    }
    edit->blocks[to] = moved;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_manager_change_position(wz_tape_manager_edit_t* edit,
                                            size_t from, size_t to)
{
    return wz_tape_manager_reorder(edit, from, to);
}

wz_result_t wz_tape_manager_add_block(wz_tape_manager_edit_t* edit,
                                      wz_tap_block_t block, size_t position)
{
    if (edit == 0 || edit->blocks == 0 || !valid_block(&block) ||
        position > edit->count || edit->count >= edit->capacity) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (size_t index = edit->count; index > position; --index)
        edit->blocks[index] = edit->blocks[index - 1u];
    edit->blocks[position] = block;
    ++edit->count;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_manager_delete_block(wz_tape_manager_edit_t* edit,
                                         size_t position)
{
    if (edit == 0 || edit->blocks == 0 || edit->count == 0u ||
        position >= edit->count) return WZ_RESULT_INVALID_ARGUMENT;
    for (size_t index = position; index + 1u < edit->count; ++index)
        edit->blocks[index] = edit->blocks[index + 1u];
    --edit->count;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_manager_edit_block(wz_tape_manager_edit_t* edit,
                                       size_t position, wz_tap_block_t block)
{
    if (edit == 0 || edit->blocks == 0 || position >= edit->count ||
        !valid_block(&block)) return WZ_RESULT_INVALID_ARGUMENT;
    edit->blocks[position] = block;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_manager_extract_block(const wz_tape_manager_edit_t* edit,
                                          size_t position, wz_tap_block_t* output)
{
    if (edit == 0 || edit->blocks == 0 || output == 0 ||
        position >= edit->count) return WZ_RESULT_INVALID_ARGUMENT;
    *output = edit->blocks[position];
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_manager_copy_block_to_new(const wz_tape_manager_edit_t* edit,
                                              size_t position, wz_tap_block_t* output)
{
    return wz_tape_manager_extract_block(edit, position, output);
}

size_t wz_tape_manager_maintenance_count(void)
{
    return sizeof(maintenance_operations) / sizeof(maintenance_operations[0]);
}

const wz_tape_manager_maintenance_operation_t*
wz_tape_manager_maintenance_at(wz_tape_manager_format_t format, size_t index)
{
    static wz_tape_manager_maintenance_operation_t native_operation;
    const wz_tape_manager_maintenance_operation_t* operation;

    if (index >= wz_tape_manager_maintenance_count()) return 0;
    operation = &maintenance_operations[index];
    if (format != WZ_TAPE_MANAGER_FORMAT_NATIVE_TAP) return operation;
    native_operation = *operation;
    native_operation.available = true;
    native_operation.unavailable_reason = 0;
    return &native_operation;
}

static const char* format_name(wz_tape_manager_format_t format)
{
    switch (format) {
    case WZ_TAPE_MANAGER_FORMAT_STANDARD_TAP: return "standard TAP";
    case WZ_TAPE_MANAGER_FORMAT_NATIVE_TAP: return "Warajevo native TAP";
    case WZ_TAPE_MANAGER_FORMAT_TZX: return "TZX";
    default: return "unknown";
    }
}

static const char* state_name(wz_tape_manager_state_t state)
{
    switch (state) {
    case WZ_TAPE_MANAGER_STATE_STOPPED: return "stopped";
    case WZ_TAPE_MANAGER_STATE_LOADING: return "loading";
    case WZ_TAPE_MANAGER_STATE_PLAYING: return "playing";
    default: return "unknown";
    }
}

static wz_result_t append_report(char* output, size_t capacity,
                                 size_t* used, const char* format, ...)
{
    int written;
    va_list arguments;

    if (*used >= capacity) return WZ_RESULT_BUFFER_TOO_SMALL;
    va_start(arguments, format);
    written = vsnprintf(output + *used, capacity - *used, format, arguments);
    va_end(arguments);
    if (written < 0 || (size_t)written >= capacity - *used)
        return WZ_RESULT_BUFFER_TOO_SMALL;
    *used += (size_t)written;
    return WZ_RESULT_OK;
}

wz_result_t wz_tape_manager_export_report(
    const wz_tape_manager_view_t* view,
    const wz_tape_manager_block_t* blocks,
    size_t block_count,
    char* output,
    size_t capacity,
    size_t* length)
{
    size_t used = 0u;

    if (view == 0 || blocks == 0 || block_count == 0u || output == 0 ||
        capacity == 0u || length == 0 || view->source_identity == 0 ||
        view->loading_mode == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (size_t index = 0u; index < block_count; ++index) {
        if (blocks[index].type == 0)
            return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (append_report(output, capacity, &used,
            "Tape Report\nSource: %s\nFormat: %s\nLoading mode: %s\n"
            "State: %s\nCurrent block: %zu\nCurrent position: %zu\n"
            "Selected block: %zu\nBlocks: %zu\n",
            view->source_identity, format_name(view->format),
            view->loading_mode, state_name(view->state), view->current_block,
            view->current_position, view->selected_block, block_count) != WZ_RESULT_OK) {
        *length = 0u;
        return WZ_RESULT_BUFFER_TOO_SMALL;
    }
    for (size_t index = 0u; index < block_count; ++index) {
        if (append_report(output, capacity, &used,
                "Block %zu: type=%s logical=%zu stored=%zu flags=%u selected=%s metadata=%s\n",
                index, blocks[index].type, blocks[index].logical_length,
                blocks[index].stored_length, blocks[index].flags,
                blocks[index].selected ? "yes" : "no",
                blocks[index].metadata == 0 ? "" : blocks[index].metadata) != WZ_RESULT_OK) {
            *length = 0u;
            return WZ_RESULT_BUFFER_TOO_SMALL;
        }
    }
    *length = used;
    return WZ_RESULT_OK;
}
