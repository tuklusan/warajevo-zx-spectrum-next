/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_raster_evidence.h"

#define WZ_EVIDENCE_FNV_OFFSET  UINT64_C(14695981039346656037)
#define WZ_EVIDENCE_FNV_PRIME   UINT64_C(1099511628211)

static void wz_hash_byte(wz_qword_t* hash, wz_byte_t value)
{
    *hash ^= value;
    *hash *= WZ_EVIDENCE_FNV_PRIME;
}

static void wz_hash_le64(wz_qword_t* hash, wz_qword_t value)
{
    unsigned index;
    for (index = 0u; index < 8u; ++index) {
        wz_hash_byte(hash, (wz_byte_t)(value >> (index * 8u)));
    }
}

wz_result_t wz_raster_buffer_hash(const wz_raster_buffer_t* buffer,
                                  wz_qword_t* hash)
{
    size_t length;
    size_t index;

    if (buffer == NULL || hash == NULL ||
        (buffer->samples == NULL && buffer->width * buffer->height != 0u)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    length = buffer->width * buffer->height;
    *hash = WZ_EVIDENCE_FNV_OFFSET;
    wz_hash_le64(hash, (wz_qword_t)buffer->width);
    wz_hash_le64(hash, (wz_qword_t)buffer->height);
    for (index = 0u; index < length; ++index) {
        wz_hash_byte(hash, buffer->samples[index]);
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_trace_events_hash(const wz_trace_event_t* events,
                                 size_t count, wz_qword_t* hash)
{
    size_t index;
    const wz_trace_event_t* event;

    if (hash == NULL || (events == NULL && count != 0u)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    *hash = WZ_EVIDENCE_FNV_OFFSET;
    wz_hash_le64(hash, (wz_qword_t)count);
    for (index = 0u; index < count; ++index) {
        event = &events[index];
        wz_hash_le64(hash, (wz_qword_t)event->kind);
        wz_hash_le64(hash, event->master_tick);
        wz_hash_le64(hash, event->sequence);
        wz_hash_le64(hash, (wz_qword_t)event->address);
        wz_hash_le64(hash, (wz_qword_t)event->program_counter);
        wz_hash_le64(hash, (wz_qword_t)event->stack_pointer);
        wz_hash_le64(hash, event->register_snapshot);
        wz_hash_byte(hash, event->value);
        wz_hash_byte(hash, event->auxiliary);
        wz_hash_byte(hash, event->cycle);
        wz_hash_byte(hash, event->t_states);
    }
    return WZ_RESULT_OK;
}
