/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_SPEED_POLICY_H
#define WZ_APP_WZ_SPEED_POLICY_H

#include <stdbool.h>

typedef enum {
    WZ_SPEED_25 = 0,
    WZ_SPEED_50,
    WZ_SPEED_100,
    WZ_SPEED_200,
    WZ_SPEED_400,
    WZ_SPEED_800,
    WZ_SPEED_UNLIMITED,
    WZ_SPEED_COUNT
} wz_speed_policy_t;

bool wz_speed_policy_valid(wz_speed_policy_t policy);
unsigned wz_speed_policy_percent(wz_speed_policy_t policy);
bool wz_speed_policy_is_unlimited(wz_speed_policy_t policy);

#endif
