/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_MACHINE_PROFILE_H
#define WZ_CORE_WZ_MACHINE_PROFILE_H

#include "core/wz_types.h"

typedef enum {
    WZ_MACHINE_48K_PAL = 0,
    WZ_MACHINE_128K_PAL
} wz_machine_kind_t;

typedef struct {
    wz_machine_kind_t kind;
    const char* name;
    wz_qword_t master_hz_num;
    wz_qword_t master_hz_den;
    wz_dword_t master_ticks_per_cpu_tstate;
    uint16_t tstates_per_line;
    uint16_t lines_per_frame;
    wz_dword_t tstates_per_frame;
    uint16_t raster_clocks_per_line;
    uint16_t active_width;
    uint16_t active_height;
    wz_dword_t interrupt_assert_tstate;
    wz_dword_t interrupt_deassert_tstate;
    wz_qword_t expected_rom_identity;
} wz_machine_profile_t;

const wz_machine_profile_t* wz_machine_profile_48k_pal(void);
const wz_machine_profile_t* wz_machine_profile_128k_pal(void);

wz_dword_t wz_profile_cpu_tstate(wz_master_tick_t tick,
                                 const wz_machine_profile_t* profile);
wz_dword_t wz_profile_cpu_phase(wz_master_tick_t tick,
                                const wz_machine_profile_t* profile);

#endif
