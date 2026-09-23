# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZMultiInstanceStress LANGUAGES C)

set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Build the opt-in Sokol host shell." FORCE)
get_filename_component(WZSN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_subdirectory("${WZSN_ROOT}/src/cmake" "${CMAKE_CURRENT_BINARY_DIR}/product")

add_executable(wz_multi_instance_worker
    "${WZSN_ROOT}/tests/multi-instance-stress.c"
    "${WZSN_ROOT}/src/app/wz_control_port.c"
    "${WZSN_ROOT}/src/app/wz_host_socket.c"
    "${WZSN_ROOT}/src/app/wz_host_config.c"
    "${WZSN_ROOT}/src/app/wz_host_media_ownership.c"
    "${WZSN_ROOT}/src/app/wz_host_output.c"
    "${WZSN_ROOT}/src/app/wz_screenshot_service.c"
    "${WZSN_ROOT}/src/app/wz_telnet_keyboard_command.c"
    "${WZSN_ROOT}/src/app/wz_command_registry.c"
    "${WZSN_ROOT}/src/app/wz_host_thread.c"
    "${WZSN_ROOT}/src/app/wz_telnet_keymap.c")
target_include_directories(wz_multi_instance_worker PRIVATE
    "${WZSN_ROOT}/src")
set_target_properties(wz_multi_instance_worker PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
target_link_libraries(wz_multi_instance_worker PRIVATE wz_core wz_warnings)
if(WIN32)
    target_link_libraries(wz_multi_instance_worker PRIVATE ws2_32)
endif()
