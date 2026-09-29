# Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
# This file is governed by the SANYALnet Labs Non-Commercial License in the
# root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
# for AI/ML model training are prohibited unless separately authorized.
# Attribution is required: "Based on original work by Supratim Sanyal of
# SANYALnet Labs." See LICENSE for full terms.

cmake_minimum_required(VERSION 3.25)
project(WZMediaCorpusIngestion LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSPECTRUM REQUIRED IMPORTED_TARGET libspectrum)

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(WZSN_ENABLE_WARNINGS ON CACHE BOOL "Enable project warnings" FORCE)
set(WZSN_BUILD_SOKOL_HOST OFF CACHE BOOL "Disable native UI host" FORCE)
add_subdirectory("${REPO_ROOT}/src/cmake" "${CMAKE_BINARY_DIR}/product" EXCLUDE_FROM_ALL)

add_executable(wz_media_corpus_ingestion media-corpus-ingestion.c)
set_target_properties(wz_media_corpus_ingestion PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
target_link_libraries(wz_media_corpus_ingestion PRIVATE
    wz_core wz_warnings PkgConfig::LIBSPECTRUM)
