# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZApplicationCommandBoundary LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        zlib
        URL https://github.com/madler/zlib/archive/refs/tags/v1.3.1.tar.gz
        URL_HASH SHA256=17e88863f3600672ab49182f217281b6fc4d3c762bde361935e436a95214d05c
    )
    FetchContent_MakeAvailable(zlib)
    if(NOT TARGET ZLIB::ZLIB)
        if(TARGET zlibstatic)
            add_library(ZLIB::ZLIB ALIAS zlibstatic)
        elseif(TARGET zlib)
            add_library(ZLIB::ZLIB ALIAS zlib)
        endif()
    endif()
endif()
if(NOT TARGET ZLIB::ZLIB)
    message(FATAL_ERROR "ZLIB::ZLIB target is unavailable")
endif()

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(SOURCE_ROOT "${REPO_ROOT}/src")

add_executable(wz_application_command_boundary
    "${REPO_ROOT}/tests/application-command-boundary.c"
    "${SOURCE_ROOT}/app/wz_command_registry.c"
    "${SOURCE_ROOT}/app/wz_host_thread.c"
    "${SOURCE_ROOT}/app/wz_ui_layout.c"
    "${SOURCE_ROOT}/app/wz_telnet_keyboard_command.c"
    "${SOURCE_ROOT}/app/wz_telnet_keymap.c"
    "${SOURCE_ROOT}/app/wz_host_output.c"
    "${SOURCE_ROOT}/app/wz_screenshot_service.c"
    "${SOURCE_ROOT}/app/wz_host_audio_policy.c"
    "${SOURCE_ROOT}/app/wz_speed_policy.c"
    "${SOURCE_ROOT}/core/wz_machine_profile.c"
    "${SOURCE_ROOT}/core/wz_keyboard_matrix.c"
    "${SOURCE_ROOT}/core/wz_kempston.c"
    "${SOURCE_ROOT}/core/audio/wz_beeper.c"
    "${SOURCE_ROOT}/core/audio/wz_ay.c"
    "${SOURCE_ROOT}/core/audio/wz_audio_mixer.c"
    "${SOURCE_ROOT}/core/audio/wz_audio_evidence.c"
    "${SOURCE_ROOT}/core/wz_bus.c"
    "${SOURCE_ROOT}/core/wz_debugger.c"
    "${SOURCE_ROOT}/core/wz_machine.c"
    "${SOURCE_ROOT}/core/wz_microdrive.c"
    "${SOURCE_ROOT}/core/wz_printer.c"
    "${SOURCE_ROOT}/core/wz_zxnet.c"
    "${SOURCE_ROOT}/core/wz_raster.c"
    "${SOURCE_ROOT}/core/wz_presentation_snapshot.c"
    "${SOURCE_ROOT}/core/wz_raster_diagnostic.c"
    "${SOURCE_ROOT}/core/wz_raster_evidence.c"
    "${SOURCE_ROOT}/core/wz_runner.c"
    "${SOURCE_ROOT}/core/wz_scheduler.c"
    "${SOURCE_ROOT}/core/wz_tape.c"
    "${SOURCE_ROOT}/core/wz_state.c"
    "${SOURCE_ROOT}/core/wz_trace.c"
    "${SOURCE_ROOT}/core/wz_z80.c")

target_include_directories(wz_application_command_boundary PRIVATE
    "${SOURCE_ROOT}")
target_link_libraries(wz_application_command_boundary PRIVATE
    Threads::Threads ZLIB::ZLIB)
