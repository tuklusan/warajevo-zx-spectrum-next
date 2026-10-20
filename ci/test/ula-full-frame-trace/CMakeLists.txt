# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZULAFullFrameTraceContract LANGUAGES C)

set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Build the opt-in Sokol host shell." FORCE)
get_filename_component(WZSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_subdirectory("${WZSN_ROOT}/src/cmake" "${CMAKE_CURRENT_BINARY_DIR}/product")

add_executable(wz_ula_full_frame_trace_fixture ula-full-frame-trace-fixture.c)
target_include_directories(wz_ula_full_frame_trace_fixture PRIVATE "${WZSN_ROOT}/src")
target_link_libraries(wz_ula_full_frame_trace_fixture PRIVATE wz_core wz_diagnostics wz_warnings)
set_target_properties(wz_ula_full_frame_trace_fixture PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output")
enable_testing()
add_test(NAME wz_ula_full_frame_trace_contract
    COMMAND "${CMAKE_COMMAND}"
        "-DTRACE_FIXTURE=$<TARGET_FILE:wz_ula_full_frame_trace_fixture>"
        "-DTRACE_DUMPER=$<TARGET_FILE:wz_trace_dump>"
        "-DTRACE_PATH=${CMAKE_CURRENT_BINARY_DIR}/output/ula-full-frame.wztrace"
        -P "${CMAKE_CURRENT_LIST_DIR}/verify-full-frame.cmake")
