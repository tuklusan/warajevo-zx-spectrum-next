# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

file(REMOVE
    "${TRACE_DIRECTORY}/wrapped-sequence.wztrace"
    "${TRACE_DIRECTORY}/abrupt-append.wztrace")
execute_process(COMMAND "${TRACE_FIXTURE}" "${TRACE_DIRECTORY}"
    RESULT_VARIABLE fixture_result ERROR_VARIABLE fixture_error)
if(NOT fixture_result STREQUAL "0")
    message(FATAL_ERROR "trace fixture failed: ${fixture_error}")
endif()

execute_process(COMMAND "${TRACE_DUMPER}"
    "${TRACE_DIRECTORY}/wrapped-sequence.wztrace"
    RESULT_VARIABLE wrapped_result OUTPUT_VARIABLE wrapped_output
    ERROR_VARIABLE wrapped_error)
if(NOT wrapped_result STREQUAL "0")
    message(FATAL_ERROR "wrapped trace dump failed: ${wrapped_error}")
endif()

set(wrapped_fragments
    "sequence=4294967295 tick=1005"
    "sequence=4294967296 tick=1011"
    "sequence=4294967297 tick=1018")
foreach(fragment IN LISTS wrapped_fragments)
    string(FIND "${wrapped_output}" "${fragment}" match_index)
    if(match_index EQUAL -1)
        message(FATAL_ERROR "wrapped output missing '${fragment}': ${wrapped_output}")
    endif()
endforeach()
if(NOT wrapped_error MATCHES "recovered=3")
    message(FATAL_ERROR "wrapped trace recovery count is wrong: ${wrapped_error}")
endif()

execute_process(COMMAND "${TRACE_DUMPER}"
    "${TRACE_DIRECTORY}/abrupt-append.wztrace"
    RESULT_VARIABLE abrupt_result OUTPUT_VARIABLE abrupt_output
    ERROR_VARIABLE abrupt_error)
if(NOT abrupt_result STREQUAL "0")
    message(FATAL_ERROR "abrupt trace dump failed: ${abrupt_error}")
endif()

foreach(fragment "sequence=42 tick=77" "sequence=43 tick=78")
    string(FIND "${abrupt_output}" "${fragment}" match_index)
    if(match_index EQUAL -1)
        message(FATAL_ERROR "abrupt output missing '${fragment}': ${abrupt_output}")
    endif()
endforeach()
if(NOT abrupt_error MATCHES "recovered=2")
    message(FATAL_ERROR "abrupt trace recovery count is wrong: ${abrupt_error}")
endif()

message(STATUS "PASS standalone trace dump recovers circular wrap, sequence rollover, and torn append")
