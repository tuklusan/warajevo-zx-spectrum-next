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
    message(FATAL_ERROR "128K ULA trace fixture failed (${fixture_result}): ${fixture_error}")
endif()

execute_process(COMMAND "${TRACE_DUMPER}" "${TRACE_PATH}"
    RESULT_VARIABLE dump_result OUTPUT_VARIABLE dump_output
    ERROR_VARIABLE dump_error)
if(NOT dump_result STREQUAL "0")
    message(FATAL_ERROR "128K ULA trace dump failed: ${dump_error}")
endif()

foreach(fragment IN ITEMS
        "ula_fetch kind=bitmap row=0 cell=0 address=4000 value=a5 tick=28722"
        "ula_fetch kind=attribute row=0 cell=0 address=5800 value=47 tick=28724"
        "ula_fetch kind=bitmap row=0 cell=1 address=4001 value=5b tick=28730"
        "ula_fetch kind=attribute row=0 cell=1 address=5801 value=15 tick=28732")
    string(FIND "${dump_output}" "${fragment}" match_index)
    if(match_index EQUAL -1)
        message(FATAL_ERROR "128K trace is missing '${fragment}'")
    endif()
endforeach()
if(NOT dump_error MATCHES "recovered=12288")
    message(FATAL_ERROR "Expected 12,288 128K fetch records: ${dump_error}")
endif()
message(STATUS "PASS 128K PAL full-frame ULA trace follows screen-bank paging")
