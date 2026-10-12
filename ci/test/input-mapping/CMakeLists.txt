# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZInputMappingContract LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(SOURCE_ROOT "${REPO_ROOT}/src")
add_executable(input_mapping_contract
    "${CMAKE_CURRENT_SOURCE_DIR}/contract.c"
    "${SOURCE_ROOT}/app/wz_keyboard_mapping.c"
    "${SOURCE_ROOT}/app/wz_kempston_mapping.c"
    "${SOURCE_ROOT}/core/wz_kempston.c")
target_include_directories(input_mapping_contract PRIVATE "${SOURCE_ROOT}")
if(MSVC)
    target_compile_options(input_mapping_contract PRIVATE /W4 /WX)
else()
    target_compile_options(input_mapping_contract PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
enable_testing()
add_test(NAME input_mapping_contract COMMAND input_mapping_contract)
