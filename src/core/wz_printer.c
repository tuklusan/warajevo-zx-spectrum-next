/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_printer.h"

#include <string.h>

static void clear_pending(wz_printer_t* printer)
{
    printer->pending_flush.length = 0u;
}

static void capture_epson(wz_printer_t* printer, wz_master_tick_t master_tick)
{
    size_t index;
    size_t output_length = printer->mode == WZ_PRINTER_MODE_EPSON_ENLARGED ?
        WZ_PRINTER_BUFFER_CAPACITY * 2u : WZ_PRINTER_BUFFER_CAPACITY;
    clear_pending(printer);
    printer->pending_flush.mode = printer->mode;
    printer->pending_flush.rows = printer->row;
    printer->pending_flush.master_tick = master_tick;
    printer->pending_flush.length = output_length;
    for (index = 0u; index < WZ_PRINTER_BUFFER_CAPACITY; ++index) {
        printer->pending_flush.data[index] = printer->buffer[index];
        if (output_length > WZ_PRINTER_BUFFER_CAPACITY) {
            printer->pending_flush.data[index + WZ_PRINTER_BUFFER_CAPACITY] =
                printer->buffer[index];
        }
    }
    printer->flush_count++;
    memset(printer->buffer, 0, sizeof(printer->buffer));
    printer->row = 0u;
}

static void capture_hp(wz_printer_t* printer, wz_master_tick_t master_tick)
{
    clear_pending(printer);
    printer->pending_flush.mode = printer->mode;
    printer->pending_flush.rows = printer->row;
    printer->pending_flush.master_tick = master_tick;
    printer->pending_flush.length = 1u;
    printer->pending_flush.data[0] = printer->hp_accumulator;
    printer->flush_count++;
    printer->hp_accumulator = 0u;
    printer->hp_bits = 0u;
}

static void flush_pending(wz_printer_t* printer, wz_master_tick_t master_tick)
{
    if (printer->mode >= WZ_PRINTER_MODE_HP) {
        if (printer->hp_bits != 0u) capture_hp(printer, master_tick);
    } else if (printer->count != WZ_PRINTER_BUFFER_CAPACITY) {
        capture_epson(printer, master_tick);
    }
}

void wz_printer_init(wz_printer_t* printer)
{
    if (printer != NULL) {
        memset(printer, 0, sizeof(*printer));
        printer->mode = WZ_PRINTER_MODE_NONE;
        printer->count = WZ_PRINTER_BUFFER_CAPACITY;
    }
}

wz_result_t wz_printer_set_mode(wz_printer_t* printer, wz_printer_mode_t mode)
{
    if (printer == NULL || mode > WZ_PRINTER_MODE_HP_ENLARGED) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    wz_printer_init(printer);
    printer->mode = mode;
    return WZ_RESULT_OK;
}

wz_printer_mode_t wz_printer_mode(const wz_printer_t* printer)
{
    return printer == NULL ? WZ_PRINTER_MODE_NONE : printer->mode;
}

wz_byte_t wz_printer_status(const wz_printer_t* printer)
{
    wz_byte_t status = 0x3eu;
    if (printer == NULL || printer->mode == WZ_PRINTER_MODE_NONE) {
        return (wz_byte_t)(status | 0x40u);
    }
    if ((printer->last_output & 0x80u) != 0u) status |= 0x80u;
    status |= 0x01u;
    return status;
}

static void write_epson_pixel(wz_printer_t* printer,
                              wz_byte_t value,
                              wz_master_tick_t master_tick)
{
    wz_word_t index;
    wz_byte_t pixel = value;
    if (printer->mode == WZ_PRINTER_MODE_EPSON_ENLARGED) {
        pixel = (wz_byte_t)((pixel >> 1u) | pixel);
    }
    if (printer->count == 0u) return;
    index = (wz_word_t)(printer->count - 1u);
    if (printer->row != 0u) {
        unsigned shift = printer->row;
        pixel = (wz_byte_t)((pixel >> shift) | (pixel << (8u - shift)));
        printer->buffer[index] |= pixel;
    } else {
        printer->buffer[index] = pixel;
    }
    printer->count = index;
    if (printer->count == 0u) {
        printer->count = WZ_PRINTER_BUFFER_CAPACITY;
        printer->row = (wz_byte_t)(printer->row + printer->mode);
        printer->motor_on = 0u;
        if ((printer->last_output & 0x02u) != 0u || printer->row >= 8u) {
            capture_epson(printer, master_tick);
        }
    }
}

static void write_hp_pixel(wz_printer_t* printer,
                           wz_byte_t value,
                           wz_master_tick_t master_tick)
{
    printer->hp_accumulator = (wz_byte_t)((printer->hp_accumulator << 1u) |
                                           ((value >> 7u) & 1u));
    printer->hp_bits++;
    if (printer->hp_bits == 8u) capture_hp(printer, master_tick);
}

wz_result_t wz_printer_write_control(wz_printer_t* printer,
                                     wz_byte_t value,
                                     wz_master_tick_t master_tick)
{
    if (printer == NULL) return WZ_RESULT_INVALID_ARGUMENT;
    printer->last_output = value;
    if ((value & 0x04u) != 0u) {
        if (printer->motor_on != 0u) {
            flush_pending(printer, master_tick);
            printer->motor_on = 0u;
        }
        return WZ_RESULT_OK;
    }
    if (printer->mode == WZ_PRINTER_MODE_NONE) return WZ_RESULT_OK;
    if (printer->motor_on == 0u) {
        printer->motor_on = 1u;
        return WZ_RESULT_OK;
    }
    if (printer->mode >= WZ_PRINTER_MODE_HP) {
        write_hp_pixel(printer, value, master_tick);
    } else {
        write_epson_pixel(printer, value, master_tick);
    }
    return WZ_RESULT_OK;
}

wz_result_t wz_printer_take_flush(wz_printer_t* printer,
                                  wz_printer_flush_event_t* event)
{
    if (printer == NULL || event == NULL) return WZ_RESULT_INVALID_ARGUMENT;
    if (printer->pending_flush.length == 0u) return WZ_RESULT_INVALID_STATE;
    *event = printer->pending_flush;
    clear_pending(printer);
    return WZ_RESULT_OK;
}

bool wz_printer_port_selected(wz_word_t address)
{
    return (address & 0xffu) == WZ_PRINTER_PORT;
}
