/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_machine_profile.h"

static const wz_machine_profile_t wz_profile_48k = {
    WZ_MACHINE_48K_PAL,
    "ZX Spectrum 48K PAL",
    7000000u,
    1u,
    2u,
    224u,
    312u,
    69888u,
    448u,
    256u,
    192u,
    0u
};

static const wz_machine_profile_t wz_profile_128k = {
    WZ_MACHINE_128K_PAL,
    "ZX Spectrum 128K PAL",
    7000000u,
    1u,
    2u,
    228u,
    311u,
    70908u,
    456u,
    256u,
    192u,
    0u
};

const wz_machine_profile_t* wz_machine_profile_48k_pal(void)
{
    return &wz_profile_48k;
}

const wz_machine_profile_t* wz_machine_profile_128k_pal(void)
{
    return &wz_profile_128k;
}

wz_dword_t wz_profile_cpu_tstate(wz_master_tick_t tick,
                                 const wz_machine_profile_t* profile)
{
    if (profile == 0 || profile->master_ticks_per_cpu_tstate == 0u) {
        return 0u;
    }

    return (wz_dword_t)(tick / profile->master_ticks_per_cpu_tstate);
}

wz_dword_t wz_profile_cpu_phase(wz_master_tick_t tick,
                                const wz_machine_profile_t* profile)
{
    if (profile == 0 || profile->master_ticks_per_cpu_tstate == 0u) {
        return 0u;
    }

    return (wz_dword_t)(tick % profile->master_ticks_per_cpu_tstate);
}
