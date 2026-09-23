/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_PRINTER_COMMANDS_H
#define WZ_APP_WZ_PRINTER_COMMANDS_H

#include "app/wz_command_registry.h"
#include "core/wz_machine.h"

#define WZ_PRINTER_MODE_COMMAND_ID "settings.peripherals.zx_printer.mode.set"

typedef struct { wz_machine_t* machine; } wz_printer_command_context_t;

wz_result_t wz_printer_commands_register(
    wz_command_registry_t* registry,
    wz_printer_command_context_t* context);

#endif
