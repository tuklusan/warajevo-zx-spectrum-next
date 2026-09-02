/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_host_audio_policy.h"

bool wz_host_audio_enabled(wz_speed_policy_t speed)
{
    return speed == WZ_SPEED_50 || speed == WZ_SPEED_100 ||
           speed == WZ_SPEED_200;
}
