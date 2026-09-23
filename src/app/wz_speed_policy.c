/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_speed_policy.h"

static const unsigned wz_speed_percentages[WZ_SPEED_COUNT] =
    {25u, 50u, 100u, 200u, 400u, 800u, 0u};

bool wz_speed_policy_valid(wz_speed_policy_t policy)
{
    return (unsigned)policy < WZ_SPEED_COUNT;
}

unsigned wz_speed_policy_percent(wz_speed_policy_t policy)
{
    return wz_speed_policy_valid(policy) ? wz_speed_percentages[policy] : 0u;
}

bool wz_speed_policy_is_unlimited(wz_speed_policy_t policy)
{
    return policy == WZ_SPEED_UNLIMITED;
}
