/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_ui_layout.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Control Port indicator contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_ui_remote_control_status_t status;
    wz_control_port_owner_t owner;
    char indicator[WZ_UI_STATUS_CAPACITY];
    const char expected_prefix[] = "Control Port: 32787";

    (void)memset(&owner, 0, sizeof(owner));
    wz_ui_remote_control_status_init(&status);
    owner.selected_port = 32787u;
    owner.ipv4_active = true;
    wz_ui_layout_sync_remote_control(NULL, &status, &owner, false);
    wz_ui_remote_control_indicator(&status, indicator, sizeof(indicator));
    REQUIRE(status.selected_control_port_available);
    REQUIRE(status.selected_control_port == 32787u);
    REQUIRE(status.ipv4_listener_up);
    REQUIRE(strncmp(indicator, expected_prefix,
                    sizeof(expected_prefix) - 1u) == 0);

    (void)printf("ui-control-port-indicator cases=4 status=pass\n");
    return 0;
}
