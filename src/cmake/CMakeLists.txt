# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# Proprietary rights reserved except as expressly licensed herein.
#
# Warajevo ZX Spectrum Next build definition.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
#
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms, warranty disclaimer, termination,
# patent, trademark, and governing-law provisions.

cmake_minimum_required(VERSION 3.25)

project(WarajevoZXSpectrumNext VERSION 0.1.0 LANGUAGES C)

set(WZSN_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(WZSN_SOURCE_ROOT "${WZSN_REPO_ROOT}/src")
set(WZSN_TEST_ROOT "${WZSN_REPO_ROOT}/tests")

find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
    if(EXISTS "${WZSN_REPO_ROOT}/third_party/zlib/CMakeLists.txt")
        set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "Disable vendored zlib example tests" FORCE)
        add_subdirectory("${WZSN_REPO_ROOT}/third_party/zlib" "${CMAKE_BINARY_DIR}/third_party/zlib" EXCLUDE_FROM_ALL)
    else()
        include(FetchContent)
        FetchContent_Declare(
            zlib
            URL https://github.com/madler/zlib/archive/refs/tags/v1.3.1.tar.gz
            URL_HASH SHA256=17e88863f3600672ab49182f217281b6fc4d3c762bde361935e436a95214d05c
        )
        FetchContent_MakeAvailable(zlib)
    endif()
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

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(WZSN_ENABLE_WARNINGS "Enable the default developer warning set." ON)
option(WZSN_ENABLE_SANITIZERS "Enable sanitizer flags when supported." OFF)
option(WZSN_ENABLE_REVIEW_GATE_TESTS "Require Python and run reviewer-gate tests." OFF)
option(WZSN_BUILD_SOKOL_HOST "Build the opt-in Sokol host shell." OFF)
set(WZSN_FUSE_Z80_TEST_INPUT "" CACHE FILEPATH
    "Pinned Fuse Z80 tests.in path for the optional conformance test.")
set(WZSN_FUSE_Z80_TEST_EXPECTED "" CACHE FILEPATH
    "Pinned Fuse Z80 tests.expected path for the optional conformance test.")

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
    ${WZSN_SOURCE_ROOT}/core/wz_machine_profile.c
    ${WZSN_SOURCE_ROOT}/core/wz_keyboard_matrix.c
    ${WZSN_SOURCE_ROOT}/core/wz_kempston.c
    ${WZSN_SOURCE_ROOT}/core/audio/wz_beeper.c
    ${WZSN_SOURCE_ROOT}/core/audio/wz_ay.c
    ${WZSN_SOURCE_ROOT}/core/audio/wz_audio_mixer.c
    ${WZSN_SOURCE_ROOT}/core/audio/wz_audio_evidence.c
    ${WZSN_SOURCE_ROOT}/core/wz_bus.c
    ${WZSN_SOURCE_ROOT}/core/wz_debugger.c
    ${WZSN_SOURCE_ROOT}/core/wz_machine.c
    ${WZSN_SOURCE_ROOT}/core/wz_microdrive.c
    ${WZSN_SOURCE_ROOT}/core/wz_printer.c
    ${WZSN_SOURCE_ROOT}/core/wz_zxnet.c
    ${WZSN_SOURCE_ROOT}/core/wz_raster.c
    ${WZSN_SOURCE_ROOT}/core/wz_presentation_snapshot.c
    ${WZSN_SOURCE_ROOT}/core/wz_raster_diagnostic.c
    ${WZSN_SOURCE_ROOT}/core/wz_raster_evidence.c
    ${WZSN_SOURCE_ROOT}/core/wz_runner.c
    ${WZSN_SOURCE_ROOT}/core/wz_scheduler.c
    ${WZSN_SOURCE_ROOT}/core/wz_tape.c
    ${WZSN_SOURCE_ROOT}/core/wz_state.c
    ${WZSN_SOURCE_ROOT}/core/wz_trace.c
    ${WZSN_SOURCE_ROOT}/core/wz_z80.c
)

target_include_directories(wz_core PUBLIC "${WZSN_SOURCE_ROOT}")
target_link_libraries(wz_core PUBLIC wz_warnings ZLIB::ZLIB)

