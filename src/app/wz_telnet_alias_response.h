/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#ifndef WZ_APP_WZ_TELNET_ALIAS_RESPONSE_H
#define WZ_APP_WZ_TELNET_ALIAS_RESPONSE_H

#include <stdbool.h>
#include <stddef.h>

bool wz_telnet_model_alias_to_do(const char* alias, char* output,
                                 size_t output_capacity);

bool wz_telnet_alias_response_rewrite(const char* alias,
                                      const char* dispatch_response,
                                      char* output,
                                      size_t output_capacity,
                                      size_t* output_length);

#endif
