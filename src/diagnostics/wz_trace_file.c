/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
Proprietary rights reserved except as expressly licensed herein.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
patent, trademark, and governing-law provisions.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include "diagnostics/wz_trace_file.h"
#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <string.h>

#define WZ_TRACE_FORMAT_VERSION 5u
#define WZ_TRACE_COMMIT UINT32_C(0x57415a43)

static void put32(wz_byte_t* p, wz_dword_t v)
{
    for (size_t i = 0u; i < 4u; ++i) p[i] = (wz_byte_t)(v >> (8u * i));
}
static void put16(wz_byte_t* p, wz_word_t v)
{
    p[0] = (wz_byte_t)v;
    p[1] = (wz_byte_t)(v >> 8u);
}
static void put64(wz_byte_t* p, wz_qword_t v)
{
    for (size_t i = 0u; i < 8u; ++i) p[i] = (wz_byte_t)(v >> (8u * i));
}
static wz_dword_t get32(const wz_byte_t* p)
{
    wz_dword_t v = 0u;
    for (size_t i = 0u; i < 4u; ++i) v |= (wz_dword_t)p[i] << (8u * i);
    return v;
}
static wz_word_t get16(const wz_byte_t* p)
{
    return (wz_word_t)p[0] | ((wz_word_t)p[1] << 8u);
}
static wz_qword_t get64(const wz_byte_t* p)
{
    wz_qword_t v = 0u;
    for (size_t i = 0u; i < 8u; ++i) v |= (wz_qword_t)p[i] << (8u * i);
    return v;
}
static wz_qword_t slot_count(void)
{
#if defined(WZ_TRACE_FILE_TEST_SLOT_COUNT)
    return (wz_qword_t)WZ_TRACE_FILE_TEST_SLOT_COUNT;
#else
    return (WZ_TRACE_FILE_SIZE - WZ_TRACE_HEADER_SIZE) / WZ_TRACE_RECORD_SIZE;
#endif
}

static bool read_slot(FILE* file, wz_qword_t slot,
                      wz_byte_t record[WZ_TRACE_RECORD_SIZE])
{
    long offset = (long)(WZ_TRACE_HEADER_SIZE + slot * WZ_TRACE_RECORD_SIZE);
    return fseek(file, offset, SEEK_SET) == 0 &&
        fread(record, 1u, WZ_TRACE_RECORD_SIZE, file) == WZ_TRACE_RECORD_SIZE;
}

static bool sequence_before_with_low(wz_qword_t upper_bound,
                                     wz_dword_t low_sequence,
                                     wz_qword_t* sequence)
{
    const wz_qword_t wrap = UINT64_C(1) << 32u;
    const wz_qword_t mask = wrap - 1u;
    wz_qword_t candidate;

    if (sequence == NULL) return false;
    candidate = (upper_bound & ~mask) | (wz_qword_t)low_sequence;
    if (candidate > upper_bound) {
        if (candidate < wrap) return false;
        candidate -= wrap;
    }
    if (candidate > upper_bound) return false;
    *sequence = candidate;
    return true;
}

static void unpack_bank(wz_z80_register_bank_t* bank, wz_qword_t packed)
{
    bank->a = (wz_byte_t)packed;
    bank->f = (wz_byte_t)(packed >> 8u);
    bank->b = (wz_byte_t)(packed >> 16u);
    bank->c = (wz_byte_t)(packed >> 24u);
    bank->d = (wz_byte_t)(packed >> 32u);
    bank->e = (wz_byte_t)(packed >> 40u);
    bank->h = (wz_byte_t)(packed >> 48u);
    bank->l = (wz_byte_t)(packed >> 56u);
}

void wz_trace_cpu_state_sync_init(wz_trace_cpu_state_sync_t* sync)
{
    if (sync != NULL) {
        memset(sync, 0, sizeof(*sync));
    }
}

