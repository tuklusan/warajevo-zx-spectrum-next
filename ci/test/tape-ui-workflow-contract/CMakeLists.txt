# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZTapeUiWorkflowContract LANGUAGES C)

set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Build the opt-in Sokol host shell." FORCE)
get_filename_component(WZSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_subdirectory("${WZSN_ROOT}/src/cmake" wzsn-product)

add_executable(wz_tape_ui_workflow_contract
    tape-ui-workflow-contract.c
    "${WZSN_ROOT}/src/app/wz_tape_insert_action.c"
    "${WZSN_ROOT}/src/app/wz_tape_media_commands.c"
    "${WZSN_ROOT}/src/app/wz_command_registry.c"
    "${WZSN_ROOT}/src/app/wz_host_thread.c")
set_target_properties(wz_tape_ui_workflow_contract PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
target_include_directories(wz_tape_ui_workflow_contract PRIVATE
    "${WZSN_ROOT}/src")
target_link_libraries(wz_tape_ui_workflow_contract PRIVATE wz_core wz_warnings)

enable_testing()
add_test(NAME wz_tape_ui_workflow_contract COMMAND wz_tape_ui_workflow_contract)
