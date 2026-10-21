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
    message(FATAL_ERROR "Interface 1 trace fixture failed: ${fixture_error}")
endif()

execute_process(COMMAND "${TRACE_DUMPER}" "${TRACE_PATH}"
    RESULT_VARIABLE dump_result OUTPUT_VARIABLE dump_output
    ERROR_VARIABLE dump_error)
if(NOT dump_result STREQUAL "0")
    message(FATAL_ERROR "Interface 1 trace dump failed: ${dump_error}")
endif()

foreach(fragment
        "interface1_io direction=write port=00ef value=06 control=06 motor_shift=ff active_motor=ff tick=10"
        "interface1_io direction=write port=a0ef value=04 control=04 motor_shift=fe active_motor=00 tick=20"
        "interface1_io direction=read port=80e7 value=00 control=04 motor_shift=fe active_motor=00 tick=30"
        "interface1_io direction=write port=00f7 value=01 control=04 motor_shift=fe active_motor=00 tick=40"
        "interface1_io direction=read port=80f7 value=ff control=04 motor_shift=fe active_motor=00 tick=50")
    string(FIND "${dump_output}" "${fragment}" match_index)
    if(match_index EQUAL -1)
        message(FATAL_ERROR "Interface 1 trace output missing '${fragment}': ${dump_output}")
    endif()
endforeach()

string(REGEX MATCHALL "kind=interface1_io\\(13\\)" interface1_events "${dump_output}")
list(LENGTH interface1_events interface1_event_count)
if(NOT interface1_event_count EQUAL 5)
    message(FATAL_ERROR "expected five Interface 1 I/O records, got ${interface1_event_count}: ${dump_output}")
endif()

message(STATUS "PASS Interface 1, Microdrive, and ZX Net port accesses retain exact ticks and latch context")
