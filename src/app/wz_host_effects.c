/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_host_effects.h"

wz_host_effects_policy_t wz_host_effects_default_policy(void)
{
    return (wz_host_effects_policy_t){
        .crt_enabled = false,
        .analog_enabled = false,
    };
}

bool wz_host_effects_are_disabled(const wz_host_effects_policy_t* policy)
{
    return policy != 0 && !policy->crt_enabled && !policy->analog_enabled;
}