add_library(wz_diagnostics STATIC
    ${WZSN_SOURCE_ROOT}/diagnostics/wz_trace_file.c
    ${WZSN_SOURCE_ROOT}/diagnostics/wz_syslog_udp.c
    ${WZSN_SOURCE_ROOT}/diagnostics/wz_diagnostics_router.c)
target_include_directories(wz_diagnostics PUBLIC "${WZSN_SOURCE_ROOT}")
target_link_libraries(wz_diagnostics PUBLIC wz_core wz_warnings)
if(WIN32)
    target_link_libraries(wz_diagnostics PUBLIC ws2_32)
endif()

add_executable(
    wz_headless
    ${WZSN_SOURCE_ROOT}/app/wz_headless_main.c
    ${WZSN_SOURCE_ROOT}/app/wz_diagnostic_block.c
)

target_link_libraries(wz_headless PRIVATE wz_core)

add_executable(wz_trace_dump ${WZSN_SOURCE_ROOT}/diagnostics/wz_trace_dump_main.c)
target_link_libraries(wz_trace_dump PRIVATE wz_diagnostics)

if(WZSN_FUSE_Z80_TEST_INPUT AND WZSN_FUSE_Z80_TEST_EXPECTED)
    add_executable(wz_tests ${WZSN_TEST_ROOT}/fuse-z80-conformance.c)
    target_link_libraries(wz_tests PRIVATE wz_core wz_warnings)
elseif(EXISTS "${WZSN_TEST_ROOT}/wz_tests_placeholder.c")
add_executable(
    wz_tests
    ${WZSN_TEST_ROOT}/wz_tests_placeholder.c
    ${WZSN_SOURCE_ROOT}/app/wz_input_timing.c
    ${WZSN_SOURCE_ROOT}/app/wz_input_replay.c
    ${WZSN_SOURCE_ROOT}/app/wz_telnet_keyboard_command.c
    ${WZSN_SOURCE_ROOT}/app/wz_command_registry.c ${WZSN_SOURCE_ROOT}/app/wz_host_thread.c
    ${WZSN_SOURCE_ROOT}/app/wz_screenshot_service.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_output.c
    ${WZSN_SOURCE_ROOT}/app/wz_input_arbiter.c
    ${WZSN_SOURCE_ROOT}/app/wz_telnet_key_press.c
    ${WZSN_SOURCE_ROOT}/app/wz_input_focus.c
    ${WZSN_SOURCE_ROOT}/app/wz_kempston_mapping.c
    ${WZSN_SOURCE_ROOT}/app/wz_speed_policy.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_pacing.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_config.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_media_ownership.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_audio_push.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_audio_policy.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_socket.c
    ${WZSN_SOURCE_ROOT}/app/wz_control_port.c
    ${WZSN_SOURCE_ROOT}/app/wz_telnet_client.c
    ${WZSN_SOURCE_ROOT}/app/wz_telnet_negotiation.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_command_queue.c
    ${WZSN_SOURCE_ROOT}/app/wz_telnet_security.c
    ${WZSN_SOURCE_ROOT}/app/wz_telnet_input.c
    ${WZSN_SOURCE_ROOT}/app/wz_telnet_keymap.c
    ${WZSN_SOURCE_ROOT}/app/wz_zxnet_transport.c
)

target_link_libraries(wz_tests PRIVATE wz_core wz_diagnostics)
if(WIN32)
    target_link_libraries(wz_tests PRIVATE ws2_32)
endif()
endif()

if(EXISTS "${WZSN_TEST_ROOT}/wz_tests_placeholder.c")
add_executable(wz_fuse_ed_runner ${WZSN_TEST_ROOT}/wz_fuse_ed_runner.c)
target_link_libraries(wz_fuse_ed_runner PRIVATE wz_core)

add_executable(wz_zxnet_loopback ${WZSN_TEST_ROOT}/wz_zxnet_loopback.c)
target_link_libraries(wz_zxnet_loopback PRIVATE wz_core)

add_executable(wz_syslog_udp_tests ${WZSN_TEST_ROOT}/wz_syslog_udp_tests.c)
target_link_libraries(wz_syslog_udp_tests PRIVATE wz_diagnostics)

add_executable(wz_diagnostics_router_tests ${WZSN_TEST_ROOT}/wz_diagnostics_router_tests.c)
target_link_libraries(wz_diagnostics_router_tests PRIVATE wz_diagnostics)

