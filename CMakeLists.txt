# Warajevo ZX Spectrum Next
# Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
# New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
# Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
# See LICENSE.txt and NOTICE.md for complete terms and provenance.

cmake_minimum_required(VERSION 3.25)

project(WarajevoZXSpectrumNext VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(WZSN_ENABLE_WARNINGS "Enable the default developer warning set." ON)
option(WZSN_ENABLE_SANITIZERS "Enable sanitizer flags when supported." OFF)

add_library(wz_warnings INTERFACE)

if(WZSN_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(wz_warnings INTERFACE /W4 /permissive-)
    else()
        target_compile_options(
            wz_warnings
            INTERFACE
            -Wall
            -Wextra
            -Wpedantic
        )
    endif()
endif()

if(WZSN_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(wz_warnings INTERFACE -fsanitize=address,undefined)
    target_link_options(wz_warnings INTERFACE -fsanitize=address,undefined)
endif()

add_library(
    wz_core
    STATIC
    src/core/wz_machine_profile.c
    src/core/wz_machine.c
    src/core/wz_scheduler.c
    src/core/wz_state.c
)

target_include_directories(wz_core PUBLIC src)
target_link_libraries(wz_core PUBLIC wz_warnings)

add_executable(
    wz_headless
    src/app/wz_headless_main.c
)

target_link_libraries(wz_headless PRIVATE wz_core)

add_executable(
    wz_tests
    tests/wz_tests_placeholder.c
)

target_link_libraries(wz_tests PRIVATE wz_core)

enable_testing()
add_test(NAME wz_tests_placeholder COMMAND wz_tests)

add_custom_target(wz_gui_stub)
