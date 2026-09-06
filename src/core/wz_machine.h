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
#include "core/audio/wz_ay.h"
#include "core/wz_machine_profile.h"
#include "core/wz_kempston.h"
#include "core/wz_raster.h"
#include "core/wz_trace.h"
#include "core/wz_tape.h"
#include "core/wz_microdrive.h"
#include "core/wz_types.h"
#include "core/wz_z80.h"
#include "core/wz_zxnet.h"

#define WZ_48K_ROM_SIZE 16384u
#define WZ_INTERFACE1_ROM_SIZE 16384u
#define WZ_48K_RAM_SIZE 49152u
#define WZ_128K_RAM_BANK_COUNT 8u
#define WZ_128K_RAM_BANK_SIZE 16384u
#define WZ_BORDER_EVENT_CAPACITY 1024u
#define WZ_INTERFACE1_CONTROL_RESET 0xeeu

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

typedef enum {
    WZ_TAPE_LOADING_NORMAL = 0,
    WZ_TAPE_LOADING_INSTANT_TRAP
} wz_tape_loading_mode_t;

typedef enum {
    WZ_NETWORKING_NONE = 0,
    WZ_NETWORKING_INTERFACE1,
    WZ_NETWORKING_EAR_MIC
} wz_networking_mode_t;

typedef enum {
    WZ_INTERFACE1_ROM_OLD = 0,
    WZ_INTERFACE1_ROM_NEW
} wz_interface1_rom_variant_t;

typedef enum {
    WZ_TAPE_TRAP_REASON_NORMAL_MODE = 0,
    WZ_TAPE_TRAP_REASON_NO_TAPE,
    WZ_TAPE_TRAP_REASON_NO_ROM,
    WZ_TAPE_TRAP_REASON_NO_RECOGNIZED_LOADER
} wz_tape_trap_reason_t;

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
    /* Zero means read-only debugger access; mutation is an explicit opt-in. */
    wz_byte_t debugger_access_mode;
    wz_byte_t memory[65536u];
    wz_byte_t has_48k_rom;
    wz_byte_t interface1_rom[WZ_INTERFACE1_ROM_SIZE];
    wz_byte_t has_interface1_rom;
    wz_interface1_rom_variant_t interface1_rom_variant;
    wz_byte_t interface1_rom_page;
    wz_qword_t interface1_rom_identity;
    wz_byte_t interface1_control_latch;
    wz_byte_t interface1_previous_control_latch;
    wz_byte_t interface1_motor_shift;
    wz_byte_t interface1_active_motor;
    wz_master_tick_t interface1_control_latch_tick;
    wz_byte_t* ram_128k;
    wz_byte_t paging_7ffd;
    wz_byte_t paging_7ffd_locked;
    wz_byte_t hardware_io_decode_enabled;
    wz_byte_t keyboard_rows[8u];
    wz_kempston_t kempston;
    wz_beeper_t beeper;
    wz_ay_t ay;
    wz_tape_t tape;
    wz_tape_state_t tape_state;
    wz_byte_t tape_mounted;
    wz_tape_loading_mode_t tape_loading_mode;
    wz_networking_mode_t networking_mode;
    wz_mdr_transport_t microdrive;
    wz_zxnet_t zxnet;
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
wz_result_t wz_machine_mount_tape(wz_machine_t* machine,
                                  const wz_tape_segment_t* segments,
                                  size_t segment_count);
wz_result_t wz_machine_unmount_tape(wz_machine_t* machine);
wz_result_t wz_machine_set_tape_motor(wz_machine_t* machine, bool motor_on);
wz_result_t wz_machine_rewind_tape(wz_machine_t* machine);
wz_result_t wz_machine_advance_tape(wz_machine_t* machine,
                                    wz_master_tick_t ticks);
wz_result_t wz_machine_set_tape_loading_mode(wz_machine_t* machine,
                                              wz_tape_loading_mode_t mode);
wz_tape_loading_mode_t wz_machine_tape_loading_mode(const wz_machine_t* machine);
wz_result_t wz_machine_set_networking_mode(wz_machine_t* machine,
                                            wz_networking_mode_t mode);
