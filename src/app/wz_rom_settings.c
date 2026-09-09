/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_rom_settings.h"

void wz_rom_settings_init(wz_rom_settings_t* settings)
{
    if (settings == 0) {
        return;
    }
    settings->machine_kind = WZ_MACHINE_48K_PAL;
    settings->identity = 0u;
    settings->status = WZ_ROM_SETTINGS_NONE;
}

wz_result_t wz_rom_settings_select(wz_rom_settings_t* settings,
                                    const wz_machine_profile_t* profile,
                                    wz_qword_t identity,
                                    bool license_approved)
{
    if (settings == 0 || profile == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (profile->expected_rom_identity == 0u) {
        settings->status = WZ_ROM_SETTINGS_UNAVAILABLE;
        return WZ_RESULT_INVALID_PROFILE;
    }
    if (identity == 0u || identity != profile->expected_rom_identity) {
        settings->status = WZ_ROM_SETTINGS_IDENTITY_MISMATCH;
        return WZ_RESULT_ROM_IDENTITY_MISMATCH;
    }
    if (!license_approved) {
        settings->status = WZ_ROM_SETTINGS_LICENSE_REQUIRED;
        return WZ_RESULT_INVALID_STATE;
    }
    settings->machine_kind = profile->kind;
    settings->identity = identity;
    settings->status = WZ_ROM_SETTINGS_READY;
    return WZ_RESULT_OK;
}

wz_machine_kind_t wz_rom_settings_machine_kind(
    const wz_rom_settings_t* settings)
{
    return settings == 0 ? WZ_MACHINE_48K_PAL : settings->machine_kind;
}

wz_qword_t wz_rom_settings_identity(const wz_rom_settings_t* settings)
{
    return settings == 0 ? 0u : settings->identity;
}

wz_rom_settings_status_t wz_rom_settings_status(
    const wz_rom_settings_t* settings)
{
    return settings == 0 ? WZ_ROM_SETTINGS_UNAVAILABLE : settings->status;
}
