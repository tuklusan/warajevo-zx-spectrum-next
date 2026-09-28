# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZTelnetKeyPressRegression LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(SOURCE_ROOT "${REPO_ROOT}/src")

add_executable(wz_telnet_key_press_regression
    telnet-key-press-regression.c
    "${SOURCE_ROOT}/app/wz_input_arbiter.c"
    "${SOURCE_ROOT}/app/wz_telnet_key_press.c")
target_include_directories(wz_telnet_key_press_regression PRIVATE
    "${SOURCE_ROOT}")
