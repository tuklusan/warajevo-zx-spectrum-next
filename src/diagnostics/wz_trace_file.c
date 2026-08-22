/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "diagnostics/wz_trace_file.h"
#include <string.h>

#define WZ_TRACE_FORMAT_VERSION 4u
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
    return (WZ_TRACE_FILE_SIZE - WZ_TRACE_HEADER_SIZE) / WZ_TRACE_RECORD_SIZE;
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
    if (sync == NULL || event == NULL || event->kind != WZ_TRACE_CPU_STATE_SYNC ||
        event->cycle >= 5u) {
        return false;
    }
    if (event->cycle == 0u) {
        memset(&sync->state, 0, sizeof(sync->state));
        sync->master_tick = event->master_tick;
        sync->last_sequence = event->sequence;
        sync->next_chunk = 1u;
    } else if (event->cycle != sync->next_chunk ||
               event->master_tick != sync->master_tick ||
               event->sequence != sync->last_sequence + 1u) {
        sync->next_chunk = 0u;
        return false;
    } else {
        sync->last_sequence = event->sequence;
        sync->next_chunk += 1u;
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
        sync->next_chunk = 0u;
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
    put64(h+96u,t->last_master_tick);
    return fseek(t->file,0,SEEK_SET)==0 && fwrite(h,1u,sizeof(h),t->file)==sizeof(h) && fflush(t->file)==0;
}

wz_result_t wz_trace_file_create(wz_trace_file_t* t,const char* path,wz_qword_t sid,wz_dword_t profile,wz_qword_t rom,wz_dword_t mask)
{
    if(!t||!path||sid==0u)return WZ_RESULT_INVALID_ARGUMENT;
    memset(t,0,sizeof(*t)); t->file=fopen(path,"wbx"); if(!t->file)return WZ_RESULT_TRACE_FAILURE;
    t->session_id=sid;t->profile_kind=profile;t->rom_identity=rom;t->event_mask=mask;t->first_sequence=UINT64_MAX;
    if(fseek(t->file,(long)(WZ_TRACE_FILE_SIZE-1u),SEEK_SET)!=0||fputc(0,t->file)==EOF||!write_header(t)){
        fclose(t->file);t->file=0;return WZ_RESULT_TRACE_FAILURE;
    }
    return WZ_RESULT_OK;
}

void wz_trace_file_emit(const wz_trace_event_t* e,void* context)
{
    wz_trace_file_t* t=(wz_trace_file_t*)context; wz_byte_t r[WZ_TRACE_RECORD_SIZE]; wz_qword_t slots=slot_count();
    wz_dword_t tick_delta;
    if(!t||!t->file||!e||t->frozen||t->failed)return;
    if ((unsigned)e->kind >= 32u || (t->event_mask & (UINT32_C(1) << (unsigned)e->kind)) == 0u) return;
    if (e->master_tick < t->last_master_tick ||
        e->master_tick - t->last_master_tick > UINT32_MAX) { t->failed=true; return; }
    tick_delta = (wz_dword_t)(e->master_tick - t->last_master_tick);
    memset(r,0,sizeof(r)); r[0]=(wz_byte_t)WZ_TRACE_RECORD_SIZE;
    r[1]=(wz_byte_t)e->kind; r[2]=e->cycle; r[3]=e->t_states;
    put32(r+4u,(wz_dword_t)e->sequence); put32(r+8u,tick_delta);
    if (e->kind == WZ_TRACE_CPU_STATE_SYNC) {
        put64(r+12u, e->register_snapshot);
    } else {
        put16(r+12u,e->address); put16(r+14u,e->program_counter);
        r[16]=e->value; r[17]=e->auxiliary;
        put16(r+18u,(wz_word_t)e->register_snapshot);
    }
    put32(r+WZ_TRACE_COMMIT_OFFSET,WZ_TRACE_COMMIT);
    if(fseek(t->file,(long)(WZ_TRACE_HEADER_SIZE+t->next_slot*WZ_TRACE_RECORD_SIZE),SEEK_SET)!=0||
       fwrite(r,1u,sizeof(r),t->file)!=sizeof(r)){t->failed=true;return;}
    if(t->first_sequence==UINT64_MAX)t->first_sequence=e->sequence;
    t->last_sequence=e->sequence;t->last_master_tick=e->master_tick;t->next_slot++;
    if(t->next_slot==slots){t->next_slot=0u;t->generation++;}
    if(t->last_sequence>=slots)t->first_sequence=t->last_sequence-slots+1u;
    if(!write_header(t))t->failed=true;
}

