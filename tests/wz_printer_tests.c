/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_printer.h"
#include "core/wz_machine.h"

#include <stdio.h>

static int test_modes_and_status(void)
{
    wz_printer_t printer;
    wz_printer_init(&printer);
    if (wz_printer_status(&printer) != 0x7eu ||
        wz_printer_set_mode(&printer, WZ_PRINTER_MODE_EPSON) != WZ_RESULT_OK ||
        wz_printer_status(&printer) != 0x3fu ||
        !wz_printer_port_selected(0x12fbu) ||
        wz_printer_port_selected(0x12fau)) return 1;
    if (wz_printer_set_mode(&printer, WZ_PRINTER_MODE_HP_ENLARGED) != WZ_RESULT_OK ||
        wz_printer_mode(&printer) != WZ_PRINTER_MODE_HP_ENLARGED ||
        wz_printer_set_mode(&printer, (wz_printer_mode_t)5u) != WZ_RESULT_INVALID_ARGUMENT) {
        return 1;
    }
    return 0;
}

static int test_hp_capture_and_motor_edge(void)
{
    wz_printer_t printer;
    wz_printer_flush_event_t event;
    wz_printer_init(&printer);
    if (wz_printer_set_mode(&printer, WZ_PRINTER_MODE_HP) != WZ_RESULT_OK ||
        wz_printer_write_control(&printer, 0x80u, 10u) != WZ_RESULT_OK) return 1;
    for (unsigned index = 0u; index < 8u; ++index) {
        if (wz_printer_write_control(&printer,
                                     index == 0u ? 0x80u : 0u,
                                     (wz_master_tick_t)(11u + index)) != WZ_RESULT_OK) {
            return 1;
        }
    }
    if (wz_printer_take_flush(&printer, &event) != WZ_RESULT_OK ||
        event.mode != WZ_PRINTER_MODE_HP || event.length != 1u ||
        event.data[0] != 0x80u ||
        wz_printer_take_flush(&printer, &event) != WZ_RESULT_INVALID_STATE) return 1;
    if (wz_printer_write_control(&printer, 0x04u, 30u) != WZ_RESULT_OK ||
        wz_printer_status(&printer) != 0x3fu) return 1;
    return 0;
}

static int test_epson_flushes(void)
{
    wz_printer_t printer;
    wz_printer_flush_event_t event;
    wz_printer_init(&printer);
    if (wz_printer_set_mode(&printer, WZ_PRINTER_MODE_EPSON) != WZ_RESULT_OK) return 1;
    for (unsigned row = 0u; row < 8u; ++row) {
        if (wz_printer_write_control(&printer, 0u, row * 300u) != WZ_RESULT_OK) return 1;
        for (unsigned pixel = 0u; pixel < WZ_PRINTER_BUFFER_CAPACITY; ++pixel) {
            if (wz_printer_write_control(&printer, 0x80u,
                                         (wz_master_tick_t)(row * 300u + pixel + 1u)) !=
                WZ_RESULT_OK) return 1;
        }
    }
    if (wz_printer_take_flush(&printer, &event) != WZ_RESULT_OK ||
        event.mode != WZ_PRINTER_MODE_EPSON || event.length != 256u ||
        event.rows != 8u || event.data[0] != 0xffu || event.data[255] != 0xffu) {
        return 1;
    }
    wz_printer_init(&printer);
    if (wz_printer_set_mode(&printer, WZ_PRINTER_MODE_EPSON_ENLARGED) != WZ_RESULT_OK ||
        wz_printer_write_control(&printer, 0u, 1u) != WZ_RESULT_OK) return 1;
    for (unsigned row = 0u; row < 4u; ++row) {
        for (unsigned pixel = 0u; pixel < WZ_PRINTER_BUFFER_CAPACITY; ++pixel) {
            if (wz_printer_write_control(&printer, 0x80u,
                                         (wz_master_tick_t)(1000u + row * 300u + pixel)) !=
                WZ_RESULT_OK) return 1;
        }
        if (row != 3u && wz_printer_write_control(&printer, 0u, 2000u + row) !=
            WZ_RESULT_OK) return 1;
    }
    if (wz_printer_take_flush(&printer, &event) != WZ_RESULT_OK ||
        event.mode != WZ_PRINTER_MODE_EPSON_ENLARGED || event.length != 512u) return 1;
    return 0;
}

static int test_partial_motor_off_flush_and_bus(void)
{
    wz_printer_t printer;
    wz_printer_flush_event_t event;
    wz_machine_t machine;
    wz_bus_request_t request;
    wz_printer_init(&printer);
    if (wz_printer_set_mode(&printer, WZ_PRINTER_MODE_EPSON) != WZ_RESULT_OK ||
        wz_printer_write_control(&printer, 0u, 1u) != WZ_RESULT_OK ||
        wz_printer_write_control(&printer, 0x80u, 2u) != WZ_RESULT_OK ||
        wz_printer_write_control(&printer, 0x04u, 3u) != WZ_RESULT_OK ||
        wz_printer_take_flush(&printer, &event) != WZ_RESULT_OK ||
        event.length != 256u) return 1;
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) return 1;
    if (wz_machine_set_printer_mode(&machine, WZ_PRINTER_MODE_HP) != WZ_RESULT_OK) {
        wz_machine_destroy(&machine); return 1;
    }
    wz_bus_request_init(&request, WZ_BUS_IO_READ, 10u, 0x12fbu, 0u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        request.source != WZ_BUS_SOURCE_PRINTER || request.value != 0x3fu) {
        wz_machine_destroy(&machine); return 1;
    }
    wz_bus_request_init(&request, WZ_BUS_IO_WRITE, 20u, 0x12fbu, 0u, 4u);
    if (wz_machine_bus_request(&machine, &request) != WZ_RESULT_OK ||
        request.source != WZ_BUS_SOURCE_PRINTER) {
        wz_machine_destroy(&machine); return 1;
    }
    wz_machine_destroy(&machine);
    return 0;
}

int main(void)
{
    if (test_modes_and_status() != 0 || test_hp_capture_and_motor_edge() != 0 ||
        test_epson_flushes() != 0 || test_partial_motor_off_flush_and_bus() != 0) {
        (void)fprintf(stderr, "printer tests failed\n");
        return 1;
    }
    return 0;
}
