/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <stdint.h>

#include "app/wz_rom_settings.h"

int main(void)
{
    wz_machine_profile_t profile = *wz_machine_profile_48k_pal();
    wz_rom_settings_t settings;

    wz_rom_settings_init(&settings);
    profile.expected_rom_identity = UINT64_C(0x123456789abcdef0);
    if (wz_rom_settings_select(&settings, &profile,
                               profile.expected_rom_identity, false) !=
            WZ_RESULT_INVALID_STATE ||
        wz_rom_settings_status(&settings) != WZ_ROM_SETTINGS_LICENSE_REQUIRED ||
        wz_rom_settings_select(&settings, &profile, 0x1u, true) !=
            WZ_RESULT_ROM_IDENTITY_MISMATCH ||
        wz_rom_settings_status(&settings) != WZ_ROM_SETTINGS_IDENTITY_MISMATCH ||
        wz_rom_settings_select(&settings, &profile,
                               profile.expected_rom_identity, true) !=
            WZ_RESULT_OK ||
        wz_rom_settings_status(&settings) != WZ_ROM_SETTINGS_READY ||
        wz_rom_settings_identity(&settings) != profile.expected_rom_identity ||
        wz_rom_settings_machine_kind(&settings) != profile.kind) {
        fputs("ROM settings transaction contract failed\n", stderr);
        return 1;
    }
    puts("ROM settings transaction contract passed");
    return 0;
}
