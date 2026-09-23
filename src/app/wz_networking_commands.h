/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_NETWORKING_COMMANDS_H
#define WZ_APP_WZ_NETWORKING_COMMANDS_H

#include <stdbool.h>

#include "app/wz_command_registry.h"
#include "core/wz_machine.h"

#define WZ_NETWORKING_COMMAND_ID "machine.networking.set"

typedef struct {
    wz_machine_t* machine;
    wz_mdr_flush_callback_t flush_callback;
    void* flush_context;
    bool discard_dirty_media;
} wz_networking_command_context_t;

wz_result_t wz_networking_commands_register(
    wz_command_registry_t* registry,
    wz_networking_command_context_t* context);

#endif
