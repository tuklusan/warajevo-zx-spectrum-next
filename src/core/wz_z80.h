/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_Z80_H
#define WZ_CORE_WZ_Z80_H

#include "core/wz_types.h"

typedef enum {
    WZ_Z80_INTERRUPT_MODE_0 = 0,
    WZ_Z80_INTERRUPT_MODE_1 = 1,
    WZ_Z80_INTERRUPT_MODE_2 = 2
} wz_z80_interrupt_mode_t;

typedef struct {
    wz_byte_t a;
    wz_byte_t f;
    wz_byte_t b;
    wz_byte_t c;
    wz_byte_t d;
    wz_byte_t e;
    wz_byte_t h;
    wz_byte_t l;
} wz_z80_register_bank_t;

typedef struct {
    wz_z80_register_bank_t main;
    wz_z80_register_bank_t alternate;
    wz_word_t ix;
    wz_word_t iy;
    wz_word_t stack_pointer;
    wz_word_t program_counter;
    wz_byte_t i;
    wz_byte_t r;
    wz_byte_t iff1;
    wz_byte_t iff2;
    wz_byte_t interrupt_mode;
    wz_byte_t halted;
} wz_z80_state_t;

void wz_z80_state_init(wz_z80_state_t* state);
wz_result_t wz_z80_state_validate(const wz_z80_state_t* state);

#endif
