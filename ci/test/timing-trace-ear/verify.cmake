file(REMOVE "${TRACE_PATH}")
execute_process(COMMAND "${TRACE_FIXTURE}" "${TRACE_PATH}"
    RESULT_VARIABLE fixture_result ERROR_VARIABLE fixture_error)
if(NOT fixture_result STREQUAL "0")
    message(FATAL_ERROR "EAR trace fixture failed: ${fixture_error}")
endif()

execute_process(COMMAND "${TRACE_DUMPER}" "${TRACE_PATH}"
    RESULT_VARIABLE dump_result OUTPUT_VARIABLE dump_output
    ERROR_VARIABLE dump_error)
if(NOT dump_result STREQUAL "0")
    message(FATAL_ERROR "EAR trace dump failed: ${dump_error}")
endif()

foreach(fragment
        "ear_transition from=0 to=1 tick=5"
        "ear_transition from=1 to=0 tick=12"
        "ear_transition from=0 to=1 tick=21")
    string(FIND "${dump_output}" "${fragment}" match_index)
    if(match_index EQUAL -1)
        message(FATAL_ERROR "EAR trace output missing '${fragment}': ${dump_output}")
    endif()
endforeach()

string(REGEX MATCHALL "kind=ear_change\\(12\\)" ear_events "${dump_output}")
list(LENGTH ear_events ear_event_count)
if(NOT ear_event_count EQUAL 3)
    message(FATAL_ERROR "expected three EAR transitions, got ${ear_event_count}: ${dump_output}")
endif()

message(STATUS "PASS tape EAR transitions follow CPU master-tick advancement")
