# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

file(REMOVE "${TRACE_PATH}")
execute_process(COMMAND "${TRACE_FIXTURE}" "${TRACE_PATH}"
    RESULT_VARIABLE fixture_result ERROR_VARIABLE fixture_error)
if(NOT fixture_result STREQUAL "0")
    message(FATAL_ERROR "ZX Net trace fixture failed: ${fixture_error}")
endif()

execute_process(COMMAND "${TRACE_DUMPER}" "${TRACE_PATH}"
    RESULT_VARIABLE dump_result OUTPUT_VARIABLE dump_output
    ERROR_VARIABLE dump_error)
if(NOT dump_result STREQUAL "0")
    message(FATAL_ERROR "ZX Net trace dump failed: ${dump_error}")
endif()

foreach(fragment
        "zxnet_event action=block_queued state=IDLE->IDLE block=0000->0007 position=0 bit_count=9 tick=0"
        "zxnet_event action=read_started state=IDLE->COLLREAD block=0007->0007 position=0 bit_count=9 tick=10"
        "zxnet_event action=read_progress state=COLLREAD->COLLREAD block=0007->0007 position=0 bit_count=8 tick=11"
        "zxnet_event action=read_progress state=COLLREAD->COLLREAD block=0007->0007 position=1 bit_count=9 tick=19"
        "zxnet_event action=claim_started state=IDLE->CLAIM block=0000->0000 position=0 bit_count=9 tick=100"
        "zxnet_event action=claim_resolved state=CLAIM->COLLWRITE block=0000->0000 position=0 bit_count=9 tick=101"
        "zxnet_event action=write_progress state=COLLWRITE->COLLWRITE block=0000->0000 position=0 bit_count=8 tick=102"
        "zxnet_event action=write_progress state=COLLWRITE->COLLWRITE block=0000->0000 position=1 bit_count=9 tick=111"
        "zxnet_event action=write_committed state=COLLWRITE->IDLE block=0000->0001 position=0 bit_count=9 tick=120")
    string(FIND "${dump_output}" "${fragment}" match_index)
    if(match_index EQUAL -1)
        message(FATAL_ERROR "ZX Net trace output missing '${fragment}': ${dump_output}")
    endif()
endforeach()

string(REGEX MATCHALL "kind=zxnet_event\\(14\\)" zxnet_events "${dump_output}")
list(LENGTH zxnet_events zxnet_event_count)
if(NOT zxnet_event_count EQUAL 24)
    message(FATAL_ERROR "expected 24 ZX Net state/progress records, got ${zxnet_event_count}: ${dump_output}")
endif()

message(STATUS "PASS ZX Net block, state, read/write progress, and commit trace exact master ticks")
