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
    message(FATAL_ERROR "trace fixture failed: ${fixture_error}")
endif()

execute_process(COMMAND "${TRACE_DUMPER}" "${TRACE_PATH}"
    RESULT_VARIABLE dump_result OUTPUT_VARIABLE dump_output
    ERROR_VARIABLE dump_error)
if(NOT dump_result STREQUAL "0")
    message(FATAL_ERROR "trace dumper failed: ${dump_error}")
endif()

set(required_output
    "bus_timing cycle=memory_read contention_delay_tstates=3"
    "requested_tick=28674 requested_frame=0 line=64 line_tstate=1 phase=0"
    "ula=active,row=0,cell=0,offset=2"
    "bus_tick=28680 bus_frame=0 line=64 line_tstate=4 phase=0"
    "ula=attribute,row=0,cell=1,offset=1")
foreach(fragment IN LISTS required_output)
    string(FIND "${dump_output}" "${fragment}" match_index)
    if(match_index EQUAL -1)
        message(FATAL_ERROR "trace timing output missing '${fragment}': ${dump_output}")
    endif()
endforeach()

if(NOT dump_error MATCHES "recovered=1")
    message(FATAL_ERROR "trace recovery count is missing: ${dump_error}")
endif()
message(STATUS "PASS trace dump reconstructs CPU bus timing and ULA fetch position")
