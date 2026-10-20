# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZTimingTraceDiagnosticsContract LANGUAGES C)

set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Build the opt-in Sokol host shell." FORCE)
get_filename_component(WZSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_subdirectory("${WZSN_ROOT}/src/cmake" "${CMAKE_CURRENT_BINARY_DIR}/product")

add_executable(wz_trace_diagnostics_fixture trace-diagnostics-fixture.c)
target_include_directories(wz_trace_diagnostics_fixture PRIVATE "${WZSN_ROOT}/src")
target_link_libraries(wz_trace_diagnostics_fixture PRIVATE wz_diagnostics wz_warnings)
set_target_properties(wz_trace_diagnostics_fixture PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output")
enable_testing()
add_test(NAME wz_trace_diagnostics_contract
    COMMAND "${CMAKE_COMMAND}"
        "-DTRACE_FIXTURE=$<TARGET_FILE:wz_trace_diagnostics_fixture>"
        "-DTRACE_DUMPER=$<TARGET_FILE:wz_trace_dump>"
        "-DTRACE_PATH=${CMAKE_CURRENT_BINARY_DIR}/output/bus-timing.wztrace"
        -P "${CMAKE_CURRENT_LIST_DIR}/verify-diagnostics.cmake")
