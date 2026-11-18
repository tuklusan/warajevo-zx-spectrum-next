# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is licensed under the GNU General Public License, version 2 or later.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZFuseZ80Regression LANGUAGES C)

set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Build the opt-in Sokol host shell." FORCE)
set(WZSN_FUSE_Z80_TEST_INPUT
    "${CMAKE_CURRENT_LIST_DIR}/../../../dist/fuse-upstream/z80/tests/tests.in"
    CACHE FILEPATH "Pinned Fuse Z80 tests.in path." FORCE)
set(WZSN_FUSE_Z80_TEST_EXPECTED
    "${CMAKE_CURRENT_LIST_DIR}/../../../dist/fuse-upstream/z80/tests/tests.expected"
    CACHE FILEPATH "Pinned Fuse Z80 tests.expected path." FORCE)
get_filename_component(WZSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_subdirectory("${WZSN_ROOT}/src/cmake" "${CMAKE_CURRENT_BINARY_DIR}/product")
