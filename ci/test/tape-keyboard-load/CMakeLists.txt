# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is licensed under the GNU General Public License, version 2 or later.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZTapeKeyboardLoad LANGUAGES C)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(WZSN_ENABLE_WARNINGS ON CACHE BOOL "Enable project warnings" FORCE)
set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Disable native UI host" FORCE)
add_subdirectory("${REPO_ROOT}/src/cmake" "${CMAKE_BINARY_DIR}/product" EXCLUDE_FROM_ALL)

add_executable(wz_tape_keyboard_load tape-keyboard-load.c)
target_link_libraries(wz_tape_keyboard_load PRIVATE wz_core)
