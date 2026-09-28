/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_telnet_status.h"

static const char* const speed_values[WZ_SPEED_COUNT] = {
    "25", "50", "100", "200", "400", "800", "UNLIMITED"
};

bool wz_telnet_status_project_machine(
    wz_telnet_status_snapshot_t* snapshot,
    const wz_machine_profile_t* profile,
    wz_networking_mode_t networking,
    bool paused,
    wz_speed_policy_t speed,
    bool audio_available,
    bool audio_muted)
{
    if (snapshot == 0 || profile == 0 || !wz_speed_policy_valid(speed)) {
        return false;
    }
    switch (profile->kind) {
    case WZ_MACHINE_48K_PAL: snapshot->model = "48K"; break;
    case WZ_MACHINE_128K_PAL: snapshot->model = "128K"; break;
    default: return false;
    }
    switch (networking) {
    case WZ_NETWORKING_NONE: snapshot->networking = "NONE"; break;
    case WZ_NETWORKING_INTERFACE1: snapshot->networking = "INTERFACE1"; break;
    case WZ_NETWORKING_EAR_MIC: snapshot->networking = "EAR_MIC"; break;
    default: return false;
    }
    snapshot->state = paused ? "PAUSED" : "RUNNING";
    snapshot->speed = speed_values[speed];
    snapshot->audio = !audio_available ? "UNAVAILABLE" :
        (audio_muted || paused ? "MUTED" : "ON");
    return true;
}
