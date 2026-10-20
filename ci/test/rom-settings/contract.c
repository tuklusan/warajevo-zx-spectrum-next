/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_rom_settings.h"
#include "core/wz_state.h"

#include <stdio.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "ROM settings contract failed at line %d\n", \
                __LINE__); \
        return 1; \
    } \
} while (0)

int main(void)
{
    wz_machine_profile_t profile = *wz_machine_profile_48k_pal();
    wz_machine_t machine;
    wz_machine_t unsupported_machine;
    wz_machine_t banked_machine;
    wz_machine_profile_t banked_profile = *wz_machine_profile_128k_pal();
    wz_rom_settings_t settings;
    static wz_byte_t image[WZ_48K_ROM_SIZE];
    static wz_byte_t mismatch[WZ_48K_ROM_SIZE];
    static wz_byte_t original_memory[WZ_48K_ROM_SIZE];
    static wz_byte_t bank0[WZ_48K_ROM_SIZE];
    static wz_byte_t bank1[WZ_48K_ROM_SIZE];
    static wz_byte_t serialized[WZ_STATE_MACHINE_LENGTH];
    wz_state_writer_t writer;
    wz_qword_t identity;

    REQUIRE(wz_machine_profile_48k_pal()->expected_rom_identity ==
            UINT64_C(0x0a927c36965e6e82));
    REQUIRE(wz_machine_profile_128k_pal()->expected_rom_identity == 0u);
    REQUIRE(wz_machine_profile_128k_pal()->expected_rom_bank0_identity ==
            UINT64_C(0x167a39561d9acbe6));
    REQUIRE(wz_machine_profile_128k_pal()->expected_rom_bank1_identity ==
            UINT64_C(0x42501740662a4075));
    for (size_t index = 0u; index < sizeof(image); ++index) {
        image[index] = (wz_byte_t)(index * 37u + 19u);
    }
    identity = wz_machine_rom_identity(image, sizeof(image));
    REQUIRE(identity != 0u);
    profile.expected_rom_identity = identity;

    memset(&machine, 0, sizeof(machine));
    REQUIRE(wz_machine_init(&machine, &profile) == WZ_RESULT_OK);
    memcpy(original_memory, machine.memory, sizeof(original_memory));
    wz_rom_settings_init(&settings);
    REQUIRE(settings.status == WZ_ROM_SETTINGS_NONE);
    REQUIRE(settings.identity == 0u);

    REQUIRE(wz_rom_settings_load(&settings, &machine, image, sizeof(image),
                                 false) == WZ_RESULT_INVALID_STATE);
    REQUIRE(settings.status == WZ_ROM_SETTINGS_LICENSE_REQUIRED);
    REQUIRE(settings.machine_kind == WZ_MACHINE_48K_PAL);
    REQUIRE(settings.expected_identity == identity);
    REQUIRE(settings.identity == 0u);
    REQUIRE(machine.has_48k_rom == 0u && machine.rom_identity == 0u);
    REQUIRE(memcmp(machine.memory, original_memory, sizeof(original_memory)) == 0);

    memcpy(mismatch, image, sizeof(mismatch));
    mismatch[0] ^= 0x80u;
    REQUIRE(wz_rom_settings_load(&settings, &machine, mismatch,
        sizeof(mismatch), true) == WZ_RESULT_ROM_IDENTITY_MISMATCH);
    REQUIRE(settings.status == WZ_ROM_SETTINGS_IDENTITY_MISMATCH);
    REQUIRE(settings.identity == wz_machine_rom_identity(mismatch,
                                                         sizeof(mismatch)));
    REQUIRE(machine.has_48k_rom == 0u && machine.rom_identity == 0u);
    REQUIRE(memcmp(machine.memory, original_memory, sizeof(original_memory)) == 0);

    REQUIRE(wz_rom_settings_load(&settings, &machine, image, sizeof(image),
                                 true) == WZ_RESULT_OK);
    REQUIRE(settings.status == WZ_ROM_SETTINGS_READY);
    REQUIRE(settings.machine_kind == WZ_MACHINE_48K_PAL);
    REQUIRE(settings.expected_identity == identity);
    REQUIRE(settings.identity == identity);
    REQUIRE(machine.has_48k_rom != 0u && machine.rom_identity == identity);
    REQUIRE(memcmp(machine.memory, image, sizeof(image)) == 0);

    REQUIRE(wz_rom_settings_load(&settings, &machine, image,
        sizeof(image) - 1u, true) == WZ_RESULT_ROM_IDENTITY_MISMATCH);
    REQUIRE(settings.status == WZ_ROM_SETTINGS_IDENTITY_MISMATCH);
    REQUIRE(settings.identity == 0u);
    REQUIRE(machine.has_48k_rom != 0u && machine.rom_identity == identity);
    REQUIRE(memcmp(machine.memory, image, sizeof(image)) == 0);

    REQUIRE(wz_rom_settings_select(&settings, wz_machine_profile_128k_pal(),
        image, sizeof(image), true) == WZ_RESULT_INVALID_PROFILE);
    REQUIRE(settings.status == WZ_ROM_SETTINGS_UNAVAILABLE);
    REQUIRE(settings.machine_kind == WZ_MACHINE_128K_PAL);
    REQUIRE(settings.expected_identity == 0u);
    REQUIRE(settings.identity == 0u);

    memset(&unsupported_machine, 0, sizeof(unsupported_machine));
    REQUIRE(wz_machine_init(&unsupported_machine,
        wz_machine_profile_128k_pal()) == WZ_RESULT_OK);
    REQUIRE(wz_rom_settings_load(&settings, &unsupported_machine, image,
        sizeof(image), true) == WZ_RESULT_INVALID_PROFILE);
    REQUIRE(settings.status == WZ_ROM_SETTINGS_UNAVAILABLE);
    REQUIRE(unsupported_machine.has_48k_rom == 0u);
    wz_machine_destroy(&unsupported_machine);

    for (size_t index = 0u; index < sizeof(bank0); ++index) {
        bank0[index] = (wz_byte_t)(index * 13u + 7u);
        bank1[index] = (wz_byte_t)(index * 29u + 3u);
    }
    banked_profile.expected_rom_bank0_identity =
        wz_machine_rom_identity(bank0, sizeof(bank0));
    banked_profile.expected_rom_bank1_identity =
        wz_machine_rom_identity(bank1, sizeof(bank1));
    memset(&banked_machine, 0, sizeof(banked_machine));
    REQUIRE(wz_machine_init(&banked_machine, &banked_profile) == WZ_RESULT_OK);
    REQUIRE(wz_rom_settings_load_128k(&settings, &banked_machine,
        bank0, sizeof(bank0), bank1, sizeof(bank1), true) == WZ_RESULT_OK);
    REQUIRE(settings.status == WZ_ROM_SETTINGS_READY);
    REQUIRE(settings.bank_identity[0] == banked_profile.expected_rom_bank0_identity);
    REQUIRE(settings.bank_identity[1] == banked_profile.expected_rom_bank1_identity);
    REQUIRE(banked_machine.has_128k_rom != 0u);
    REQUIRE(banked_machine.rom_128k_identity[0] ==
            banked_profile.expected_rom_bank0_identity);
    REQUIRE(banked_machine.rom_128k_identity[1] ==
            banked_profile.expected_rom_bank1_identity);
    REQUIRE(wz_machine_memory_read(&banked_machine, 0u) == bank0[0]);
    wz_state_writer_init(&writer, serialized, sizeof(serialized));
    REQUIRE(wz_state_serialize_machine(&banked_machine, &writer) ==
            WZ_RESULT_OK);
    REQUIRE(serialized[WZ_STATE_MACHINE_MEMORY_OFFSET + 65536u + 4u] == 5u);
    REQUIRE(serialized[WZ_STATE_MACHINE_MEMORY_OFFSET + 65536u + 20u] == 1u);
    REQUIRE(wz_machine_128k_paging_write(&banked_machine, 0x7ffdu, 0x10u) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_memory_read(&banked_machine, 0u) == bank1[0]);
    REQUIRE(wz_machine_128k_paging_write(&banked_machine, 0x7ffdu, 0u) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_memory_read(&banked_machine, 0u) == bank0[0]);
    bank1[0] ^= 0xffu;
    REQUIRE(wz_rom_settings_load_128k(&settings, &banked_machine,
        bank0, sizeof(bank0), bank1, sizeof(bank1), true) ==
        WZ_RESULT_ROM_IDENTITY_MISMATCH);
    REQUIRE(settings.status == WZ_ROM_SETTINGS_IDENTITY_MISMATCH);
    REQUIRE(wz_machine_memory_read(&banked_machine, 0u) == bank0[0]);
    wz_machine_destroy(&banked_machine);

    wz_machine_destroy(&machine);
    puts("ROM settings contract passed");
    return 0;
}
