/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#ifndef WZ_APP_WZ_HOST_OUTPUT_UTF8_H
#define WZ_APP_WZ_HOST_OUTPUT_UTF8_H

#include <stdbool.h>
#include <stddef.h>

/* Atomically replace a file addressed by an absolute or relative UTF-8 path. */
bool wz_host_output_write_atomic_utf8(const char* utf8_path,
                                      const void* data,
                                      size_t size);

#endif
