# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZSnapshotInspectorContract LANGUAGES C)

set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Build the opt-in Sokol host shell." FORCE)
get_filename_component(WZSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_subdirectory("${WZSN_ROOT}/src/cmake" wzsn-product)

add_executable(wz_snapshot_inspector_contract
    "${WZSN_ROOT}/ci/test/snapshot-inspector/snapshot-inspector-contract.c"
    "${WZSN_ROOT}/src/app/wz_snapshot_inspector.c")
set_target_properties(wz_snapshot_inspector_contract PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
target_include_directories(wz_snapshot_inspector_contract PRIVATE
    "${WZSN_ROOT}/src")
target_link_libraries(wz_snapshot_inspector_contract PRIVATE
    wz_core wz_warnings)

enable_testing()
add_test(NAME wz_snapshot_inspector_contract
         COMMAND wz_snapshot_inspector_contract)
