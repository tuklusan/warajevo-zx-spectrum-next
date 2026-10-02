# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZCompatibilityToolsContract LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
find_package(Threads REQUIRED)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(SOURCE_ROOT "${REPO_ROOT}/src")
add_executable(wz_compatibility_tools_contract
    "${CMAKE_CURRENT_SOURCE_DIR}/contract.c"
    "${SOURCE_ROOT}/app/wz_compatibility_tools.c"
    "${SOURCE_ROOT}/app/wz_file_open_run.c"
    "${SOURCE_ROOT}/app/wz_command_registry.c"
    "${SOURCE_ROOT}/app/wz_host_thread.c")
target_include_directories(wz_compatibility_tools_contract PRIVATE
    "${SOURCE_ROOT}")
target_link_libraries(wz_compatibility_tools_contract PRIVATE Threads::Threads)
if(MSVC)
    target_compile_options(wz_compatibility_tools_contract PRIVATE /W4 /WX)
else()
    target_compile_options(wz_compatibility_tools_contract PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
enable_testing()
add_test(NAME wz_compatibility_tools_contract
    COMMAND wz_compatibility_tools_contract)
