/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_TYPES_H
#define WZ_CORE_WZ_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t wz_byte_t;
typedef uint16_t wz_word_t;
typedef uint32_t wz_dword_t;
typedef uint64_t wz_qword_t;
typedef uint64_t wz_master_tick_t;

typedef enum {
    WZ_RESULT_OK = 0,
    WZ_RESULT_INVALID_ARGUMENT,
    WZ_RESULT_INVALID_PROFILE,
    WZ_RESULT_ROM_IDENTITY_MISMATCH,
    WZ_RESULT_SERIALIZATION_FAILURE,
    WZ_RESULT_INVALID_STATE,
    WZ_RESULT_UNSUPPORTED_OPERATION,
    WZ_RESULT_TRACE_FAILURE
} wz_result_t;

_Static_assert(sizeof(wz_byte_t) == 1u, "wz_byte_t must be 8 bits");
_Static_assert(sizeof(wz_word_t) == 2u, "wz_word_t must be 16 bits");
_Static_assert(sizeof(wz_dword_t) == 4u, "wz_dword_t must be 32 bits");
_Static_assert(sizeof(wz_qword_t) == 8u, "wz_qword_t must be 64 bits");

static inline wz_word_t wz_read_le16(const wz_byte_t bytes[2])
{
    return (wz_word_t)bytes[0] | ((wz_word_t)bytes[1] << 8u);
}

static inline void wz_write_le16(wz_byte_t bytes[2], wz_word_t value)
{
    bytes[0] = (wz_byte_t)(value & 0xffu);
    bytes[1] = (wz_byte_t)(value >> 8u);
}

static inline bool wz_bit_is_set(wz_byte_t value, unsigned bit)
{
    return bit < 8u && ((value & (wz_byte_t)(1u << bit)) != 0u);
}

#endif
