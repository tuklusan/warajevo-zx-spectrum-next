/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#ifndef WZ_APP_WZ_FILE_DIALOG_H
#define WZ_APP_WZ_FILE_DIALOG_H

#include <stddef.h>

typedef enum {
    WZ_FILE_DIALOG_SELECTED = 0,
    WZ_FILE_DIALOG_CANCELLED,
    WZ_FILE_DIALOG_FAILED
} wz_file_dialog_result_t;

/* Select one existing file and return its absolute UTF-8 path. */
wz_file_dialog_result_t wz_file_dialog_open(char* utf8_path,
                                            size_t path_capacity);

/* Select a standard TAP destination and return its absolute UTF-8 path. */
wz_file_dialog_result_t wz_file_dialog_save_tap(char* utf8_path,
                                                size_t path_capacity);

/* Select an absolute UTF-8 destination for a SNA or Z80 snapshot. */
wz_file_dialog_result_t wz_file_dialog_save_snapshot(char* utf8_path,
                                                      size_t path_capacity);

#endif