add_executable(wz_printer_tests ${WZSN_TEST_ROOT}/wz_printer_tests.c)
target_link_libraries(wz_printer_tests PRIVATE wz_core)

add_executable(wz_debugger_memory_tools_tests ${WZSN_TEST_ROOT}/wz_debugger_memory_tools_tests.c)
target_link_libraries(wz_debugger_memory_tools_tests PRIVATE wz_core)

add_executable(wz_compatibility_tools_tests
    ${WZSN_TEST_ROOT}/wz_compatibility_tools_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_compatibility_tools.c)
target_link_libraries(wz_compatibility_tools_tests PRIVATE wz_warnings)
target_include_directories(wz_compatibility_tools_tests PRIVATE "${WZSN_SOURCE_ROOT}")

add_executable(wz_debugger_page_controls_tests ${WZSN_TEST_ROOT}/wz_debugger_page_controls_tests.c)
target_link_libraries(wz_debugger_page_controls_tests PRIVATE wz_core)

add_executable(wz_debugger_disassembly_tests ${WZSN_TEST_ROOT}/wz_debugger_disassembly_tests.c)
target_link_libraries(wz_debugger_disassembly_tests PRIVATE wz_core)

add_executable(wz_debugger_jump_undo_tests ${WZSN_TEST_ROOT}/wz_debugger_jump_undo_tests.c)
target_link_libraries(wz_debugger_jump_undo_tests PRIVATE wz_core)

add_executable(
    wz_debugger_window_tests
    ${WZSN_TEST_ROOT}/wz_debugger_window_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_debugger_window.c
    ${WZSN_SOURCE_ROOT}/app/wz_command_registry.c ${WZSN_SOURCE_ROOT}/app/wz_host_thread.c)
target_link_libraries(wz_debugger_window_tests PRIVATE wz_core)

add_executable(
    wz_diagnostics_window_tests
    ${WZSN_TEST_ROOT}/wz_diagnostics_window_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_diagnostics_window.c
    ${WZSN_SOURCE_ROOT}/app/wz_command_registry.c ${WZSN_SOURCE_ROOT}/app/wz_host_thread.c)
target_link_libraries(wz_diagnostics_window_tests PRIVATE wz_core wz_diagnostics)

add_executable(wz_machine_reset_tests ${WZSN_TEST_ROOT}/wz_machine_reset_tests.c)
target_link_libraries(wz_machine_reset_tests PRIVATE wz_core)

add_executable(
    wz_host_media_ownership_tests
    ${WZSN_TEST_ROOT}/wz_host_media_ownership_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_media_ownership.c)
target_link_libraries(wz_host_media_ownership_tests PRIVATE wz_warnings)
target_include_directories(wz_host_media_ownership_tests PRIVATE "${WZSN_SOURCE_ROOT}")

add_executable(wz_printer_export_tests
    ${WZSN_TEST_ROOT}/wz_printer_export_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_printer_export.c)
target_link_libraries(wz_printer_export_tests PRIVATE wz_core)

add_executable(wz_printer_manager_tests
    ${WZSN_TEST_ROOT}/wz_printer_manager_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_printer_manager.c
    ${WZSN_SOURCE_ROOT}/app/wz_printer_export.c
    ${WZSN_SOURCE_ROOT}/app/wz_command_registry.c ${WZSN_SOURCE_ROOT}/app/wz_host_thread.c)
target_link_libraries(wz_printer_manager_tests PRIVATE wz_core)

add_executable(wz_printer_commands_tests
    ${WZSN_TEST_ROOT}/wz_printer_commands_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_printer_commands.c
    ${WZSN_SOURCE_ROOT}/app/wz_command_registry.c ${WZSN_SOURCE_ROOT}/app/wz_host_thread.c)
target_link_libraries(wz_printer_commands_tests PRIVATE wz_core)

add_executable(wz_diagnostic_block_tests
    ${WZSN_TEST_ROOT}/wz_diagnostic_block_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_diagnostic_block.c)
target_link_libraries(wz_diagnostic_block_tests PRIVATE wz_core)

add_executable(
    wz_application_lifecycle_tests
    ${WZSN_TEST_ROOT}/wz_application_lifecycle_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_application_lifecycle.c)
