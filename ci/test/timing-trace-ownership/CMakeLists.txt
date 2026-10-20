# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZTimingTraceOwnershipContract LANGUAGES C)

set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Build the opt-in Sokol host shell." FORCE)
get_filename_component(WZSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_subdirectory("${WZSN_ROOT}/src/cmake" "${CMAKE_CURRENT_BINARY_DIR}/product")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_executable(wz_trace_owner_fixture trace-owner-fixture.c)
target_include_directories(wz_trace_owner_fixture PRIVATE "${WZSN_ROOT}/src")
target_link_libraries(wz_trace_owner_fixture PRIVATE wz_diagnostics wz_warnings)
set_target_properties(wz_trace_owner_fixture PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)

enable_testing()
add_test(NAME wz_trace_process_ownership
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_LIST_DIR}/verify-ownership.py"
        --fixture "$<TARGET_FILE:wz_trace_owner_fixture>"
        --workdir "${CMAKE_CURRENT_BINARY_DIR}/ownership-output")
