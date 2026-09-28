/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_telnet_status.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "status projection regression failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    static const char* const speeds[WZ_SPEED_COUNT] = {
        "25", "50", "100", "200", "400", "800", "UNLIMITED"
    };
    wz_telnet_status_snapshot_t snapshot = {
        30740u, true, false, true, 0, 0, 0, 0, 0
    };

    REQUIRE(wz_telnet_status_project_machine(
        &snapshot, wz_machine_profile_48k_pal(), WZ_NETWORKING_NONE,
        false, WZ_SPEED_100, true, false, false));
    REQUIRE(strcmp(snapshot.model, "48K") == 0);
    REQUIRE(strcmp(snapshot.state, "RUNNING") == 0);
    REQUIRE(strcmp(snapshot.speed, "100") == 0);
    REQUIRE(strcmp(snapshot.audio, "ON") == 0);
    REQUIRE(strcmp(snapshot.networking, "NONE") == 0);
    for (size_t index = 0u; index < WZ_SPEED_COUNT; ++index) {
        REQUIRE(wz_telnet_status_project_machine(
            &snapshot, wz_machine_profile_48k_pal(), WZ_NETWORKING_NONE,
            false, (wz_speed_policy_t)index, true, false, false));
        REQUIRE(strcmp(snapshot.speed, speeds[index]) == 0);
    }
    REQUIRE(wz_telnet_status_project_machine(
        &snapshot, wz_machine_profile_128k_pal(), WZ_NETWORKING_INTERFACE1,
        true, WZ_SPEED_800, true, false, false));
    REQUIRE(strcmp(snapshot.model, "128K") == 0);
    REQUIRE(strcmp(snapshot.state, "PAUSED") == 0);
    REQUIRE(strcmp(snapshot.speed, "800") == 0);
    REQUIRE(strcmp(snapshot.audio, "MUTED") == 0);
    REQUIRE(strcmp(snapshot.networking, "INTERFACE1") == 0);

    REQUIRE(wz_telnet_status_project_machine(
        &snapshot, wz_machine_profile_48k_pal(), WZ_NETWORKING_NONE,
        false, WZ_SPEED_100, true, false, true));
    REQUIRE(strcmp(snapshot.audio, "DEGRADED") == 0);

    REQUIRE(wz_telnet_status_project_machine(
        &snapshot, wz_machine_profile_48k_pal(), WZ_NETWORKING_EAR_MIC,
        false, WZ_SPEED_UNLIMITED, false, false, false));
    REQUIRE(strcmp(snapshot.speed, speeds[WZ_SPEED_UNLIMITED]) == 0);
    REQUIRE(strcmp(snapshot.audio, "UNAVAILABLE") == 0);
    REQUIRE(strcmp(snapshot.networking, "EAR_MIC") == 0);
    REQUIRE(!wz_telnet_status_project_machine(
        &snapshot, wz_machine_profile_48k_pal(),
        (wz_networking_mode_t)99, false, WZ_SPEED_100, true, false, false));

    puts("status projection regression passed");
    return 0;
}
