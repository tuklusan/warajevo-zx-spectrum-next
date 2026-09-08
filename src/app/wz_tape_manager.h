/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_TAPE_MANAGER_H
#define WZ_APP_WZ_TAPE_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

#include "core/wz_tape.h"

typedef enum {
    WZ_TAPE_MANAGER_FORMAT_STANDARD_TAP = 0,
    WZ_TAPE_MANAGER_FORMAT_NATIVE_TAP,
    WZ_TAPE_MANAGER_FORMAT_TZX
} wz_tape_manager_format_t;

typedef enum {
    WZ_TAPE_MANAGER_STATE_STOPPED = 0,
    WZ_TAPE_MANAGER_STATE_LOADING,
    WZ_TAPE_MANAGER_STATE_PLAYING
} wz_tape_manager_state_t;

typedef struct {
    const char* source_identity;
    wz_tape_manager_format_t format;
    const char* loading_mode;
    size_t current_block;
    size_t current_position;
    wz_tape_manager_state_t state;
    size_t selected_block;
} wz_tape_manager_view_t;

typedef struct {
    size_t ordinal;
    const char* type;
    size_t logical_length;
    size_t stored_length;
    unsigned int flags;
    const char* metadata;
    bool selected;
} wz_tape_manager_block_t;

typedef struct {
    wz_tap_block_t* blocks;
    size_t count;
    size_t capacity;
} wz_tape_manager_edit_t;

wz_result_t wz_tape_manager_view_init(
    wz_tape_manager_view_t* view,
    const char* source_identity,
    wz_tape_manager_format_t format,
    const char* loading_mode,
    size_t current_block,
    size_t current_position,
    wz_tape_manager_state_t state,
    size_t selected_block);

wz_result_t wz_tape_manager_blocks_from_tap(
    const wz_tap_block_t* blocks, size_t block_count, size_t selected_block,
    wz_tape_manager_block_t* output, size_t capacity, size_t* count);
wz_result_t wz_tape_manager_blocks_from_native(
    const wz_native_tap_record_t* records, size_t record_count, size_t selected_block,
    wz_tape_manager_block_t* output, size_t capacity, size_t* count);
wz_result_t wz_tape_manager_blocks_from_tzx(
    const wz_tzx_block_t* blocks, size_t block_count, size_t selected_block,
    wz_tape_manager_block_t* output, size_t capacity, size_t* count);

wz_result_t wz_tape_manager_edit_init(wz_tape_manager_edit_t* edit,
                                      wz_tap_block_t* blocks,
                                      size_t count, size_t capacity);
wz_result_t wz_tape_manager_reorder(wz_tape_manager_edit_t* edit,
                                    size_t from, size_t to);
wz_result_t wz_tape_manager_change_position(wz_tape_manager_edit_t* edit,
                                            size_t from, size_t to);
wz_result_t wz_tape_manager_add_block(wz_tape_manager_edit_t* edit,
                                      wz_tap_block_t block, size_t position);
wz_result_t wz_tape_manager_delete_block(wz_tape_manager_edit_t* edit,
                                         size_t position);
wz_result_t wz_tape_manager_edit_block(wz_tape_manager_edit_t* edit,
                                       size_t position, wz_tap_block_t block);
wz_result_t wz_tape_manager_extract_block(const wz_tape_manager_edit_t* edit,
                                          size_t position, wz_tap_block_t* output);
wz_result_t wz_tape_manager_copy_block_to_new(const wz_tape_manager_edit_t* edit,
                                              size_t position, wz_tap_block_t* output);

#endif
