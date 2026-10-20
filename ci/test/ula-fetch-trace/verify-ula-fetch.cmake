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
    message(FATAL_ERROR "ULA trace fixture failed: ${fixture_error}")
endif()

execute_process(COMMAND "${TRACE_DUMPER}" "${TRACE_PATH}"
    RESULT_VARIABLE dump_result OUTPUT_VARIABLE dump_output
    ERROR_VARIABLE dump_error)
if(NOT dump_result STREQUAL "0")
    message(FATAL_ERROR "ULA trace dump failed: ${dump_error}")
endif()

set(required_output
    "ula_fetch kind=bitmap row=0 cell=0 address=4000 value=a5 tick=28670"
    "frame=0 line=63 line_tstate=223 phase=0 ula=bitmap,row=0,cell=0,offset=0"
    "ula_fetch kind=attribute row=0 cell=0 address=5800 value=41 tick=28672"
    "ula_fetch kind=bitmap row=0 cell=1 address=4001 value=5a tick=28678"
    "ula_fetch kind=attribute row=0 cell=1 address=5801 value=73 tick=28680"
    "frame=0 line=64 line_tstate=4 phase=0 ula=attribute,row=0,cell=1,offset=1")
foreach(fragment IN LISTS required_output)
    string(FIND "${dump_output}" "${fragment}" match_index)
    if(match_index EQUAL -1)
        message(FATAL_ERROR "ULA trace output missing '${fragment}': ${dump_output}")
    endif()
endforeach()

if(NOT dump_error MATCHES "recovered=4")
    message(FATAL_ERROR "ULA trace recovery count is wrong: ${dump_error}")
endif()
message(STATUS "PASS trace dump reports timed ULA bitmap and attribute fetches")
