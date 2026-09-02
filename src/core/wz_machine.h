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
#include "core/audio/wz_beeper.h"
#include "core/wz_machine_profile.h"
#include "core/wz_kempston.h"
#include "core/wz_raster.h"
#include "core/wz_trace.h"
#include "core/wz_types.h"
#include "core/wz_z80.h"

#define WZ_48K_ROM_SIZE 16384u
#define WZ_48K_RAM_SIZE 49152u
#define WZ_BORDER_EVENT_CAPACITY 1024u

typedef struct {
    wz_qword_t frame_number;
    wz_dword_t frame_raster_clock;
    wz_dword_t line;
    wz_dword_t raster_clock;
} wz_raster_position_t;

typedef enum {
    WZ_ULA_FETCH_BITMAP = 0,
    WZ_ULA_FETCH_ATTRIBUTE
} wz_ula_fetch_kind_t;

typedef struct {
    wz_ula_fetch_kind_t kind;
    wz_master_tick_t master_tick;
    wz_word_t address;
    wz_byte_t value;
} wz_ula_fetch_event_t;

typedef struct {
    wz_master_tick_t master_tick;
    wz_byte_t color;
} wz_border_event_t;

typedef struct wz_machine {
    const wz_machine_profile_t* profile;
    wz_z80_state_t cpu;
    wz_bus_observer_t bus_observer;
    wz_bus_input_t bus_input;
    wz_bus_data_source_t bus_data_source;
    wz_trace_sink_t* timing_trace;
    wz_byte_t memory[65536u];
    wz_byte_t has_48k_rom;
    wz_byte_t hardware_io_decode_enabled;
    wz_byte_t keyboard_rows[8u];
    wz_kempston_t kempston;
    wz_beeper_t beeper;
    wz_byte_t ula_output;
    wz_qword_t rom_identity;
    wz_master_tick_t master_tick;
    wz_master_tick_t ula_output_tick;
    wz_byte_t border_color;
    wz_border_event_t border_events[WZ_BORDER_EVENT_CAPACITY];
    size_t border_event_count;
    wz_byte_t maskable_interrupt_line_low;
    wz_byte_t im0_injected_opcode;
    wz_byte_t im0_injected_opcode_pending;
} wz_machine_t;

wz_result_t wz_machine_init(wz_machine_t* machine,
                            const wz_machine_profile_t* profile);
void wz_machine_destroy(wz_machine_t* machine);
const char* wz_machine_boot_message(void);
void wz_machine_set_timing_trace(wz_machine_t* machine, wz_trace_sink_t* trace);
wz_result_t wz_machine_set_hardware_io_decode(wz_machine_t* machine, bool enabled);
wz_result_t wz_machine_set_keyboard_key(wz_machine_t* machine,
                                        wz_byte_t row,
                                        wz_byte_t key,
                                        bool pressed);
wz_result_t wz_machine_set_kempston_control(wz_machine_t* machine,
                                             wz_kempston_control_t control,
                                             bool pressed);
wz_byte_t wz_machine_kempston_read(const wz_machine_t* machine,
                                   wz_word_t address);
wz_byte_t wz_machine_contention_delay(const wz_machine_t* machine,
                                      wz_bus_cycle_t cycle,
                                      wz_word_t address,
                                      wz_master_tick_t master_tick,
                                      wz_byte_t t_states);
wz_result_t wz_machine_load_48k_rom(wz_machine_t* machine,
                                    const wz_byte_t* bytes,
                                    size_t length);
wz_qword_t wz_machine_rom_identity(const wz_byte_t* bytes, size_t length);
wz_byte_t wz_machine_memory_read(const wz_machine_t* machine, wz_word_t address);
void wz_machine_memory_write(wz_machine_t* machine, wz_word_t address,
                             wz_byte_t value);
void wz_machine_memory_write_at_tick(wz_machine_t* machine, wz_word_t address,
                                     wz_byte_t value, wz_master_tick_t master_tick);
bool wz_machine_ula_port_fe_selected(wz_word_t address);
wz_byte_t wz_machine_beeper_level(const wz_machine_t* machine);
wz_byte_t wz_machine_mic_level(const wz_machine_t* machine);
size_t wz_machine_beeper_events(const wz_machine_t* machine,
                                wz_beeper_event_t* events,
                                size_t capacity);
wz_byte_t wz_machine_ula_port_fe_read(const wz_machine_t* machine,
                                       wz_word_t address);
void wz_machine_ula_port_fe_write(wz_machine_t* machine, wz_word_t address,
                                  wz_byte_t value, wz_master_tick_t master_tick);
wz_byte_t wz_machine_border_color(const wz_machine_t* machine);
size_t wz_machine_border_events(const wz_machine_t* machine,
                                wz_border_event_t* events, size_t capacity);
void wz_machine_update_interrupt_line(wz_machine_t* machine);
bool wz_machine_maskable_interrupt_line_low(const wz_machine_t* machine);
wz_result_t wz_machine_raster_position(const wz_machine_t* machine,
                                       wz_raster_position_t* position);
wz_result_t wz_machine_ula_fetches_at_tick(const wz_machine_t* machine,
                                           wz_master_tick_t master_tick,
                                           wz_ula_fetch_event_t* events,
                                           size_t capacity,
                                           size_t* count);
wz_byte_t wz_machine_floating_bus_value(const wz_machine_t* machine,
                                        wz_master_tick_t master_tick);
bool wz_machine_flash_phase(const wz_machine_t* machine,
                            wz_master_tick_t master_tick);

#endif
