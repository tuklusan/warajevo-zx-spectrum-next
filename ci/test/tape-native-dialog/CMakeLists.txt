# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZTapeNativeDialogContract LANGUAGES C)

set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Build the opt-in Sokol host shell." FORCE)
get_filename_component(WZSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_subdirectory("${WZSN_ROOT}/src/cmake" wzsn-product)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0>=3.20)
add_executable(wz_tape_native_dialog_contract
    tape-native-dialog-contract.c
    "${WZSN_ROOT}/src/app/wz_file_dialog_linux.c")
set_target_properties(wz_tape_native_dialog_contract PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
target_include_directories(wz_tape_native_dialog_contract PRIVATE
    "${WZSN_ROOT}/src")
target_link_libraries(wz_tape_native_dialog_contract PRIVATE
    wz_core wz_warnings PkgConfig::GTK3)