target_link_libraries(wz_application_lifecycle_tests PRIVATE wz_core)

add_executable(
    wz_command_registry_tests
    ${WZSN_TEST_ROOT}/wz_command_registry_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_command_registry.c ${WZSN_SOURCE_ROOT}/app/wz_host_thread.c)
target_link_libraries(wz_command_registry_tests PRIVATE wz_core)

add_executable(
    wz_networking_commands_tests
    ${WZSN_TEST_ROOT}/wz_networking_commands_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_networking_commands.c
    ${WZSN_SOURCE_ROOT}/app/wz_command_registry.c ${WZSN_SOURCE_ROOT}/app/wz_host_thread.c)
target_link_libraries(wz_networking_commands_tests PRIVATE wz_core)

add_executable(
    wz_ui_layout_tests
    ${WZSN_TEST_ROOT}/wz_ui_layout_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_config.c
    ${WZSN_SOURCE_ROOT}/app/wz_ui_layout.c
    ${WZSN_SOURCE_ROOT}/app/wz_command_registry.c ${WZSN_SOURCE_ROOT}/app/wz_host_thread.c
    ${WZSN_SOURCE_ROOT}/app/wz_speed_policy.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_audio_policy.c)
target_link_libraries(wz_ui_layout_tests PRIVATE wz_core)

add_executable(
    wz_tape_manager_tests
    ${WZSN_TEST_ROOT}/wz_tape_manager_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_tape_manager.c)
target_link_libraries(wz_tape_manager_tests PRIVATE wz_core)

add_executable(
    wz_microdrive_manager_tests
    ${WZSN_TEST_ROOT}/wz_microdrive_manager_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_microdrive_manager.c
    ${WZSN_SOURCE_ROOT}/app/wz_command_registry.c ${WZSN_SOURCE_ROOT}/app/wz_host_thread.c)
target_link_libraries(wz_microdrive_manager_tests PRIVATE wz_core)

add_executable(
    wz_ui_window_tests
    ${WZSN_TEST_ROOT}/wz_ui_window_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_ui_window.c
    ${WZSN_SOURCE_ROOT}/app/wz_ui_layout.c
    ${WZSN_SOURCE_ROOT}/app/wz_command_registry.c ${WZSN_SOURCE_ROOT}/app/wz_host_thread.c
    ${WZSN_SOURCE_ROOT}/app/wz_speed_policy.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_audio_policy.c)
target_link_libraries(wz_ui_window_tests PRIVATE wz_core)

add_executable(wz_screen_inspection_tests ${WZSN_TEST_ROOT}/wz_screen_inspection_tests.c)
target_link_libraries(wz_screen_inspection_tests PRIVATE wz_core)

enable_testing()
if(EXISTS "${WZSN_TEST_ROOT}/wz_tests_placeholder.c")
    add_test(NAME wz_tests_placeholder COMMAND wz_tests)
endif()
add_test(NAME wz_zxnet_loopback COMMAND wz_zxnet_loopback)
add_test(NAME wz_syslog_udp_tests COMMAND wz_syslog_udp_tests)
add_test(NAME wz_diagnostics_router_tests COMMAND wz_diagnostics_router_tests)
add_test(NAME wz_printer_tests COMMAND wz_printer_tests)
add_test(NAME wz_debugger_memory_tools_tests COMMAND wz_debugger_memory_tools_tests)
add_test(NAME wz_compatibility_tools_tests COMMAND wz_compatibility_tools_tests)
add_test(NAME wz_debugger_page_controls_tests COMMAND wz_debugger_page_controls_tests)
add_test(NAME wz_debugger_disassembly_tests COMMAND wz_debugger_disassembly_tests)
add_test(NAME wz_debugger_jump_undo_tests COMMAND wz_debugger_jump_undo_tests)
add_test(NAME wz_debugger_window_tests COMMAND wz_debugger_window_tests)
add_test(NAME wz_diagnostics_window_tests COMMAND wz_diagnostics_window_tests)
add_test(NAME wz_machine_reset_tests COMMAND wz_machine_reset_tests)
add_test(NAME wz_host_media_ownership_tests COMMAND wz_host_media_ownership_tests)
add_test(NAME wz_printer_export_tests COMMAND wz_printer_export_tests)
add_test(NAME wz_diagnostic_block_tests COMMAND wz_diagnostic_block_tests)
add_test(NAME wz_application_lifecycle_tests COMMAND wz_application_lifecycle_tests)
add_test(NAME wz_command_registry_tests COMMAND wz_command_registry_tests)
add_test(NAME wz_networking_commands_tests COMMAND wz_networking_commands_tests)
add_test(NAME wz_ui_layout_tests COMMAND wz_ui_layout_tests)
add_test(NAME wz_tape_manager_tests COMMAND wz_tape_manager_tests)
add_test(NAME wz_microdrive_manager_tests COMMAND wz_microdrive_manager_tests)
add_test(NAME wz_printer_manager_tests COMMAND wz_printer_manager_tests)
add_test(NAME wz_printer_commands_tests COMMAND wz_printer_commands_tests)
add_test(NAME wz_ui_window_tests COMMAND wz_ui_window_tests)

