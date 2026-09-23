/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_PRINTER_H
#define WZ_CORE_WZ_PRINTER_H

#include <stddef.h>

#include "core/wz_types.h"

#define WZ_PRINTER_PORT 0xfbu
#define WZ_PRINTER_BUFFER_CAPACITY 256u
#define WZ_PRINTER_OUTPUT_CAPACITY 512u

typedef enum {
    WZ_PRINTER_MODE_NONE = 0,
    WZ_PRINTER_MODE_EPSON = 1,
    WZ_PRINTER_MODE_EPSON_ENLARGED = 2,
    WZ_PRINTER_MODE_HP = 3,
    WZ_PRINTER_MODE_HP_ENLARGED = 4
} wz_printer_mode_t;

typedef struct {
    wz_printer_mode_t mode;
    wz_byte_t rows;
    wz_master_tick_t master_tick;
    size_t length;
    wz_byte_t data[WZ_PRINTER_OUTPUT_CAPACITY];
} wz_printer_flush_event_t;

typedef struct {
    wz_printer_mode_t mode;
    wz_byte_t last_output;
    wz_byte_t motor_on;
    wz_byte_t row;
    wz_word_t count;
    wz_byte_t buffer[WZ_PRINTER_BUFFER_CAPACITY];
    wz_byte_t hp_accumulator;
    wz_byte_t hp_bits;
    wz_printer_flush_event_t pending_flush;
    wz_qword_t flush_count;
} wz_printer_t;

void wz_printer_init(wz_printer_t* printer);
wz_result_t wz_printer_set_mode(wz_printer_t* printer, wz_printer_mode_t mode);
wz_printer_mode_t wz_printer_mode(const wz_printer_t* printer);
wz_byte_t wz_printer_status(const wz_printer_t* printer);
wz_result_t wz_printer_write_control(wz_printer_t* printer,
                                     wz_byte_t value,
                                     wz_master_tick_t master_tick);
wz_result_t wz_printer_take_flush(wz_printer_t* printer,
                                  wz_printer_flush_event_t* event);
bool wz_printer_port_selected(wz_word_t address);

#endif
