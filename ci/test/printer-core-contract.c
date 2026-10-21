/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "core/wz_printer.h"

#include <stdio.h>
#include <string.h>

static unsigned checks;

#define CHECK(expr) do { \
    checks++; \
    if (!(expr)) { \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expr); \
        return 1; \
    } \
} while (0)

static int test_modes_and_status(void)
{
    wz_printer_t printer;

    wz_printer_init(&printer);
    CHECK(wz_printer_mode(&printer) == WZ_PRINTER_MODE_NONE);
    CHECK(wz_printer_status(&printer) == 0x7eu);
    CHECK(wz_printer_port_selected(0x12fbu));
    CHECK(!wz_printer_port_selected(0x12fau));
    CHECK(wz_printer_set_mode(NULL, WZ_PRINTER_MODE_EPSON) ==
          WZ_RESULT_INVALID_ARGUMENT);
    CHECK(wz_printer_set_mode(&printer, (wz_printer_mode_t)5) ==
          WZ_RESULT_INVALID_ARGUMENT);
    CHECK(wz_printer_set_mode(&printer, WZ_PRINTER_MODE_EPSON) ==
          WZ_RESULT_OK);
    CHECK(wz_printer_status(&printer) == 0x3fu);
    CHECK(wz_printer_write_control(&printer, 0x80u, 1u) == WZ_RESULT_OK);
    CHECK(wz_printer_status(&printer) == 0xbfu);
    CHECK(printer.motor_on == 1u);
    CHECK(printer.count == WZ_PRINTER_BUFFER_CAPACITY);
    return 0;
}

static int test_epson_rows(void)
{
    wz_printer_t printer;
    wz_printer_flush_event_t event;
    wz_master_tick_t tick = 0u;
    size_t row;
    size_t pixel;

    CHECK(wz_printer_set_mode(&printer, WZ_PRINTER_MODE_EPSON) ==
          WZ_RESULT_OK);
    for (row = 0u; row < 8u; ++row) {
        CHECK(wz_printer_write_control(&printer, 0x00u, ++tick) ==
              WZ_RESULT_OK);
        for (pixel = 0u; pixel < WZ_PRINTER_BUFFER_CAPACITY; ++pixel) {
            CHECK(wz_printer_write_control(&printer, 0x80u, ++tick) ==
                  WZ_RESULT_OK);
        }
    }
    CHECK(printer.flush_count == 1u);
    CHECK(printer.row == 0u);
    CHECK(printer.motor_on == 0u);
    CHECK(wz_printer_take_flush(&printer, &event) == WZ_RESULT_OK);
    CHECK(event.mode == WZ_PRINTER_MODE_EPSON);
    CHECK(event.rows == 8u);
    CHECK(event.master_tick == tick);
    CHECK(event.length == WZ_PRINTER_BUFFER_CAPACITY);
    for (pixel = 0u; pixel < event.length; ++pixel) {
        CHECK(event.data[pixel] == 0xffu);
    }
    CHECK(wz_printer_take_flush(&printer, &event) == WZ_RESULT_INVALID_STATE);
    return 0;
}

static int test_epson_enlarged_and_partial_flush(void)
{
    wz_printer_t printer;
    wz_printer_flush_event_t event;
    size_t row;
    size_t pixel;
    wz_master_tick_t tick = 0u;

    CHECK(wz_printer_set_mode(&printer, WZ_PRINTER_MODE_EPSON_ENLARGED) ==
          WZ_RESULT_OK);
    for (row = 0u; row < 4u; ++row) {
        CHECK(wz_printer_write_control(&printer, 0x00u, ++tick) ==
              WZ_RESULT_OK);
        for (pixel = 0u; pixel < WZ_PRINTER_BUFFER_CAPACITY; ++pixel) {
            CHECK(wz_printer_write_control(&printer, 0x80u, ++tick) ==
                  WZ_RESULT_OK);
        }
    }
    CHECK(wz_printer_take_flush(&printer, &event) == WZ_RESULT_OK);
    CHECK(event.mode == WZ_PRINTER_MODE_EPSON_ENLARGED);
    CHECK(event.rows == 8u);
    CHECK(event.length == WZ_PRINTER_BUFFER_CAPACITY * 2u);
    for (pixel = 0u; pixel < WZ_PRINTER_BUFFER_CAPACITY; ++pixel) {
        CHECK(event.data[pixel] == 0xffu);
        CHECK(event.data[pixel + WZ_PRINTER_BUFFER_CAPACITY] == 0xffu);
    }

    CHECK(wz_printer_set_mode(&printer, WZ_PRINTER_MODE_EPSON) ==
          WZ_RESULT_OK);
    CHECK(wz_printer_write_control(&printer, 0x00u, ++tick) == WZ_RESULT_OK);
    CHECK(wz_printer_write_control(&printer, 0x80u, ++tick) == WZ_RESULT_OK);
    CHECK(wz_printer_write_control(&printer, 0x80u, ++tick) == WZ_RESULT_OK);
    CHECK(wz_printer_write_control(&printer, 0x04u, ++tick) == WZ_RESULT_OK);
    CHECK(printer.motor_on == 0u);
    CHECK(wz_printer_take_flush(&printer, &event) == WZ_RESULT_OK);
    CHECK(event.length == WZ_PRINTER_BUFFER_CAPACITY);
    CHECK(event.master_tick == tick);
    CHECK(event.data[WZ_PRINTER_BUFFER_CAPACITY - 1u] == 0x80u);
    CHECK(event.data[WZ_PRINTER_BUFFER_CAPACITY - 2u] == 0x80u);
    return 0;
}

static int test_hp_bit_order_and_api_errors(void)
{
    wz_printer_t printer;
    wz_printer_flush_event_t event;
    const wz_byte_t bits[8] = {1u, 0u, 1u, 0u, 0u, 1u, 0u, 1u};
    size_t index;

    CHECK(wz_printer_set_mode(&printer, WZ_PRINTER_MODE_HP) == WZ_RESULT_OK);
    CHECK(wz_printer_take_flush(&printer, &event) == WZ_RESULT_INVALID_STATE);
    CHECK(wz_printer_write_control(&printer, 0x00u, 10u) == WZ_RESULT_OK);
    for (index = 0u; index < 8u; ++index) {
        CHECK(wz_printer_write_control(&printer,
                                       bits[index] != 0u ? 0x80u : 0x00u,
                                       (wz_master_tick_t)(11u + index)) ==
              WZ_RESULT_OK);
    }
    CHECK(wz_printer_take_flush(&printer, &event) == WZ_RESULT_OK);
    CHECK(event.mode == WZ_PRINTER_MODE_HP);
    CHECK(event.length == 1u);
    CHECK(event.data[0] == 0xa5u);
    CHECK(event.master_tick == 18u);
    CHECK(wz_printer_take_flush(NULL, &event) == WZ_RESULT_INVALID_ARGUMENT);
    CHECK(wz_printer_take_flush(&printer, NULL) == WZ_RESULT_INVALID_ARGUMENT);
    CHECK(wz_printer_write_control(NULL, 0u, 0u) ==
          WZ_RESULT_INVALID_ARGUMENT);
    return 0;
}

int main(void)
{
    if (test_modes_and_status() != 0 ||
        test_epson_rows() != 0 ||
        test_epson_enlarged_and_partial_flush() != 0 ||
        test_hp_bit_order_and_api_errors() != 0) {
        return 1;
    }
    printf("PASS printer core contract: %u assertions\n", checks);
    return 0;
}
