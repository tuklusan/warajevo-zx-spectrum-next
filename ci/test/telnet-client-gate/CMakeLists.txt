# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZTelnetClientGateRegression LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(SOURCE_ROOT "${REPO_ROOT}/src")
add_executable(wz_telnet_client_gate_regression
    telnet-client-gate-regression.c
    "${SOURCE_ROOT}/app/wz_control_port.c"
    "${SOURCE_ROOT}/app/wz_host_socket.c"
    "${SOURCE_ROOT}/app/wz_telnet_client.c"
    "${SOURCE_ROOT}/app/wz_telnet_input.c"
    "${SOURCE_ROOT}/app/wz_input_arbiter.c")
target_include_directories(wz_telnet_client_gate_regression PRIVATE "${SOURCE_ROOT}")
if(WIN32)
    target_link_libraries(wz_telnet_client_gate_regression PRIVATE ws2_32)
endif()
