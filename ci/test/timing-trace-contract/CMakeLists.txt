# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZTimingTraceContract LANGUAGES C)

set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Build the opt-in Sokol host shell." FORCE)
get_filename_component(WZSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_subdirectory("${WZSN_ROOT}/src/cmake" "${CMAKE_CURRENT_BINARY_DIR}/product")

add_executable(wz_timing_trace_contract
    "${CMAKE_CURRENT_LIST_DIR}/timing-trace-contract.c"
    "${WZSN_ROOT}/src/diagnostics/wz_trace_file.c")
target_include_directories(wz_timing_trace_contract PRIVATE
    "${WZSN_ROOT}/src")
target_compile_definitions(wz_timing_trace_contract PRIVATE
    WZ_TRACE_FILE_TEST_SLOT_COUNT=3)
set_target_properties(wz_timing_trace_contract PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
target_link_libraries(wz_timing_trace_contract PRIVATE wz_core wz_warnings)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/output")
enable_testing()
add_test(NAME wz_timing_trace_contract
    COMMAND wz_timing_trace_contract "${CMAKE_CURRENT_BINARY_DIR}/output")
