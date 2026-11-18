# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZNativeTapDecompressionContract LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

add_subdirectory("${REPO_ROOT}/src/cmake" "${CMAKE_BINARY_DIR}/project")

add_executable(
    wz_native_tap_decompression_contract
    "${REPO_ROOT}/ci/test/native-tap-decompression-contract.c")
target_include_directories(wz_native_tap_decompression_contract PRIVATE
    "${REPO_ROOT}/src")
target_link_libraries(wz_native_tap_decompression_contract PRIVATE wz_core wz_warnings)

enable_testing()
add_test(NAME wz_native_tap_decompression_contract
         COMMAND wz_native_tap_decompression_contract)
