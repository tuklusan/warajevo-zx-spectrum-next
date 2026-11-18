# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is licensed under the GNU General Public License, version 2 or later.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZNativeTapContract LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

add_subdirectory("${REPO_ROOT}/src/cmake" "${CMAKE_BINARY_DIR}/project")

add_executable(
    wz_native_tap_contract
    "${REPO_ROOT}/ci/test/native-tap-contract.c")
target_include_directories(wz_native_tap_contract PRIVATE
    "${REPO_ROOT}/src")
target_link_libraries(wz_native_tap_contract PRIVATE wz_core wz_warnings)

enable_testing()
add_test(NAME wz_native_tap_contract COMMAND wz_native_tap_contract)