add_executable(
    wz_file_open_run_tests
    ${WZSN_TEST_ROOT}/wz_file_open_run_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_file_open_run.c)
target_link_libraries(wz_file_open_run_tests PRIVATE wz_warnings)
target_include_directories(wz_file_open_run_tests PRIVATE "${WZSN_SOURCE_ROOT}")
add_test(NAME wz_file_open_run_tests COMMAND wz_file_open_run_tests)

add_executable(
    wz_recent_files_tests
    ${WZSN_TEST_ROOT}/wz_recent_files_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_recent_files.c)
target_link_libraries(wz_recent_files_tests PRIVATE wz_warnings)
target_include_directories(wz_recent_files_tests PRIVATE "${WZSN_SOURCE_ROOT}")
add_test(NAME wz_recent_files_tests COMMAND wz_recent_files_tests)

add_executable(
    wz_snapshot_workflow_tests
    ${WZSN_TEST_ROOT}/wz_snapshot_workflow_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_snapshot_workflow.c)
target_link_libraries(wz_snapshot_workflow_tests PRIVATE wz_core wz_warnings)
target_include_directories(wz_snapshot_workflow_tests PRIVATE "${WZSN_SOURCE_ROOT}")
add_test(NAME wz_snapshot_workflow_tests COMMAND wz_snapshot_workflow_tests)

add_executable(
    wz_snapshot_save_workflow_tests
    ${WZSN_TEST_ROOT}/wz_snapshot_save_workflow_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_snapshot_save_workflow.c)
target_link_libraries(wz_snapshot_save_workflow_tests PRIVATE wz_warnings)
target_include_directories(wz_snapshot_save_workflow_tests PRIVATE "${WZSN_SOURCE_ROOT}")
add_test(NAME wz_snapshot_save_workflow_tests COMMAND wz_snapshot_save_workflow_tests)

add_executable(
    wz_snapshot_inspector_tests
    ${WZSN_TEST_ROOT}/wz_snapshot_inspector_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_snapshot_inspector.c)
target_link_libraries(wz_snapshot_inspector_tests PRIVATE wz_core wz_warnings)
target_include_directories(wz_snapshot_inspector_tests PRIVATE "${WZSN_SOURCE_ROOT}")
add_test(NAME wz_snapshot_inspector_tests COMMAND wz_snapshot_inspector_tests)

add_executable(
    wz_snapshot_state_editor_tests
    ${WZSN_TEST_ROOT}/wz_snapshot_state_editor_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_snapshot_state_editor.c)
target_link_libraries(wz_snapshot_state_editor_tests PRIVATE wz_core wz_warnings)
target_include_directories(wz_snapshot_state_editor_tests PRIVATE "${WZSN_SOURCE_ROOT}")
add_test(NAME wz_snapshot_state_editor_tests COMMAND wz_snapshot_state_editor_tests)

add_executable(
    wz_screenshot_service_tests
    ${WZSN_TEST_ROOT}/wz_screenshot_service_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_screenshot_service.c)
target_link_libraries(wz_screenshot_service_tests PRIVATE wz_core wz_warnings)
target_include_directories(wz_screenshot_service_tests PRIVATE "${WZSN_SOURCE_ROOT}")
add_test(NAME wz_screenshot_service_tests COMMAND wz_screenshot_service_tests)