bool wz_trace_cpu_state_sync_apply(wz_trace_cpu_state_sync_t* sync,
                                   const wz_trace_event_t* event)
{
    if (sync == NULL || event == NULL ||
        (event->kind != WZ_TRACE_CPU_STATE_SYNC && event->kind != WZ_TRACE_CPU_STATE_DELTA) ||
        event->cycle >= 5u) {
        return false;
    }
    if (event->kind == WZ_TRACE_CPU_STATE_SYNC && event->cycle == 0u) {
        memset(&sync->state, 0, sizeof(sync->state));
        sync->master_tick = event->master_tick;
        sync->last_sequence = event->sequence;
        sync->next_chunk = 1u;
        sync->has_absolute_state = false;
    } else if (event->kind == WZ_TRACE_CPU_STATE_SYNC && (event->cycle != sync->next_chunk ||
               event->master_tick != sync->master_tick ||
               event->sequence != sync->last_sequence + 1u)) {
        sync->next_chunk = 0u;
        return false;
    } else if (event->kind == WZ_TRACE_CPU_STATE_SYNC) {
        sync->last_sequence = event->sequence;
        sync->next_chunk += 1u;
    } else if (!sync->has_absolute_state || sync->next_chunk != 0u) {
        return false;
    }
    switch (event->cycle) {
    case 0u:
        unpack_bank(&sync->state.main, event->register_snapshot);
        break;
    case 1u:
        unpack_bank(&sync->state.alternate, event->register_snapshot);
        break;
    case 2u:
        sync->state.ix = (wz_word_t)event->register_snapshot;
        sync->state.iy = (wz_word_t)(event->register_snapshot >> 16u);
        sync->state.stack_pointer = (wz_word_t)(event->register_snapshot >> 32u);
        sync->state.program_counter = (wz_word_t)(event->register_snapshot >> 48u);
        break;
    case 3u:
        sync->state.memptr = (wz_word_t)event->register_snapshot;
        sync->state.i = (wz_byte_t)(event->register_snapshot >> 16u);
        sync->state.r = (wz_byte_t)(event->register_snapshot >> 24u);
        sync->state.iff1 = (wz_byte_t)(event->register_snapshot >> 32u);
        sync->state.iff2 = (wz_byte_t)(event->register_snapshot >> 40u);
        sync->state.interrupt_enable_delay = (wz_byte_t)(event->register_snapshot >> 48u);
        sync->state.interrupt_mode = (wz_byte_t)(event->register_snapshot >> 56u);
        break;
    default:
        sync->state.halted = (wz_byte_t)event->register_snapshot;
        if (event->kind == WZ_TRACE_CPU_STATE_SYNC) {
            sync->next_chunk = 0u;
            sync->has_absolute_state = wz_z80_state_validate(&sync->state) == WZ_RESULT_OK;
            return sync->has_absolute_state;
        }
        return wz_z80_state_validate(&sync->state) == WZ_RESULT_OK;
    }
    if (event->kind == WZ_TRACE_CPU_STATE_DELTA) {
        sync->master_tick = event->master_tick;
        sync->last_sequence = event->sequence;
        return wz_z80_state_validate(&sync->state) == WZ_RESULT_OK;
    }
    return false;
}

static bool write_header(wz_trace_file_t* t)
{
    wz_byte_t h[WZ_TRACE_HEADER_SIZE]; memset(h,0,sizeof(h)); memcpy(h,"WZSNTRC",7u);
    put32(h+8u,WZ_TRACE_FORMAT_VERSION); put32(h+12u,WZ_TRACE_HEADER_SIZE);
    put32(h+16u,WZ_TRACE_RECORD_SIZE); put32(h+20u,WZ_TRACE_FILE_SIZE);
    put64(h+24u,t->session_id); put32(h+32u,t->profile_kind); put32(h+36u,t->event_mask);
    put64(h+40u,t->next_slot); put64(h+48u,t->generation);
    put64(h+56u,t->first_sequence); put64(h+64u,t->last_sequence); h[72]=t->frozen?1u:0u;
    put64(h+80u,t->rom_identity); put32(h+88u,WZ_TRACE_RECORD_SIZE); put32(h+92u,WZ_TRACE_RECORD_SIZE);
    put64(h+96u,t->last_master_tick); put64(h+104u,t->record_count);
    return fseek(t->file,0,SEEK_SET)==0 && fwrite(h,1u,sizeof(h),t->file)==sizeof(h) && fflush(t->file)==0;
}

