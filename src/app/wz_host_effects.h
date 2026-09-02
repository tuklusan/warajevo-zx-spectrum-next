/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_HOST_EFFECTS_H
#define WZ_APP_WZ_HOST_EFFECTS_H

#include <stdbool.h>

typedef struct {
    bool crt_enabled;
    bool analog_enabled;
} wz_host_effects_policy_t;

wz_host_effects_policy_t wz_host_effects_default_policy(void);
bool wz_host_effects_are_disabled(const wz_host_effects_policy_t* policy);

#endif
