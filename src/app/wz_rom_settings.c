/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_rom_settings.h"

#include "core/wz_machine.h"

void wz_rom_settings_init(wz_rom_settings_t* settings)
{
    if (settings == 0) {
        return;
    }
    settings->machine_kind = WZ_MACHINE_48K_PAL;
    settings->expected_identity = 0u;
    settings->identity = 0u;
    settings->expected_bank_identity[0] = 0u;
    settings->expected_bank_identity[1] = 0u;
    settings->bank_identity[0] = 0u;
    settings->bank_identity[1] = 0u;
    settings->status = WZ_ROM_SETTINGS_NONE;
}

wz_result_t wz_rom_settings_select(wz_rom_settings_t* settings,
                                    const wz_machine_profile_t* profile,
                                    const wz_byte_t* bytes,
                                    size_t length,
                                    bool license_approved)
{
    wz_qword_t identity;

    if (settings == 0 || profile == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    settings->machine_kind = profile->kind;
    settings->expected_identity = profile->expected_rom_identity;
    settings->identity = 0u;
    settings->expected_bank_identity[0] = profile->expected_rom_bank0_identity;
    settings->expected_bank_identity[1] = profile->expected_rom_bank1_identity;
    settings->bank_identity[0] = 0u;
    settings->bank_identity[1] = 0u;
    if (profile->expected_rom_identity == 0u) {
        settings->status = WZ_ROM_SETTINGS_UNAVAILABLE;
        return WZ_RESULT_INVALID_PROFILE;
    }
    if (!license_approved) {
        settings->status = WZ_ROM_SETTINGS_LICENSE_REQUIRED;
        return WZ_RESULT_INVALID_STATE;
    }
    identity = wz_machine_rom_identity(bytes, length);
    settings->identity = identity;
    if (identity == 0u || identity != profile->expected_rom_identity) {
        settings->status = WZ_ROM_SETTINGS_IDENTITY_MISMATCH;
        return WZ_RESULT_ROM_IDENTITY_MISMATCH;
    }
    settings->status = WZ_ROM_SETTINGS_READY;
    return WZ_RESULT_OK;
}

wz_result_t wz_rom_settings_select_128k(wz_rom_settings_t* settings,
                                        const wz_machine_profile_t* profile,
                                        const wz_byte_t* bank0,
                                        size_t bank0_length,
                                        const wz_byte_t* bank1,
                                        size_t bank1_length,
                                        bool license_approved)
{
    if (settings == 0 || profile == 0) return WZ_RESULT_INVALID_ARGUMENT;
    settings->machine_kind = profile->kind;
    settings->expected_identity = 0u;
    settings->identity = 0u;
    settings->expected_bank_identity[0] = profile->expected_rom_bank0_identity;
    settings->expected_bank_identity[1] = profile->expected_rom_bank1_identity;
    settings->bank_identity[0] = wz_machine_rom_identity(bank0, bank0_length);
    settings->bank_identity[1] = wz_machine_rom_identity(bank1, bank1_length);
    if (profile->kind != WZ_MACHINE_128K_PAL ||
        settings->expected_bank_identity[0] == 0u ||
        settings->expected_bank_identity[1] == 0u) {
        settings->status = WZ_ROM_SETTINGS_UNAVAILABLE;
        return WZ_RESULT_INVALID_PROFILE;
    }
    if (!license_approved) {
        settings->status = WZ_ROM_SETTINGS_LICENSE_REQUIRED;
        return WZ_RESULT_INVALID_STATE;
    }
    if (settings->bank_identity[0] != settings->expected_bank_identity[0] ||
        settings->bank_identity[1] != settings->expected_bank_identity[1]) {
        settings->status = WZ_ROM_SETTINGS_IDENTITY_MISMATCH;
        return WZ_RESULT_ROM_IDENTITY_MISMATCH;
    }
    settings->status = WZ_ROM_SETTINGS_READY;
    return WZ_RESULT_OK;
}

wz_result_t wz_rom_settings_load_128k(wz_rom_settings_t* settings,
                                      wz_machine_t* machine,
                                      const wz_byte_t* bank0,
                                      size_t bank0_length,
                                      const wz_byte_t* bank1,
                                      size_t bank1_length,
                                      bool license_approved)
{
    wz_result_t result;
    if (settings == 0 || machine == 0 || machine->profile == 0)
        return WZ_RESULT_INVALID_ARGUMENT;
    result = wz_rom_settings_select_128k(settings, machine->profile,
        bank0, bank0_length, bank1, bank1_length, license_approved);
    if (result != WZ_RESULT_OK) return result;
    result = wz_machine_load_128k_roms(machine, bank0, bank0_length,
                                       bank1, bank1_length);
    if (result != WZ_RESULT_OK) settings->status = WZ_ROM_SETTINGS_LOAD_FAILED;
    return result;
}

wz_result_t wz_rom_settings_load(wz_rom_settings_t* settings,
                                 wz_machine_t* machine,
                                 const wz_byte_t* bytes,
                                 size_t length,
                                 bool license_approved)
{
    wz_result_t result;

    if (settings == 0 || machine == 0 || machine->profile == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    result = wz_rom_settings_select(settings, machine->profile, bytes, length,
                                    license_approved);
    if (result != WZ_RESULT_OK) {
        return result;
    }
    result = wz_machine_load_48k_rom(machine, bytes, length);
    if (result != WZ_RESULT_OK) {
        settings->status = WZ_ROM_SETTINGS_LOAD_FAILED;
        return result;
    }
    return result;
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
