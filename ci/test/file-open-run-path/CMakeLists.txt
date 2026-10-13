# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZFileOpenRunPathContract LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
add_executable(wz_file_open_run_path_contract
    file-open-run-path-contract.c
    "${REPO_ROOT}/src/app/wz_file_open_run.c")
target_include_directories(wz_file_open_run_path_contract PRIVATE
    "${REPO_ROOT}/src")
if(MSVC)
    target_compile_options(wz_file_open_run_path_contract PRIVATE /W4 /WX)
else()
    target_compile_options(wz_file_open_run_path_contract PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
endif()
enable_testing()
add_test(NAME wz_file_open_run_path_contract
    COMMAND wz_file_open_run_path_contract)
