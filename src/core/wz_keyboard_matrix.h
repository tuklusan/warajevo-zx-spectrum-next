/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_KEYBOARD_MATRIX_H
#define WZ_CORE_WZ_KEYBOARD_MATRIX_H

#include <stdbool.h>
#include <stddef.h>

#include "core/wz_types.h"

#define WZ_KEYBOARD_MATRIX_ROW_COUNT 8u
#define WZ_KEYBOARD_MATRIX_KEYS_PER_ROW 5u
#define WZ_KEYBOARD_MATRIX_KEY_COUNT 40u

typedef enum {
    WZ_KEY_SHIFT = 0u, WZ_KEY_Z, WZ_KEY_X, WZ_KEY_C, WZ_KEY_V,
    WZ_KEY_A, WZ_KEY_S, WZ_KEY_D, WZ_KEY_F, WZ_KEY_G,
    WZ_KEY_Q, WZ_KEY_W, WZ_KEY_E, WZ_KEY_R, WZ_KEY_T,
    WZ_KEY_1, WZ_KEY_2, WZ_KEY_3, WZ_KEY_4, WZ_KEY_5,
    WZ_KEY_0, WZ_KEY_9, WZ_KEY_8, WZ_KEY_7, WZ_KEY_6,
    WZ_KEY_P, WZ_KEY_O, WZ_KEY_I, WZ_KEY_U, WZ_KEY_Y,
    WZ_KEY_ENTER, WZ_KEY_L, WZ_KEY_K, WZ_KEY_J, WZ_KEY_H,
    WZ_KEY_SPACE, WZ_KEY_SYMBOL_SHIFT, WZ_KEY_M, WZ_KEY_N, WZ_KEY_B
} wz_keyboard_key_t;

typedef struct {
    unsigned char rows[WZ_KEYBOARD_MATRIX_ROW_COUNT];
} wz_keyboard_matrix_t;

void wz_keyboard_matrix_init(wz_keyboard_matrix_t* matrix);
bool wz_keyboard_matrix_set(wz_keyboard_matrix_t* matrix,
                            wz_keyboard_key_t key, bool pressed);
bool wz_keyboard_matrix_key_position(wz_keyboard_key_t key,
                                     size_t* row, size_t* column);
wz_byte_t wz_keyboard_matrix_scan(const wz_keyboard_matrix_t* matrix,
                                  unsigned char row_select);

#endif
