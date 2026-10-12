/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_ROM_SETTINGS_H
#define WZ_APP_WZ_ROM_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>

#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"
#include "core/wz_types.h"

typedef enum {
    WZ_ROM_SETTINGS_NONE = 0,
    WZ_ROM_SETTINGS_READY,
    WZ_ROM_SETTINGS_UNAVAILABLE,
    WZ_ROM_SETTINGS_IDENTITY_MISMATCH,
    WZ_ROM_SETTINGS_LICENSE_REQUIRED,
    WZ_ROM_SETTINGS_LOAD_FAILED
} wz_rom_settings_status_t;

typedef struct {
    wz_machine_kind_t machine_kind;
    /* Identity required by the active machine profile. */
    wz_qword_t expected_identity;
    /* Identity of the selected candidate, including rejected candidates. */
    wz_qword_t identity;
    wz_rom_settings_status_t status;
} wz_rom_settings_t;

void wz_rom_settings_init(wz_rom_settings_t* settings);
wz_result_t wz_rom_settings_select(wz_rom_settings_t* settings,
                                   const wz_machine_profile_t* profile,
                                   const wz_byte_t* bytes,
                                   size_t length,
                                   bool license_approved);
wz_result_t wz_rom_settings_load(wz_rom_settings_t* settings,
                                 wz_machine_t* machine,
                                 const wz_byte_t* bytes,
                                 size_t length,
                                 /* Caller-confirmed rights for these bytes. */
                                 bool license_approved);
wz_machine_kind_t wz_rom_settings_machine_kind(
    const wz_rom_settings_t* settings);
wz_qword_t wz_rom_settings_identity(const wz_rom_settings_t* settings);
wz_rom_settings_status_t wz_rom_settings_status(
    const wz_rom_settings_t* settings);

#endif
