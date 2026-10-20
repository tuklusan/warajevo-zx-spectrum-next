/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <stdint.h>
#include <stdio.h>

#include "core/wz_machine.h"
#include "core/wz_machine_internal.h"
#include "core/wz_machine_profile.h"
#include "core/wz_trace.h"
#include "core/wz_zxnet.h"
#include "diagnostics/wz_trace_file.h"

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "ZX Net trace fixture failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

typedef struct {
    size_t calls;
    wz_word_t block_id;
    size_t length;
} write_log_t;

static wz_result_t accept_write(wz_word_t block_id, const wz_byte_t* data,
                                size_t length, void* opaque)
{
    write_log_t* log = (write_log_t*)opaque;
    if (log == NULL || data == NULL || length != 1u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    ++log->calls;
    log->block_id = block_id;
    log->length = length;
    return WZ_RESULT_OK;
}

int main(int argc, char** argv)
{
    wz_machine_t machine = {0};
    wz_zxnet_t transmit = {0};
    wz_trace_file_t trace_file = {0};
    wz_trace_sink_t trace_sink = {0};
    wz_byte_t input[WZ_ZXNET_DATA_CAPACITY] = {0};
    wz_byte_t value = 0u;
    write_log_t write_log = {0};
    const wz_dword_t event_mask = UINT32_C(1) << WZ_TRACE_ZXNET_EVENT;
    int result = 1;

    REQUIRE(argc == 2);
    REQUIRE(wz_machine_init(&machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_trace_file_create(&trace_file, argv[1], 1u,
            (wz_dword_t)WZ_MACHINE_48K_PAL, 0u, event_mask) == WZ_RESULT_OK);
    wz_trace_sink_init(&trace_sink, wz_trace_file_emit, &trace_file);
    wz_machine_set_timing_trace(&machine, &trace_sink);

    REQUIRE(wz_zxnet_feed_read_block(&machine.zxnet, 7u, input,
            sizeof(input)) == WZ_RESULT_OK);
    wz_machine_advance_master_ticks(&machine, 10u);
    REQUIRE(wz_zxnet_start_read_collection(&machine.zxnet) == WZ_RESULT_OK);
    for (size_t index = 0u; index < 9u; ++index) {
        wz_machine_advance_master_ticks(&machine, 1u);
        REQUIRE(wz_zxnet_read(&machine.zxnet, &value) == WZ_RESULT_OK);
    }
    REQUIRE(machine.zxnet.state.buffer_position == 1u);

    wz_zxnet_init(&transmit);
    wz_zxnet_set_timing_trace(&transmit, &trace_sink);
    wz_zxnet_set_master_tick(&transmit, 100u);
    REQUIRE(wz_zxnet_begin_claim(&transmit, 0x42u) == WZ_RESULT_OK);
    wz_zxnet_set_master_tick(&transmit, 101u);
    REQUIRE(wz_zxnet_accept_claim(&transmit, true) == WZ_RESULT_OK);
    wz_zxnet_set_write_callback(&transmit, accept_write, &write_log);
    for (size_t index = 0u; index < 10u; ++index) {
        wz_zxnet_set_master_tick(&transmit, (wz_master_tick_t)(102u + index));
        REQUIRE(wz_zxnet_write_bit(&transmit, (index & 1u) != 0u) ==
                WZ_RESULT_OK);
    }
    wz_zxnet_set_master_tick(&transmit, 120u);
    REQUIRE(wz_zxnet_finish_write(&transmit) == WZ_RESULT_OK);
    REQUIRE(write_log.calls == 1u && write_log.block_id == 1u &&
            write_log.length == 1u);

    wz_machine_set_timing_trace(&machine, NULL);
    wz_zxnet_set_timing_trace(&transmit, NULL);
    REQUIRE(wz_trace_file_freeze(&trace_file) == WZ_RESULT_OK);
    result = 0;

cleanup:
    wz_machine_set_timing_trace(&machine, NULL);
    wz_zxnet_set_timing_trace(&transmit, NULL);
    wz_trace_file_close(&trace_file);
    wz_machine_destroy(&machine);
    return result;
}