wz_result_t wz_machine_reconfigure_networking_mode(wz_machine_t* machine,
                                                   wz_networking_mode_t mode);
wz_result_t wz_machine_reconfigure_networking_mode_with_mdr_resolution(
    wz_machine_t* machine, wz_networking_mode_t mode,
    wz_mdr_flush_callback_t flush_callback, void* flush_context,
    bool discard_dirty_media);
wz_networking_mode_t wz_machine_networking_mode(const wz_machine_t* machine);
wz_tape_trap_reason_t wz_machine_tape_trap_reason(const wz_machine_t* machine);
wz_byte_t wz_machine_tape_ear_level(const wz_machine_t* machine);
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
wz_result_t wz_machine_load_interface1_rom(wz_machine_t* machine,
                                           const wz_byte_t* bytes,
                                           size_t length,
                                           wz_interface1_rom_variant_t variant);
wz_result_t wz_machine_clear_interface1_rom(wz_machine_t* machine);
wz_result_t wz_machine_set_interface1_rom_page(wz_machine_t* machine,
                                               wz_byte_t page);
wz_interface1_rom_variant_t wz_machine_interface1_rom_variant(
    const wz_machine_t* machine);
bool wz_machine_has_interface1_rom(const wz_machine_t* machine);
wz_byte_t wz_machine_interface1_rom_page(const wz_machine_t* machine);
wz_qword_t wz_machine_interface1_rom_identity(const wz_machine_t* machine);
wz_result_t wz_machine_interface1_control_write(wz_machine_t* machine,
                                                wz_byte_t value,
                                                wz_master_tick_t master_tick);
wz_byte_t wz_machine_interface1_control_latch(const wz_machine_t* machine);
wz_byte_t wz_machine_interface1_previous_control_latch(const wz_machine_t* machine);
wz_byte_t wz_machine_interface1_motor_shift(const wz_machine_t* machine);
wz_byte_t wz_machine_interface1_active_motor(const wz_machine_t* machine);
wz_master_tick_t wz_machine_interface1_control_latch_tick(
    const wz_machine_t* machine);
bool wz_machine_interface1_port_selected(wz_word_t address, wz_byte_t port_low);
wz_qword_t wz_machine_rom_identity(const wz_byte_t* bytes, size_t length);
wz_byte_t wz_machine_memory_read(const wz_machine_t* machine, wz_word_t address);
void wz_machine_memory_write(wz_machine_t* machine, wz_word_t address,
                             wz_byte_t value);
wz_result_t wz_machine_128k_paging_write(wz_machine_t* machine,
                                         wz_word_t address,
                                         wz_byte_t value);
wz_byte_t wz_machine_128k_paging_value(const wz_machine_t* machine);
wz_byte_t wz_machine_128k_screen_bank(const wz_machine_t* machine);
wz_byte_t wz_machine_128k_rom_bank(const wz_machine_t* machine);
void wz_machine_memory_write_at_tick(wz_machine_t* machine, wz_word_t address,
                                     wz_byte_t value, wz_master_tick_t master_tick);
bool wz_machine_ula_port_fe_selected(wz_word_t address);
wz_byte_t wz_machine_beeper_level(const wz_machine_t* machine);
wz_byte_t wz_machine_mic_level(const wz_machine_t* machine);
size_t wz_machine_beeper_events(const wz_machine_t* machine,
                                wz_beeper_event_t* events,
                                size_t capacity);
wz_result_t wz_machine_mic_capture_begin(wz_machine_t* machine);
wz_result_t wz_machine_mic_capture_end(wz_machine_t* machine);
size_t wz_machine_mic_events(const wz_machine_t* machine,
                             wz_mic_event_t* events,
                             size_t capacity);
size_t wz_machine_ay_events(const wz_machine_t* machine,
                            wz_ay_event_t* events, size_t capacity);
wz_byte_t wz_machine_ay_selected_register(const wz_machine_t* machine);
wz_byte_t wz_machine_ay_register_value(const wz_machine_t* machine,
                                       wz_byte_t register_index);
bool wz_machine_mic_capture_overflowed(const wz_machine_t* machine);
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