wz_result_t wz_trace_file_create(wz_trace_file_t* t,const char* path,wz_qword_t sid,wz_dword_t profile,wz_qword_t rom,wz_dword_t mask)
{
    int fd;
    if(!t||!path||sid==0u)return WZ_RESULT_INVALID_ARGUMENT;
    memset(t,0,sizeof(*t));
#if defined(_WIN32)
    fd = _open(path, _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
    fd = open(path, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
#endif
    if (fd < 0) return WZ_RESULT_TRACE_FAILURE;
    t->file = fdopen(fd, "w+b");
    if (t->file == NULL) {
#if defined(_WIN32)
        _close(fd);
#else
        close(fd);
#endif
        (void)remove(path);
        return WZ_RESULT_TRACE_FAILURE;
    }
    t->session_id=sid;t->profile_kind=profile;t->rom_identity=rom;t->event_mask=mask;t->first_sequence=UINT64_MAX;
    if(fseek(t->file,(long)(WZ_TRACE_FILE_SIZE-1u),SEEK_SET)!=0||fputc(0,t->file)==EOF||!write_header(t)){
        fclose(t->file);t->file=0;(void)remove(path);return WZ_RESULT_TRACE_FAILURE;
    }
    return WZ_RESULT_OK;
}

void wz_trace_file_set_forwarder(wz_trace_file_t* t,wz_trace_emit_fn emit,void* context)
{
    if(!t||t->frozen)return;
    t->append_emit=emit;
    t->append_context=context;
}

void wz_trace_file_emit(const wz_trace_event_t* e,void* context)
{
    wz_trace_file_t* t=(wz_trace_file_t*)context; wz_byte_t r[WZ_TRACE_RECORD_SIZE]; wz_qword_t slots=slot_count();
    wz_dword_t tick_delta;
    if(!t||!t->file||!e||t->frozen||t->failed)return;
    if ((unsigned)e->kind >= 32u || (t->event_mask & (UINT32_C(1) << (unsigned)e->kind)) == 0u) return;
    if (e->master_tick < t->last_master_tick ||
        e->master_tick - t->last_master_tick > UINT32_MAX) { t->failed=true; return; }
    if (t->record_count != 0u && e->sequence <= t->last_sequence) {
        t->failed = true;
        return;
    }
    tick_delta = (wz_dword_t)(e->master_tick - t->last_master_tick);
    memset(r,0,sizeof(r)); r[0]=(wz_byte_t)WZ_TRACE_RECORD_SIZE;
    r[1]=(wz_byte_t)e->kind; r[2]=e->cycle; r[3]=e->t_states;
    put32(r+4u,(wz_dword_t)e->sequence); put32(r+8u,tick_delta);
    if (e->kind == WZ_TRACE_CPU_STATE_SYNC || e->kind == WZ_TRACE_CPU_STATE_DELTA) {
        put64(r+12u, e->register_snapshot);
    } else {
        put16(r+12u,e->address); put16(r+14u,e->program_counter);
        r[16]=e->value; r[17]=e->auxiliary;
        put16(r+18u,(wz_word_t)e->register_snapshot);
    }
    put32(r+WZ_TRACE_COMMIT_OFFSET,WZ_TRACE_COMMIT);
    if(fseek(t->file,(long)(WZ_TRACE_HEADER_SIZE+t->next_slot*WZ_TRACE_RECORD_SIZE),SEEK_SET)!=0||
       fwrite(r,1u,sizeof(r),t->file)!=sizeof(r)){t->failed=true;return;}
    if (t->record_count == 0u) t->first_sequence = e->sequence;
    t->last_sequence=e->sequence;t->last_master_tick=e->master_tick;t->next_slot++;
    if(t->next_slot==slots){t->next_slot=0u;t->generation++;}
    if (t->record_count < slots) {
        t->record_count++;
    } else {
        wz_byte_t oldest[WZ_TRACE_RECORD_SIZE];
        if (!read_slot(t->file, t->next_slot, oldest) ||
            oldest[0] != WZ_TRACE_RECORD_SIZE ||
            get32(oldest + WZ_TRACE_COMMIT_OFFSET) != WZ_TRACE_COMMIT ||
            !sequence_before_with_low(e->sequence, get32(oldest + 4u),
                                      &t->first_sequence)) {
            t->failed = true;
            return;
        }
    }
    if(!write_header(t)){t->failed=true;return;}
    if(t->append_emit)t->append_emit(e,t->append_context);
}

wz_result_t wz_trace_file_freeze(wz_trace_file_t* t){if(!t||!t->file)return WZ_RESULT_INVALID_ARGUMENT;t->frozen=true;return write_header(t)?WZ_RESULT_OK:WZ_RESULT_TRACE_FAILURE;}
void wz_trace_file_close(wz_trace_file_t* t){if(t&&t->file){fclose(t->file);t->file=0;}}

wz_result_t wz_trace_file_recover(const char* path,wz_trace_recover_fn fn,void* context,size_t* count)
{
    FILE* f;
    wz_byte_t header[WZ_TRACE_HEADER_SIZE];
    wz_byte_t record[WZ_TRACE_RECORD_SIZE];
    wz_qword_t slots = slot_count();
    wz_qword_t first_sequence;
    wz_qword_t last_sequence;
    wz_qword_t next_slot;
    wz_qword_t record_count;
    wz_qword_t first_slot;
    wz_qword_t* ticks = NULL;
    wz_qword_t sequence;
    wz_qword_t last_tick;
    bool dropped_torn_oldest = false;
    size_t recovered = 0u;
    long file_size;
    wz_result_t result = WZ_RESULT_INVALID_STATE;

    if (path == NULL || fn == NULL || count == NULL) return WZ_RESULT_INVALID_ARGUMENT;
    *count = 0u;
    f = fopen(path, "rb");
    if (f == NULL) return WZ_RESULT_TRACE_FAILURE;
    if (fseek(f, 0L, SEEK_END) != 0 || (file_size = ftell(f)) < 0 ||
        (wz_qword_t)file_size != WZ_TRACE_FILE_SIZE || fseek(f, 0L, SEEK_SET) != 0 ||
        fread(header, 1u, sizeof(header), f) != sizeof(header) ||
        memcmp(header, "WZSNTRC", 7u) != 0 ||
        get32(header + 8u) != WZ_TRACE_FORMAT_VERSION ||
        get32(header + 12u) != WZ_TRACE_HEADER_SIZE ||
        get32(header + 16u) != WZ_TRACE_RECORD_SIZE ||
        get32(header + 20u) != WZ_TRACE_FILE_SIZE ||
        get32(header + 88u) != WZ_TRACE_RECORD_SIZE ||
        get32(header + 92u) != WZ_TRACE_RECORD_SIZE) goto cleanup;

    next_slot = get64(header + 40u);
    first_sequence = get64(header + 56u);
    last_sequence = get64(header + 64u);
    last_tick = get64(header + 96u);
    record_count = get64(header + 104u);
    if (slots == 0u || next_slot >= slots || record_count > slots) goto cleanup;
    if (record_count == 0u) {
        if (first_sequence != UINT64_MAX) goto cleanup;
        result = WZ_RESULT_OK;
        goto cleanup;
    }
    if (first_sequence == UINT64_MAX || last_sequence < first_sequence) goto cleanup;
    first_slot = (next_slot + slots - (record_count % slots)) % slots;
    if (!read_slot(f, first_slot, record) || record[0] != WZ_TRACE_RECORD_SIZE ||
        get32(record + WZ_TRACE_COMMIT_OFFSET) != WZ_TRACE_COMMIT) {
        if (record_count != slots) goto cleanup;
        dropped_torn_oldest = true;
        first_slot = (first_slot + 1u) % slots;
        record_count--;
        if (record_count == 0u || !read_slot(f, first_slot, record) ||
            record[0] != WZ_TRACE_RECORD_SIZE ||
            get32(record + WZ_TRACE_COMMIT_OFFSET) != WZ_TRACE_COMMIT ||
            !sequence_before_with_low(last_sequence, get32(record + 4u),
                                      &first_sequence)) goto cleanup;
    }

    if (record_count > SIZE_MAX / sizeof(*ticks)) goto cleanup;
    ticks = (wz_qword_t*)malloc((size_t)record_count * sizeof(*ticks));
    if (ticks == NULL) {
        result = WZ_RESULT_OUT_OF_MEMORY;
        goto cleanup;
    }
    ticks[record_count - 1u] = last_tick;
    for (wz_qword_t index = record_count; index-- > 1u;) {
        wz_qword_t slot = (first_slot + index) % slots;
        if (!read_slot(f, slot, record) || record[0] != WZ_TRACE_RECORD_SIZE ||
            record[1] >= 32u ||
            get32(record + WZ_TRACE_COMMIT_OFFSET) != WZ_TRACE_COMMIT ||
            get32(record + 8u) > ticks[index]) goto cleanup;
        ticks[index - 1u] = ticks[index] - get32(record + 8u);
    }

    sequence = first_sequence;
    for (wz_qword_t index = 0u; index < record_count; ++index) {
        wz_qword_t slot = (first_slot + index) % slots;
        wz_dword_t low_sequence;
        if (!read_slot(f, slot, record) || record[0] != WZ_TRACE_RECORD_SIZE ||
            record[1] >= 32u ||
            get32(record + WZ_TRACE_COMMIT_OFFSET) != WZ_TRACE_COMMIT) goto cleanup;
        low_sequence = get32(record + 4u);
        if (index == 0u) {
            if ((wz_dword_t)sequence != low_sequence) goto cleanup;
        } else {
            wz_dword_t sequence_delta = low_sequence - (wz_dword_t)sequence;
            if (sequence_delta == 0u || UINT64_MAX - sequence < sequence_delta) goto cleanup;
            sequence += sequence_delta;
        }
    }
    if (sequence != last_sequence) goto cleanup;

    sequence = first_sequence;
    for (wz_qword_t index = 0u; index < record_count; ++index) {
        wz_qword_t slot = (first_slot + index) % slots;
        wz_trace_event_t event;
        if (!read_slot(f, slot, record)) goto cleanup;
        if (index != 0u) sequence += (wz_dword_t)(get32(record + 4u) - (wz_dword_t)sequence);
        memset(&event, 0, sizeof(event));
        event.kind = (wz_trace_event_kind_t)record[1];
        event.cycle = record[2];
        event.t_states = record[3];
        event.sequence = sequence;
        event.master_tick = ticks[index];
        if (event.kind == WZ_TRACE_CPU_STATE_SYNC ||
            event.kind == WZ_TRACE_CPU_STATE_DELTA) {
            event.register_snapshot = get64(record + 12u);
        } else {
            event.address = get16(record + 12u);
            event.program_counter = get16(record + 14u);
            event.value = record[16];
            event.auxiliary = record[17];
            event.register_snapshot = get16(record + 18u);
        }
        recovered++;
        if (!fn(&event, context)) break;
    }
    *count = recovered;
    result = WZ_RESULT_OK;

cleanup:
    free(ticks);
    fclose(f);
    return result;
}
