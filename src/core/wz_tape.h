/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_TAPE_H
#define WZ_CORE_WZ_TAPE_H

#include <stdbool.h>
#include <stddef.h>

#include "core/wz_types.h"

typedef struct {
    wz_master_tick_t duration;
    wz_byte_t ear_level;
} wz_tape_segment_t;

typedef struct {
    const wz_tape_segment_t* segments;
    size_t segment_count;
} wz_tape_t;

typedef struct {
    const wz_tape_t* tape;
    size_t segment_index;
    wz_master_tick_t segment_elapsed;
    wz_byte_t ear_level;
    bool motor_on;
    bool at_end;
} wz_tape_state_t;

typedef struct {
    const wz_byte_t* data;
    size_t length;
} wz_tap_block_t;

wz_result_t wz_tape_validate(const wz_tape_segment_t* segments,
                             size_t segment_count);
wz_result_t wz_tape_mount(wz_tape_t* tape,
                          const wz_tape_segment_t* segments,
                          size_t segment_count);
wz_result_t wz_tape_state_init(wz_tape_state_t* state,
                               const wz_tape_t* tape);
wz_result_t wz_tape_state_advance(wz_tape_state_t* state,
                                  wz_master_tick_t ticks);
wz_result_t wz_tape_state_rewind(wz_tape_state_t* state);
wz_result_t wz_tape_state_set_motor(wz_tape_state_t* state, bool motor_on);
wz_byte_t wz_tape_state_ear_level(const wz_tape_state_t* state);
bool wz_tape_state_at_end(const wz_tape_state_t* state);

/* Parse standard TAP into caller-owned canonical master-tick segments. */
wz_result_t wz_tape_parse_standard_tap(const wz_byte_t* data,
                                       size_t length,
                                       wz_dword_t master_ticks_per_tstate,
                                       wz_tape_segment_t* segments,
                                       size_t capacity,
                                       size_t* count);
wz_result_t wz_tape_write_standard_tap(const wz_tap_block_t* blocks,
                                       size_t block_count,
                                       wz_byte_t* output,
                                       size_t capacity,
                                       size_t* length);

#endif