add_executable(
    wz_screenshot_save_workflow_tests
    ${WZSN_TEST_ROOT}/wz_screenshot_save_workflow_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_screenshot_save_workflow.c
    ${WZSN_SOURCE_ROOT}/app/wz_screenshot_service.c)
target_link_libraries(wz_screenshot_save_workflow_tests PRIVATE wz_core wz_warnings)
target_include_directories(wz_screenshot_save_workflow_tests PRIVATE "${WZSN_SOURCE_ROOT}")
add_test(NAME wz_screenshot_save_workflow_tests COMMAND wz_screenshot_save_workflow_tests)

add_executable(
    wz_file_snapshot_screenshot_acceptance_tests
    ${WZSN_TEST_ROOT}/wz_file_snapshot_screenshot_acceptance_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_file_open_run.c
    ${WZSN_SOURCE_ROOT}/app/wz_snapshot_workflow.c
    ${WZSN_SOURCE_ROOT}/app/wz_screenshot_save_workflow.c
    ${WZSN_SOURCE_ROOT}/app/wz_screenshot_service.c)
target_link_libraries(wz_file_snapshot_screenshot_acceptance_tests PRIVATE wz_core wz_warnings)
target_include_directories(wz_file_snapshot_screenshot_acceptance_tests PRIVATE "${WZSN_SOURCE_ROOT}")
add_test(NAME wz_file_snapshot_screenshot_acceptance_tests
         COMMAND wz_file_snapshot_screenshot_acceptance_tests)

add_executable(
    wz_tape_insert_workflow_tests
    ${WZSN_TEST_ROOT}/wz_tape_insert_workflow_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_tape_insert_workflow.c)
target_link_libraries(wz_tape_insert_workflow_tests PRIVATE wz_core wz_warnings)
target_include_directories(wz_tape_insert_workflow_tests PRIVATE "${WZSN_SOURCE_ROOT}")
add_test(NAME wz_tape_insert_workflow_tests COMMAND wz_tape_insert_workflow_tests)

add_executable(
    wz_tape_eject_workflow_tests
    ${WZSN_TEST_ROOT}/wz_tape_eject_workflow_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_tape_eject_workflow.c)
target_link_libraries(wz_tape_eject_workflow_tests PRIVATE wz_core wz_warnings)
target_include_directories(wz_tape_eject_workflow_tests PRIVATE "${WZSN_SOURCE_ROOT}")
add_test(NAME wz_tape_eject_workflow_tests COMMAND wz_tape_eject_workflow_tests)

add_executable(
    wz_ui_help_tests
    ${WZSN_TEST_ROOT}/wz_ui_help_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_ui_help.c)
target_link_libraries(wz_ui_help_tests PRIVATE wz_core)
add_test(NAME wz_ui_help_tests COMMAND wz_ui_help_tests)

add_test(NAME wz_screen_inspection_tests COMMAND wz_screen_inspection_tests)

add_executable(wz_presentation_snapshot_tests
    ${WZSN_TEST_ROOT}/wz_presentation_snapshot_tests.c)
target_link_libraries(wz_presentation_snapshot_tests PRIVATE wz_core)
add_test(NAME wz_presentation_snapshot_tests COMMAND wz_presentation_snapshot_tests)

add_executable(wz_host_presentation_tests
    ${WZSN_TEST_ROOT}/wz_host_presentation_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_presentation.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_scaling.c)
target_link_libraries(wz_host_presentation_tests PRIVATE wz_core)
add_test(NAME wz_host_presentation_tests COMMAND wz_host_presentation_tests)

add_executable(wz_rom_settings_tests
    ${WZSN_TEST_ROOT}/wz_rom_settings_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_rom_settings.c)
target_link_libraries(wz_rom_settings_tests PRIVATE wz_core)
add_test(NAME wz_rom_settings_tests COMMAND wz_rom_settings_tests)

add_executable(
    wz_host_output_tests
    ${WZSN_TEST_ROOT}/wz_host_output_tests.c
    ${WZSN_SOURCE_ROOT}/app/wz_host_output.c)
target_link_libraries(wz_host_output_tests PRIVATE wz_warnings)
target_include_directories(wz_host_output_tests PRIVATE "${WZSN_SOURCE_ROOT}")
add_test(NAME wz_host_output_tests COMMAND wz_host_output_tests)

