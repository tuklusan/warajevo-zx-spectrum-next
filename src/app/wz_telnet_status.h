/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#ifndef WZ_APP_WZ_TELNET_STATUS_H
#define WZ_APP_WZ_TELNET_STATUS_H

#include "app/wz_speed_policy.h"
#include "app/wz_telnet_keyboard_command.h"
#include "core/wz_machine.h"

bool wz_telnet_status_project_machine(
    wz_telnet_status_snapshot_t* snapshot,
    const wz_machine_profile_t* profile,
    wz_networking_mode_t networking,
    bool paused,
    wz_speed_policy_t speed,
    bool audio_available,
    bool audio_muted);

#endif
