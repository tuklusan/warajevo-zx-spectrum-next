/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "diagnostics/wz_trace_file.h"
#include <string.h>

#define WZ_TRACE_FORMAT_VERSION 2u
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

static bool write_header(wz_trace_file_t* t)
{
    wz_byte_t h[WZ_TRACE_HEADER_SIZE]; memset(h,0,sizeof(h)); memcpy(h,"WZSNTRC",7u);
    put32(h+8u,WZ_TRACE_FORMAT_VERSION); put32(h+12u,WZ_TRACE_HEADER_SIZE);
    put32(h+16u,WZ_TRACE_RECORD_SIZE); put32(h+20u,WZ_TRACE_FILE_SIZE);
    put64(h+24u,t->session_id); put32(h+32u,t->profile_kind); put32(h+36u,t->event_mask);
    put64(h+40u,t->next_slot); put64(h+48u,t->generation);
    put64(h+56u,t->first_sequence); put64(h+64u,t->last_sequence); h[72]=t->frozen?1u:0u;
    put64(h+80u,t->rom_identity); put32(h+88u,WZ_TRACE_RECORD_SIZE); put32(h+92u,WZ_TRACE_RECORD_SIZE);
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
    if(!t||!t->file||!e||t->frozen||t->failed)return;
    if ((unsigned)e->kind >= 32u || (t->event_mask & (UINT32_C(1) << (unsigned)e->kind)) == 0u) return;
    memset(r,0,sizeof(r)); r[0]=(wz_byte_t)WZ_TRACE_RECORD_SIZE;
    r[1]=(wz_byte_t)e->kind; r[2]=e->cycle; r[3]=e->t_states;
    put32(r+4u,(wz_dword_t)e->sequence); put64(r+8u,e->master_tick);
    put16(r+16u,e->address); put16(r+18u,e->program_counter);
    r[20]=e->value; r[21]=e->auxiliary;
    put16(r+22u,(wz_word_t)e->register_snapshot); put32(r+24u,WZ_TRACE_COMMIT);
    if(fseek(t->file,(long)(WZ_TRACE_HEADER_SIZE+t->next_slot*WZ_TRACE_RECORD_SIZE),SEEK_SET)!=0||
       fwrite(r,1u,sizeof(r),t->file)!=sizeof(r)){t->failed=true;return;}
    if(t->first_sequence==UINT64_MAX)t->first_sequence=e->sequence;
    t->last_sequence=e->sequence;t->next_slot++;
    if(t->next_slot==slots){t->next_slot=0u;t->generation++;}
    if(t->last_sequence>=slots)t->first_sequence=t->last_sequence-slots+1u;
    if(!write_header(t))t->failed=true;
}

wz_result_t wz_trace_file_freeze(wz_trace_file_t* t){if(!t||!t->file)return WZ_RESULT_INVALID_ARGUMENT;t->frozen=true;return write_header(t)?WZ_RESULT_OK:WZ_RESULT_TRACE_FAILURE;}
void wz_trace_file_close(wz_trace_file_t* t){if(t&&t->file){fclose(t->file);t->file=0;}}

wz_result_t wz_trace_file_recover(const char* path,wz_trace_recover_fn fn,void* context,size_t* count)
{
    FILE* f;wz_byte_t h[WZ_TRACE_HEADER_SIZE],r[WZ_TRACE_RECORD_SIZE];wz_qword_t first,last;size_t n=0u;
    if (!path || !fn || !count) return WZ_RESULT_INVALID_ARGUMENT;
    *count = 0u;
    f = fopen(path, "rb");
    if (!f) return WZ_RESULT_TRACE_FAILURE;
    if(fread(h,1u,sizeof(h),f)!=sizeof(h)||memcmp(h,"WZSNTRC",7u)!=0||get32(h+8u)!=WZ_TRACE_FORMAT_VERSION){fclose(f);return WZ_RESULT_INVALID_STATE;}
    first=get64(h+56u);last=get64(h+64u);
    if(first!=UINT64_MAX)for(wz_qword_t seq=first;seq<=last;++seq){
        wz_qword_t slot=seq%slot_count();wz_trace_event_t e;
        if(fseek(f,(long)(WZ_TRACE_HEADER_SIZE+slot*WZ_TRACE_RECORD_SIZE),SEEK_SET)!=0||fread(r,1u,sizeof(r),f)!=sizeof(r))break;
        if(r[0]!=WZ_TRACE_RECORD_SIZE||get32(r+24u)!=WZ_TRACE_COMMIT||get32(r+4u)!=(wz_dword_t)seq)continue;
        memset(&e, 0, sizeof(e)); e.kind=(wz_trace_event_kind_t)r[1];
        e.cycle=r[2]; e.t_states=r[3]; e.sequence=seq; e.master_tick=get64(r+8u);
        e.address=get16(r+16u); e.program_counter=get16(r+18u);
        e.value=r[20]; e.auxiliary=r[21]; e.register_snapshot=get16(r+22u);
        n++;if(!fn(&e,context))break;
        if(seq==UINT64_MAX)break;
    }
    fclose(f);*count=n;return WZ_RESULT_OK;
}