endif()
if(WZSN_FUSE_Z80_TEST_INPUT AND WZSN_FUSE_Z80_TEST_EXPECTED)
    enable_testing()
    add_test(NAME fuse_z80_conformance
        COMMAND wz_tests "${WZSN_FUSE_Z80_TEST_INPUT}"
                        "${WZSN_FUSE_Z80_TEST_EXPECTED}")
endif()
if(WZSN_ENABLE_REVIEW_GATE_TESTS AND EXISTS "${WZSN_TEST_ROOT}/test_review_gate_v4.py")
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(
        NAME review_gate_tests
        COMMAND ${Python3_EXECUTABLE} "${WZSN_TEST_ROOT}/test_review_gate_v4.py"
    )
    add_test(
        NAME remote_harness_gate_tests
        COMMAND ${Python3_EXECUTABLE} "${WZSN_TEST_ROOT}/test_remote_harness_gate.py"
    )
    add_test(
        NAME fuse_harness_gate_tests
        COMMAND ${Python3_EXECUTABLE} "${WZSN_TEST_ROOT}/test_fuse_harness_gate.py"
    )
    add_test(
        NAME fuse_review_evidence_tests
        COMMAND ${Python3_EXECUTABLE} "${WZSN_TEST_ROOT}/test_fuse_review_evidence.py"
    )
    add_test(
        NAME differential_scenario_manifest_tests
        COMMAND ${Python3_EXECUTABLE} "${WZSN_TEST_ROOT}/test_differential_scenarios.py"
    )
endif()

add_custom_target(wz_gui_stub)

