# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZHostDisplaySettingsContract LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(SOURCE_ROOT "${REPO_ROOT}/src")
add_executable(host_display_settings_contract
    "${CMAKE_CURRENT_SOURCE_DIR}/contract.c"
    "${SOURCE_ROOT}/app/wz_host_config.c"
    "${SOURCE_ROOT}/app/wz_host_scaling.c"
    "${SOURCE_ROOT}/app/wz_host_presentation.c")
target_include_directories(host_display_settings_contract PRIVATE
    "${SOURCE_ROOT}")
if(MSVC)
    target_compile_options(host_display_settings_contract PRIVATE /W4 /WX)
else()
    target_compile_options(host_display_settings_contract PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
enable_testing()
add_test(NAME host_display_settings_contract
    COMMAND host_display_settings_contract)