wz_result_t wz_trace_file_freeze(wz_trace_file_t* t){if(!t||!t->file)return WZ_RESULT_INVALID_ARGUMENT;t->frozen=true;return write_header(t)?WZ_RESULT_OK:WZ_RESULT_TRACE_FAILURE;}
void wz_trace_file_close(wz_trace_file_t* t){if(t&&t->file){fclose(t->file);t->file=0;}}

wz_result_t wz_trace_file_recover(const char* path,wz_trace_recover_fn fn,void* context,size_t* count)
{
    FILE* f;wz_byte_t h[WZ_TRACE_HEADER_SIZE],r[WZ_TRACE_RECORD_SIZE];wz_qword_t first,last,tick;size_t n=0u;
    if (!path || !fn || !count) return WZ_RESULT_INVALID_ARGUMENT;
    *count = 0u;
    f = fopen(path, "rb");
    if (!f) return WZ_RESULT_TRACE_FAILURE;
    if(fread(h,1u,sizeof(h),f)!=sizeof(h)||memcmp(h,"WZSNTRC",7u)!=0||get32(h+8u)!=WZ_TRACE_FORMAT_VERSION){fclose(f);return WZ_RESULT_INVALID_STATE;}
    first=get64(h+56u);last=get64(h+64u);tick=get64(h+96u);
    if(first!=UINT64_MAX)for(wz_qword_t seq=last;seq>first;--seq){
        wz_qword_t slot=seq%slot_count();
        if(fseek(f,(long)(WZ_TRACE_HEADER_SIZE+slot*WZ_TRACE_RECORD_SIZE),SEEK_SET)!=0||
           fread(r,1u,sizeof(r),f)!=sizeof(r)||r[0]!=WZ_TRACE_RECORD_SIZE||
           get32(r+WZ_TRACE_COMMIT_OFFSET)!=WZ_TRACE_COMMIT||get32(r+4u)!=(wz_dword_t)seq)break;
        tick-=get32(r+8u);
    }
    if(first!=UINT64_MAX)for(wz_qword_t seq=first;seq<=last;++seq){
        wz_qword_t slot=seq%slot_count();wz_trace_event_t e;
        if(fseek(f,(long)(WZ_TRACE_HEADER_SIZE+slot*WZ_TRACE_RECORD_SIZE),SEEK_SET)!=0||fread(r,1u,sizeof(r),f)!=sizeof(r))break;
        if(r[0]!=WZ_TRACE_RECORD_SIZE||get32(r+WZ_TRACE_COMMIT_OFFSET)!=WZ_TRACE_COMMIT||get32(r+4u)!=(wz_dword_t)seq)continue;
        memset(&e, 0, sizeof(e)); e.kind=(wz_trace_event_kind_t)r[1];
        e.cycle=r[2]; e.t_states=r[3]; e.sequence=seq; e.master_tick=tick;
        if (e.kind == WZ_TRACE_CPU_STATE_SYNC) {
            e.register_snapshot=get64(r+12u);
        } else {
            e.address=get16(r+12u); e.program_counter=get16(r+14u);
            e.value=r[16]; e.auxiliary=r[17]; e.register_snapshot=get16(r+18u);
        }
        n++;if(!fn(&e,context))break;
        tick+=get32(r+8u);
        if(seq==UINT64_MAX)break;
    }
    fclose(f);*count=n;return WZ_RESULT_OK;
}