if(WZSN_BUILD_SOKOL_HOST)
    include(FetchContent)
    FetchContent_Declare(
        sokol
        GIT_REPOSITORY https://github.com/floooh/sokol.git
        GIT_TAG 1847290135f95e57e6d220b0a41208306aafc0dd
        GIT_SHALLOW FALSE
    )
    FetchContent_MakeAvailable(sokol)

    find_package(Threads REQUIRED)
    if(APPLE)
        add_executable(wz_sokol_host ${WZSN_SOURCE_ROOT}/app/wz_sokol_macos.m ${WZSN_SOURCE_ROOT}/app/wz_ui_window.c ${WZSN_SOURCE_ROOT}/app/wz_ui_layout.c ${WZSN_SOURCE_ROOT}/app/wz_command_registry.c ${WZSN_SOURCE_ROOT}/app/wz_host_thread.c ${WZSN_SOURCE_ROOT}/app/wz_sokol_raster.c ${WZSN_SOURCE_ROOT}/app/wz_application_lifecycle.c ${WZSN_SOURCE_ROOT}/app/wz_host_presentation.c ${WZSN_SOURCE_ROOT}/app/wz_host_scaling.c ${WZSN_SOURCE_ROOT}/app/wz_host_effects.c ${WZSN_SOURCE_ROOT}/app/wz_input_arbiter.c ${WZSN_SOURCE_ROOT}/app/wz_input_timing.c ${WZSN_SOURCE_ROOT}/app/wz_input_focus.c ${WZSN_SOURCE_ROOT}/app/wz_kempston_mapping.c ${WZSN_SOURCE_ROOT}/app/wz_speed_policy.c ${WZSN_SOURCE_ROOT}/app/wz_host_pacing.c ${WZSN_SOURCE_ROOT}/app/wz_host_config.c ${WZSN_SOURCE_ROOT}/app/wz_host_media_ownership.c ${WZSN_SOURCE_ROOT}/app/wz_host_audio_push.c ${WZSN_SOURCE_ROOT}/app/wz_host_audio_policy.c ${WZSN_SOURCE_ROOT}/app/wz_sokol_audio.c ${WZSN_SOURCE_ROOT}/app/wz_host_socket.c ${WZSN_SOURCE_ROOT}/app/wz_control_port.c ${WZSN_SOURCE_ROOT}/app/wz_telnet_client.c ${WZSN_SOURCE_ROOT}/app/wz_telnet_negotiation.c ${WZSN_SOURCE_ROOT}/app/wz_telnet_security.c ${WZSN_SOURCE_ROOT}/app/wz_telnet_input.c ${WZSN_SOURCE_ROOT}/app/wz_telnet_keymap.c ${WZSN_SOURCE_ROOT}/app/wz_telnet_keyboard_command.c ${WZSN_SOURCE_ROOT}/app/wz_screenshot_service.c ${WZSN_SOURCE_ROOT}/app/wz_host_output.c)
    else()
        add_executable(wz_sokol_host ${WZSN_SOURCE_ROOT}/app/wz_sokol_main.c ${WZSN_SOURCE_ROOT}/app/wz_ui_window.c ${WZSN_SOURCE_ROOT}/app/wz_ui_layout.c ${WZSN_SOURCE_ROOT}/app/wz_command_registry.c ${WZSN_SOURCE_ROOT}/app/wz_host_thread.c ${WZSN_SOURCE_ROOT}/app/wz_sokol_raster.c ${WZSN_SOURCE_ROOT}/app/wz_application_lifecycle.c ${WZSN_SOURCE_ROOT}/app/wz_host_presentation.c ${WZSN_SOURCE_ROOT}/app/wz_host_scaling.c ${WZSN_SOURCE_ROOT}/app/wz_host_effects.c ${WZSN_SOURCE_ROOT}/app/wz_input_arbiter.c ${WZSN_SOURCE_ROOT}/app/wz_input_timing.c ${WZSN_SOURCE_ROOT}/app/wz_input_focus.c ${WZSN_SOURCE_ROOT}/app/wz_kempston_mapping.c ${WZSN_SOURCE_ROOT}/app/wz_speed_policy.c ${WZSN_SOURCE_ROOT}/app/wz_host_pacing.c ${WZSN_SOURCE_ROOT}/app/wz_host_config.c ${WZSN_SOURCE_ROOT}/app/wz_host_media_ownership.c ${WZSN_SOURCE_ROOT}/app/wz_host_audio_push.c ${WZSN_SOURCE_ROOT}/app/wz_host_audio_policy.c ${WZSN_SOURCE_ROOT}/app/wz_sokol_audio.c ${WZSN_SOURCE_ROOT}/app/wz_host_socket.c ${WZSN_SOURCE_ROOT}/app/wz_control_port.c ${WZSN_SOURCE_ROOT}/app/wz_telnet_client.c ${WZSN_SOURCE_ROOT}/app/wz_telnet_negotiation.c ${WZSN_SOURCE_ROOT}/app/wz_telnet_security.c ${WZSN_SOURCE_ROOT}/app/wz_telnet_input.c ${WZSN_SOURCE_ROOT}/app/wz_telnet_keymap.c ${WZSN_SOURCE_ROOT}/app/wz_telnet_keyboard_command.c ${WZSN_SOURCE_ROOT}/app/wz_screenshot_service.c ${WZSN_SOURCE_ROOT}/app/wz_host_output.c)
    endif()
    set_target_properties(wz_sokol_host PROPERTIES OUTPUT_NAME "Warajevo-ZX-Spectrum-Next")
    target_include_directories(wz_sokol_host PRIVATE ${sokol_SOURCE_DIR} ${sokol_SOURCE_DIR}/util "${WZSN_SOURCE_ROOT}")
    target_link_libraries(wz_sokol_host PRIVATE wz_core wz_warnings Threads::Threads)
    if(WIN32)
        target_link_libraries(wz_sokol_host PRIVATE d3d11 dxgi user32 gdi32)
        target_link_libraries(wz_sokol_host PRIVATE ws2_32)
    elseif(APPLE)
        target_link_libraries(wz_sokol_host PRIVATE "-framework Cocoa" "-framework Metal" "-framework QuartzCore" "-framework AudioToolbox" "-framework CoreAudio")
    else()
        find_package(X11 REQUIRED)
        find_package(OpenGL REQUIRED)
        find_library(WZSN_ALSA_LIBRARY asound REQUIRED)
        target_link_libraries(
            wz_sokol_host
            PRIVATE
            ${X11_LIBRARIES}
            ${X11_Xi_LIB}
            ${X11_Xcursor_LIB}
            OpenGL::GL
            ${WZSN_ALSA_LIBRARY}
            ${CMAKE_DL_LIBS}
            m
        )
        target_compile_definitions(wz_sokol_host PRIVATE ${X11_DEFINITIONS})
    endif()
endif()
