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
    message(FATAL_ERROR "MIC trace fixture failed: ${fixture_error}")
endif()

execute_process(COMMAND "${TRACE_DUMPER}" "${TRACE_PATH}"
    RESULT_VARIABLE dump_result OUTPUT_VARIABLE dump_output
    ERROR_VARIABLE dump_error)
if(NOT dump_result STREQUAL "0")
    message(FATAL_ERROR "MIC trace dump failed: ${dump_error}")
endif()

foreach(fragment
        "mic_transition port=00fe from=0 to=1 tick=10"
        "mic_transition port=00fe from=1 to=0 tick=25")
    string(FIND "${dump_output}" "${fragment}" match_index)
    if(match_index EQUAL -1)
        message(FATAL_ERROR "MIC trace output missing '${fragment}': ${dump_output}")
    endif()
endforeach()

string(REGEX MATCHALL "kind=mic_change\\(11\\)" mic_events "${dump_output}")
list(LENGTH mic_events mic_event_count)
if(NOT mic_event_count EQUAL 2)
    message(FATAL_ERROR "expected two MIC transitions, got ${mic_event_count}: ${dump_output}")
endif()

message(STATUS "PASS trace records MIC line transitions at the ULA write tick")
