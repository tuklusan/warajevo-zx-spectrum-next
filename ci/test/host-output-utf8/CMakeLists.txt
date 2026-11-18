# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is licensed under the GNU General Public License, version 2 or later.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZHostOutputUtf8 LANGUAGES C)

get_filename_component(WZSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_executable(wz_host_output_utf8_tests
    host-output-utf8.c
    "${WZSN_ROOT}/src/app/wz_host_output_utf8.c"
    "${WZSN_ROOT}/src/app/wz_host_output.c")
target_include_directories(wz_host_output_utf8_tests PRIVATE "${WZSN_ROOT}/src")
target_compile_features(wz_host_output_utf8_tests PRIVATE c_std_11)
if(MSVC)
    target_compile_options(wz_host_output_utf8_tests PRIVATE /W4 /WX)
else()
    target_compile_options(wz_host_output_utf8_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

enable_testing()
add_test(NAME wz_host_output_utf8_tests COMMAND wz_host_output_utf8_tests)
