# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZKeyboardMenu LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(SOURCE_ROOT "${REPO_ROOT}/src")

add_executable(wz_keyboard_menu_contract
    keyboard-menu.c
    "${SOURCE_ROOT}/app/wz_ui_keyboard_menu.c"
    "${SOURCE_ROOT}/app/wz_input_focus.c"
    "${SOURCE_ROOT}/app/wz_input_arbiter.c")
target_include_directories(wz_keyboard_menu_contract PRIVATE "${SOURCE_ROOT}")
