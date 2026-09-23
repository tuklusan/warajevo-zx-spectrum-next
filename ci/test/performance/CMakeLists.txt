# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
# patent, trademark, and governing-law provisions.

cmake_minimum_required(VERSION 3.25)

project(WZSNPerformanceBaseline LANGUAGES C)

set(WZSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
add_subdirectory("${WZSN_ROOT}/src/cmake" "${CMAKE_CURRENT_BINARY_DIR}/project")

add_executable(
    wz_performance_baseline
    "${CMAKE_CURRENT_LIST_DIR}/baseline.c"
    "${WZSN_ROOT}/src/app/wz_command_registry.c"
    "${WZSN_ROOT}/src/app/wz_control_port.c"
    "${WZSN_ROOT}/src/app/wz_host_audio_policy.c"
    "${WZSN_ROOT}/src/app/wz_host_socket.c"
    "${WZSN_ROOT}/src/app/wz_speed_policy.c"
    "${WZSN_ROOT}/src/app/wz_ui_layout.c")
target_include_directories(wz_performance_baseline PRIVATE "${WZSN_ROOT}/src")
target_link_libraries(wz_performance_baseline PRIVATE wz_core wz_warnings)
set_target_properties(
    wz_performance_baseline
    PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
target_compile_definitions(
    wz_performance_baseline
    PRIVATE
    WZ_PERF_COMPILER_ID="${CMAKE_C_COMPILER_ID}"
    WZ_PERF_COMPILER_VERSION="${CMAKE_C_COMPILER_VERSION}")
if(WIN32)
    target_link_libraries(wz_performance_baseline PRIVATE ws2_32)
endif()
