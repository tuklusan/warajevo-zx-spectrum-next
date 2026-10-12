# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZHostAudioSettingsContract LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(SOURCE_ROOT "${REPO_ROOT}/src")
add_executable(host_audio_settings_contract
    "${CMAKE_CURRENT_SOURCE_DIR}/contract.c"
    "${SOURCE_ROOT}/app/wz_host_audio_policy.c"
    "${SOURCE_ROOT}/app/wz_speed_policy.c"
    "${SOURCE_ROOT}/app/wz_host_config.c")
target_include_directories(host_audio_settings_contract PRIVATE
    "${SOURCE_ROOT}")
if(MSVC)
    target_compile_options(host_audio_settings_contract PRIVATE /W4 /WX)
else()
    target_compile_options(host_audio_settings_contract PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
enable_testing()
add_test(NAME host_audio_settings_contract
    COMMAND host_audio_settings_contract)
