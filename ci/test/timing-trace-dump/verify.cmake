# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

execute_process(
    COMMAND "${TRACE_FIXTURE}" "${TRACE_PATH}"
    RESULT_VARIABLE fixture_status
    OUTPUT_VARIABLE fixture_output
    ERROR_VARIABLE fixture_error)
if(NOT fixture_status EQUAL 0)
    message(FATAL_ERROR "trace fixture failed: ${fixture_output}${fixture_error}")
endif()

execute_process(
    COMMAND "${TRACE_DUMPER}" "${TRACE_PATH}"
    RESULT_VARIABLE dump_status
    OUTPUT_VARIABLE dump_output
    ERROR_VARIABLE dump_error)
if(NOT dump_status EQUAL 0)
    message(FATAL_ERROR "trace dump failed: ${dump_output}${dump_error}")
endif()

set(expected_output
    "kind=cpu_state_sync(6)"
    "cpu_state sequence=5 tick=0 A=12 F=34 B=56 C=78 D=9a E=bc H=de L=f0"
    "_altA=13 _altF=35 _altB=57 _altC=79 _altD=9b _altE=bd _altH=df _altL=f1"
    "IX=1122 IY=3344 SP=5566 PC=7788 MEMPTR=99aa I=bb R=cc IFF1=1 IFF2=0 EI_DELAY=1 IM=2 HALTED=0 Q=01"
    "sequence=6 tick=42 kind=cpu_bus(4) address=4000 pc=7788"
    "payload=000000000000beef value=a5 auxiliary=06 cycle=02 t_states=3"
    "bus_timing source=ula cycle=memory_write contention_delay_tstates=6"
    "sequence=7 tick=42 kind=interrupt(5)"
    "auxiliary=03"
    "recovered=7")
foreach(expected IN LISTS expected_output)
    string(FIND "${dump_output}${dump_error}" "${expected}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "trace dump is missing '${expected}': ${dump_output}${dump_error}")
    endif()
endforeach()

string(FIND "${dump_output}" "sequence=6 tick=42" bus_position)
string(FIND "${dump_output}" "sequence=7 tick=42" interrupt_position)
if(bus_position GREATER_EQUAL interrupt_position)
    message(FATAL_ERROR "same-tick trace events were not emitted in sequence order")
endif()

message("PASS trace dump fields, same-tick ordering, and reconstructed CPU state")
