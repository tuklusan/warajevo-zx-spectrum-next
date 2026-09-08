/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_printer_manager.h"

#include <string.h>

static bool available(const void* context, const char** reason)
{
    (void)context;
    if (reason != 0) *reason = 0;
    return true;
}

static wz_result_t handle(const void* context, wz_command_arguments_t arguments,
                          wz_command_result_t* result)
{
    (void)context;
    (void)arguments;
    if (result != 0) result->status = WZ_COMMAND_RESULT_SUCCESS;
    return WZ_RESULT_OK;
}

static int test_view_and_render(void)
{
    wz_printer_manager_t manager;
    wz_printer_manager_view_t view;
    wz_printer_flush_event_t event;
    wz_printer_bitmap_t bitmap;
    wz_byte_t output[64];
    wz_printer_manager_init(&manager);
    if (wz_printer_manager_view(&manager, &view) != WZ_RESULT_INVALID_STATE) return 1;
    memset(&event, 0, sizeof(event));
    event.mode = WZ_PRINTER_MODE_EPSON;
    event.rows = 3u;
    event.master_tick = 44u;
    event.length = 2u;
    event.data[0] = 0x81u;
    event.data[1] = 0x24u;
    if (wz_printer_manager_accept_flush(&manager, &event) != WZ_RESULT_OK ||
        wz_printer_manager_view(&manager, &view) != WZ_RESULT_OK ||
        !view.available || view.mode != WZ_PRINTER_MODE_EPSON ||
        view.rows != 3u || view.master_tick != 44u || view.bitmap.bytes != 16u ||
        wz_printer_manager_render(&manager, output, sizeof(output), &bitmap) != WZ_RESULT_OK ||
        bitmap.width != 2u || bitmap.height != 8u || output[0] != 0xffu ||
        output[1] != 0u || output[5] != 0xffu) return 1;
    return 0;
}

static int test_registry_surface(void)
{
    wz_command_metadata_t storage[2];
    wz_command_registry_t registry;
    if (wz_command_registry_init(&registry, storage, 2u) != WZ_RESULT_OK ||
        wz_printer_manager_register_commands(&registry, 0, handle, 0) !=
            WZ_RESULT_INVALID_ARGUMENT) {
        return 1;
    }
    wz_command_registry_init(&registry, storage, 2u);
    if (wz_printer_manager_register_commands(&registry, available, handle, 0) !=
            WZ_RESULT_OK || wz_command_registry_finalize(&registry) != WZ_RESULT_OK ||
        wz_command_registry_find(&registry, "media.zx_printer.manager") == 0 ||
        wz_command_registry_find(&registry, "media.zx_printer.export") == 0) return 1;
    return 0;
}

int main(void)
{
    return test_view_and_render() != 0 || test_registry_surface() != 0;
}
