# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

if(PROJECT_NAME STREQUAL "WarajevoZXSpectrumNext")
    function(wzsn_check_core_dependency_boundary)
        foreach(target IN ITEMS wz_core wz_headless)
            if(NOT TARGET "${target}")
                message(FATAL_ERROR "Required target ${target} is missing")
            endif()
        endforeach()

        get_target_property(core_sources wz_core SOURCES)
        get_target_property(core_links wz_core LINK_LIBRARIES)
        get_target_property(core_interface_links wz_core INTERFACE_LINK_LIBRARIES)
        get_target_property(headless_links wz_headless LINK_LIBRARIES)
        if(NOT core_sources OR core_sources MATCHES "-NOTFOUND$")
            message(FATAL_ERROR "wz_core has no sources")
        endif()
        if(NOT core_links OR core_links MATCHES "-NOTFOUND$")
            message(FATAL_ERROR "wz_core has no declared dependencies")
        endif()
        if(NOT core_interface_links OR core_interface_links MATCHES "-NOTFOUND$")
            message(FATAL_ERROR "wz_core has no declared interface dependencies")
        endif()
        if(NOT headless_links OR headless_links MATCHES "-NOTFOUND$")
            message(FATAL_ERROR "wz_headless has no declared dependencies")
        endif()

        set(core_root "${WZSN_SOURCE_ROOT}/core")
        file(REAL_PATH "${core_root}" core_root)
        foreach(source IN LISTS core_sources)
            if(source MATCHES "\\$<")
                message(FATAL_ERROR "Generator-expression source is not permitted in wz_core: ${source}")
            endif()
            file(REAL_PATH "${source}" resolved_source)
            cmake_path(IS_PREFIX core_root "${resolved_source}" NORMALIZE is_core_source)
            if(NOT is_core_source)
                message(FATAL_ERROR "wz_core source escapes src/core: ${resolved_source}")
            endif()
        endforeach()

        set(expected_core_links wz_warnings ZLIB::ZLIB)
        set(actual_core_links ${core_links})
        set(actual_core_interface_links ${core_interface_links})
        list(REMOVE_DUPLICATES actual_core_links)
        list(REMOVE_DUPLICATES actual_core_interface_links)
        list(SORT expected_core_links)
        list(SORT actual_core_links)
        list(SORT actual_core_interface_links)
        if(NOT "${actual_core_links}" STREQUAL "${expected_core_links}")
            message(FATAL_ERROR "Unexpected wz_core link dependencies: ${core_links}")
        endif()
        if(NOT "${actual_core_interface_links}" STREQUAL "${expected_core_links}")
            message(FATAL_ERROR "Unexpected wz_core interface link dependencies: ${core_interface_links}")
        endif()
        if(NOT headless_links STREQUAL "wz_core")
            message(FATAL_ERROR "wz_headless must link directly to wz_core only: ${headless_links}")
        endif()

        file(WRITE "${CMAKE_BINARY_DIR}/core-boundary-targets.txt"
            "core_sources=${core_sources}\n"
            "core_links=${core_links}\n"
            "core_interface_links=${core_interface_links}\n"
            "headless_links=${headless_links}\n")
        message(STATUS "Core dependency boundary target inspection passed")
    endfunction()

    cmake_language(DEFER CALL wzsn_check_core_dependency_boundary)
endif()
