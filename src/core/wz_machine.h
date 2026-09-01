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
#include "core/wz_trace.h"
#include "core/wz_types.h"
#include "core/wz_z80.h"

#define WZ_48K_ROM_SIZE 16384u
#define WZ_48K_RAM_SIZE 49152u

typedef struct wz_machine {
    const wz_machine_profile_t* profile;
    wz_z80_state_t cpu;
    wz_bus_observer_t bus_observer;
    wz_bus_input_t bus_input;
    wz_trace_sink_t* timing_trace;
    wz_byte_t memory[65536u];
    wz_byte_t has_48k_rom;
    wz_master_tick_t master_tick;
    wz_byte_t im0_injected_opcode;
    wz_byte_t im0_injected_opcode_pending;
} wz_machine_t;

wz_result_t wz_machine_init(wz_machine_t* machine,
                            const wz_machine_profile_t* profile);
void wz_machine_destroy(wz_machine_t* machine);
const char* wz_machine_boot_message(void);
void wz_machine_set_timing_trace(wz_machine_t* machine, wz_trace_sink_t* trace);
wz_result_t wz_machine_load_48k_rom(wz_machine_t* machine,
                                    const wz_byte_t* bytes,
                                    size_t length);
wz_byte_t wz_machine_memory_read(const wz_machine_t* machine, wz_word_t address);
void wz_machine_memory_write(wz_machine_t* machine, wz_word_t address,
                             wz_byte_t value);

#endif
