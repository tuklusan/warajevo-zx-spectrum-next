/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#ifndef WZ_APP_WZ_INPUT_PROJECTION_H
#define WZ_APP_WZ_INPUT_PROJECTION_H

#include <stdbool.h>

#include "app/wz_input_arbiter.h"
#include "core/wz_machine.h"

/* Resolve host-source ownership, then project normalized matrix levels into
 * the machine. No source identity crosses this app-to-core boundary. */
bool wz_input_projection_apply_keyboard(const wz_input_arbiter_t* arbiter,
                                       wz_machine_t* machine);

#endif
