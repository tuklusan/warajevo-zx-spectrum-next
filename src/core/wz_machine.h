/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_MACHINE_H
#define WZ_CORE_WZ_MACHINE_H

#include <stddef.h>

#include "core/wz_bus.h"
#include "core/wz_machine_profile.h"
#include "core/wz_types.h"
#include "core/wz_z80.h"

typedef struct wz_machine {
    const wz_machine_profile_t* profile;
    wz_z80_state_t cpu;
    wz_bus_observer_t bus_observer;
    wz_byte_t memory[65536u];
    wz_master_tick_t master_tick;
} wz_machine_t;

wz_result_t wz_machine_init(wz_machine_t* machine,
                            const wz_machine_profile_t* profile);
void wz_machine_destroy(wz_machine_t* machine);
const char* wz_machine_boot_message(void);

#endif
