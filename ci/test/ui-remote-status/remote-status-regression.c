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
        fprintf(stderr, "remote status regression failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_ui_layout_state_t state;
    wz_ui_remote_control_status_t status;
    wz_control_port_owner_t owner;
    char indicator[WZ_UI_STATUS_CAPACITY];
    char page[WZ_UI_REMOTE_STATUS_CAPACITY];

    memset(&owner, 0, sizeof(owner));
    wz_ui_layout_state_init(&state);
    wz_ui_remote_control_status_init(&status);
    owner.selected_port = 30741u;
    owner.ipv4_active = true;
    owner.ipv6_active = false;
    wz_ui_layout_sync_remote_control(&state, &status, &owner, true);
    REQUIRE(state.control_port_available);
    REQUIRE(state.control_port == 30741u);
    REQUIRE(status.selected_control_port_available);
    REQUIRE(status.selected_control_port == 30741u);
    REQUIRE(status.ipv4_listener_up);
    REQUIRE(!status.ipv6_listener_up);
    REQUIRE(status.listener_state == WZ_UI_REMOTE_LISTENER_DEGRADED);
    REQUIRE(status.active_client);
    wz_ui_remote_control_indicator(&status, indicator, sizeof(indicator));
    REQUIRE(strcmp(indicator,
            "Control Port: 30741 | IPv4: UP | IPv6: DOWN | "
            "Listener: DEGRADED | Telnet: ACTIVE") == 0);
    wz_ui_remote_control_status_page(&status, page, sizeof(page));
    REQUIRE(strstr(page, "Base Control Port: 30740") != NULL);
    REQUIRE(strstr(page, "Probe range: 30740-32787") != NULL);
    REQUIRE(strstr(page, "Selected Control Port: 30741") != NULL);
    REQUIRE(strstr(page, "IPv4 listener: UP | IPv6 listener: DOWN") != NULL);
    REQUIRE(strstr(page, "Listener: DEGRADED | Active client: ACTIVE") != NULL);
    REQUIRE(strstr(page, "Warning: plaintext/no-authentication") != NULL);
    REQUIRE(strstr(page, "Permissions: default-deny host-file/destructive/quit policy") != NULL);

    owner.selected_port = 0u;
    owner.ipv4_active = false;
    owner.ipv6_active = false;
    wz_ui_layout_sync_remote_control(&state, &status, &owner, false);
    wz_ui_remote_control_indicator(&status, indicator, sizeof(indicator));
    REQUIRE(strcmp(indicator,
            "Control Port: unavailable | IPv4: DOWN | IPv6: DOWN | "
            "Listener: UNAVAILABLE | Telnet: NONE") == 0);
    wz_ui_remote_control_status_page(&status, page, sizeof(page));
    REQUIRE(strstr(page, "Selected Control Port: unavailable") != NULL);
    REQUIRE(state.remote_listener_state == WZ_UI_REMOTE_LISTENER_UNAVAILABLE);
    puts("UI remote status projection regression passed");
    return 0;
}
