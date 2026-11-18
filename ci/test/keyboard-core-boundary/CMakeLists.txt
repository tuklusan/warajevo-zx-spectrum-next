# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is licensed under the GNU General Public License, version 2 or later.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZKeyboardCoreBoundary LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(WZSN_ENABLE_WARNINGS ON CACHE BOOL "Enable project warnings" FORCE)
set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Disable native UI host" FORCE)
add_subdirectory("${REPO_ROOT}/src/cmake" "${CMAKE_BINARY_DIR}/product" EXCLUDE_FROM_ALL)

add_executable(wz_keyboard_core_boundary
    keyboard-core-boundary.c
    "${REPO_ROOT}/src/app/wz_input_arbiter.c"
    "${REPO_ROOT}/src/app/wz_telnet_input.c"
    "${REPO_ROOT}/src/app/wz_input_projection.c")
target_include_directories(wz_keyboard_core_boundary PRIVATE
    "${REPO_ROOT}/src")
target_link_libraries(wz_keyboard_core_boundary PRIVATE wz_core wz_warnings)
