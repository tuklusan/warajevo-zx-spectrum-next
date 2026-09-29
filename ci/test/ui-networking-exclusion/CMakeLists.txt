# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZUiNetworkingExclusionContract LANGUAGES C)

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(WZSN_ENABLE_WARNINGS ON CACHE BOOL "Enable project warnings" FORCE)
set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Disable native UI host" FORCE)
add_subdirectory("${REPO_ROOT}/src/cmake" "${CMAKE_BINARY_DIR}/product" EXCLUDE_FROM_ALL)

find_package(Threads REQUIRED)

add_executable(wz_ui_networking_exclusion_contract
    ui-networking-exclusion-contract.c
    "${REPO_ROOT}/src/app/wz_command_registry.c"
    "${REPO_ROOT}/src/app/wz_host_audio_policy.c"
    "${REPO_ROOT}/src/app/wz_host_thread.c"
    "${REPO_ROOT}/src/app/wz_networking_commands.c"
    "${REPO_ROOT}/src/app/wz_speed_policy.c"
    "${REPO_ROOT}/src/app/wz_ui_layout.c")
set_target_properties(wz_ui_networking_exclusion_contract PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
target_include_directories(wz_ui_networking_exclusion_contract PRIVATE
    "${REPO_ROOT}/src")
target_link_libraries(wz_ui_networking_exclusion_contract PRIVATE
    wz_core wz_warnings Threads::Threads)
