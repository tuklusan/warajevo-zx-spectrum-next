/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_host_config.h"
#include "app/wz_ui_layout.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Control Port state contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_ui_layout_state_t layout;
    wz_ui_remote_control_status_t status;
    wz_control_port_owner_t owner;
    wz_host_preferences_t preferences;
    char indicator[WZ_UI_STATUS_CAPACITY];
    char serialized[512];
    size_t written = 0u;
    unsigned cases = 0u;

    (void)memset(&owner, 0, sizeof(owner));
    wz_ui_layout_state_init(&layout);
    wz_ui_remote_control_status_init(&status);
    owner.selected_port = 0u;
    wz_ui_layout_sync_remote_control(&layout, &status, &owner, false);
    wz_ui_remote_control_indicator(&status, indicator, sizeof(indicator));
    REQUIRE(strncmp(indicator, "Control Port: unavailable", 25u) == 0);
    REQUIRE(layout.control_port_available == false);
    REQUIRE(status.selected_control_port_available == false);
    REQUIRE(status.listener_state == WZ_UI_REMOTE_LISTENER_UNAVAILABLE);
    cases += 4u;

    wz_host_preferences_init(&preferences);
    REQUIRE(wz_host_preferences_serialize(&preferences, serialized,
                                           sizeof(serialized), &written));
    REQUIRE(written > 0u && written < sizeof(serialized));
    REQUIRE(strstr(serialized, "control_port") == NULL);
    REQUIRE(strstr(serialized, "selected_port") == NULL);
    REQUIRE(strstr(serialized, "30740") == NULL);
    cases += 4u;

    (void)printf("ui-control-port-state cases=%u status=pass\n", cases);
    return 0;
}
