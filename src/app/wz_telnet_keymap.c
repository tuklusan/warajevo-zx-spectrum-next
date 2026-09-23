/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <string.h>

#include "app/wz_telnet_keymap.h"
#include "core/wz_keyboard_matrix.h"

typedef struct {
    const char* name;
    wz_keyboard_key_t key;
} wz_telnet_key_name_t;

static const wz_telnet_key_name_t wz_telnet_key_names[] = {
    {"CAPS_SHIFT", WZ_KEY_SHIFT}, {"Z", WZ_KEY_Z}, {"X", WZ_KEY_X},
    {"C", WZ_KEY_C}, {"V", WZ_KEY_V}, {"A", WZ_KEY_A}, {"S", WZ_KEY_S},
    {"D", WZ_KEY_D}, {"F", WZ_KEY_F}, {"G", WZ_KEY_G}, {"Q", WZ_KEY_Q},
    {"W", WZ_KEY_W}, {"E", WZ_KEY_E}, {"R", WZ_KEY_R}, {"T", WZ_KEY_T},
    {"1", WZ_KEY_1}, {"2", WZ_KEY_2}, {"3", WZ_KEY_3}, {"4", WZ_KEY_4},
    {"5", WZ_KEY_5}, {"0", WZ_KEY_0}, {"9", WZ_KEY_9}, {"8", WZ_KEY_8},
    {"7", WZ_KEY_7}, {"6", WZ_KEY_6}, {"P", WZ_KEY_P}, {"O", WZ_KEY_O},
    {"I", WZ_KEY_I}, {"U", WZ_KEY_U}, {"Y", WZ_KEY_Y}, {"ENTER", WZ_KEY_ENTER},
    {"L", WZ_KEY_L}, {"K", WZ_KEY_K}, {"J", WZ_KEY_J}, {"H", WZ_KEY_H},
    {"SPACE", WZ_KEY_SPACE}, {"SYMBOL_SHIFT", WZ_KEY_SYMBOL_SHIFT},
    {"M", WZ_KEY_M}, {"N", WZ_KEY_N}, {"B", WZ_KEY_B}
};

bool wz_telnet_keymap_lookup(const char* name, size_t* physical_key)
{
    if (name == 0 || physical_key == 0) {
        return false;
    }
    for (size_t index = 0u;
         index < sizeof(wz_telnet_key_names) / sizeof(wz_telnet_key_names[0]);
         ++index) {
        if (strcmp(name, wz_telnet_key_names[index].name) == 0) {
            *physical_key = (size_t)wz_telnet_key_names[index].key;
            return true;
        }
    }
    return false;
}
