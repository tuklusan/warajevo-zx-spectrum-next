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
    message(FATAL_ERROR "Full-frame ULA trace fixture failed: ${fixture_error}")
endif()

execute_process(COMMAND "${TRACE_DUMPER}" "${TRACE_PATH}"
    RESULT_VARIABLE dump_result OUTPUT_VARIABLE dump_output
    ERROR_VARIABLE dump_error)
if(NOT dump_result STREQUAL "0")
    message(FATAL_ERROR "Full-frame ULA trace dump failed: ${dump_error}")
endif()

foreach(fragment IN ITEMS
        "ula_fetch kind=bitmap row=0 cell=0 address=4000"
        "ula_fetch kind=attribute row=0 cell=0 address=5800"
        "ula_fetch kind=bitmap row=191 cell=31 address=57ff"
        "ula_fetch kind=attribute row=191 cell=31 address=5aff")
    string(FIND "${dump_output}" "${fragment}" match_index)
    if(match_index EQUAL -1)
        message(FATAL_ERROR "Full-frame trace is missing '${fragment}'")
    endif()
endforeach()
if(NOT dump_error MATCHES "recovered=12288")
    message(FATAL_ERROR "Expected 12,288 fetch records: ${dump_error}")
endif()
message(STATUS "PASS complete 48K PAL frame has all timed ULA fetches and same-tick bus ordering")
