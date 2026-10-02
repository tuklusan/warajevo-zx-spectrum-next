# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZMicrodriveUIContract LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(Threads REQUIRED)
get_filename_component(WZSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(SOURCE_ROOT "${WZSN_ROOT}/src")
set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Build the opt-in Sokol host shell." FORCE)
add_subdirectory("${SOURCE_ROOT}/cmake" wzsn-product)

add_executable(wz_microdrive_ui_contract
    microdrive-ui-contract.c
    "${SOURCE_ROOT}/app/wz_command_registry.c"
    "${SOURCE_ROOT}/app/wz_host_thread.c"
    "${SOURCE_ROOT}/app/wz_host_audio_policy.c"
    "${SOURCE_ROOT}/app/wz_speed_policy.c"
    "${SOURCE_ROOT}/app/wz_ui_layout.c"
    "${SOURCE_ROOT}/app/wz_microdrive_eject_workflow.c")
target_include_directories(wz_microdrive_ui_contract PRIVATE
    "${SOURCE_ROOT}")
target_link_libraries(wz_microdrive_ui_contract PRIVATE
    wz_core wz_warnings Threads::Threads)

enable_testing()
add_test(NAME wz_microdrive_ui_contract COMMAND wz_microdrive_ui_contract)
