# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZRomSettingsContract LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(SOURCE_ROOT "${REPO_ROOT}/src")

add_subdirectory("${SOURCE_ROOT}/cmake" "${CMAKE_CURRENT_BINARY_DIR}/project")

add_executable(rom_settings_contract
    "${CMAKE_CURRENT_SOURCE_DIR}/contract.c"
    "${SOURCE_ROOT}/app/wz_rom_settings.c")
target_include_directories(rom_settings_contract PRIVATE "${SOURCE_ROOT}")
target_link_libraries(rom_settings_contract PRIVATE wz_core)

if(MSVC)
    target_compile_options(rom_settings_contract PRIVATE /W4 /WX)
else()
    target_compile_options(rom_settings_contract PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()

enable_testing()
add_test(NAME rom_settings_contract COMMAND rom_settings_contract)
