# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZTapeNativeDialogDesktopContract LANGUAGES C)

set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Build the opt-in Sokol host shell." FORCE)
get_filename_component(WZSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_subdirectory("${WZSN_ROOT}/src/cmake" wzsn-product)

if(WIN32)
    include(FetchContent)
    FetchContent_Declare(sokol
        GIT_REPOSITORY https://github.com/floooh/sokol.git
        GIT_TAG 1847290135f95e57e6d220b0a41208306aafc0dd
        GIT_SHALLOW FALSE)
    FetchContent_MakeAvailable(sokol)
    add_executable(wz_tape_native_dialog_desktop_contract
        dialog-contract.c windows-shim.c
        "${WZSN_ROOT}/src/app/wz_file_dialog_win32.c")
    target_include_directories(wz_tape_native_dialog_desktop_contract PRIVATE
        "${WZSN_ROOT}/src" "${sokol_SOURCE_DIR}")
    target_link_libraries(wz_tape_native_dialog_desktop_contract PRIVATE
        wz_core wz_warnings ole32 shell32 uuid)
elseif(APPLE)
    enable_language(OBJC)
    add_executable(wz_tape_native_dialog_desktop_contract
        dialog-contract.c macos-shim.m
        "${WZSN_ROOT}/src/app/wz_file_dialog_macos.m")
    target_include_directories(wz_tape_native_dialog_desktop_contract PRIVATE
        "${WZSN_ROOT}/src")
    target_link_libraries(wz_tape_native_dialog_desktop_contract PRIVATE
        wz_core wz_warnings "-framework AppKit")
else()
    message(FATAL_ERROR "This contract targets Windows and macOS runners.")
endif()

set_target_properties(wz_tape_native_dialog_desktop_contract PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
