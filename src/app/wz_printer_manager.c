/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_printer_manager.h"

#include <stdint.h>
#include <string.h>

static const wz_printer_manager_operation_t operations[] = {
    {WZ_PRINTER_MANAGER_OPEN, "media.zx_printer.manager", "Printer Manager",
     "View captured virtual ZX Printer output", "none", "menu-or-command",
     WZ_COMMAND_HOST_READ, false, false},
    {WZ_PRINTER_MANAGER_EXPORT, "media.zx_printer.export", "Export Printer Output",
     "Export captured virtual ZX Printer output as a bitmap", "path",
     "save-file-dialog", WZ_COMMAND_HOST_WRITE, false, false}
};

void wz_printer_manager_init(wz_printer_manager_t* manager)
{
    if (manager != 0) memset(manager, 0, sizeof(*manager));
}

wz_result_t wz_printer_manager_accept_flush(
    wz_printer_manager_t* manager,
    const wz_printer_flush_event_t* event)
{
    wz_printer_bitmap_t bitmap;
    if (manager == 0 || event == 0 || event->length == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (event->mode == WZ_PRINTER_MODE_NONE ||
        event->length > WZ_PRINTER_OUTPUT_CAPACITY) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (event->mode == WZ_PRINTER_MODE_EPSON ||
        event->mode == WZ_PRINTER_MODE_EPSON_ENLARGED) {
        bitmap.width = event->length;
        bitmap.height = 8u;
    } else if (event->mode == WZ_PRINTER_MODE_HP ||
               event->mode == WZ_PRINTER_MODE_HP_ENLARGED) {
        bitmap.width = 8u;
        bitmap.height = 1u;
    } else {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (bitmap.width > SIZE_MAX / bitmap.height) return WZ_RESULT_INVALID_ARGUMENT;
    bitmap.stride = bitmap.width;
    bitmap.bytes = bitmap.width * bitmap.height;
    manager->event = *event;
    manager->bitmap = bitmap;
    manager->available = true;
    return WZ_RESULT_OK;
}

wz_result_t wz_printer_manager_view(
    const wz_printer_manager_t* manager,
    wz_printer_manager_view_t* view)
{
    if (manager == 0 || view == 0) return WZ_RESULT_INVALID_ARGUMENT;
    if (!manager->available) return WZ_RESULT_INVALID_STATE;
    view->available = true;
    view->mode = manager->event.mode;
    view->rows = manager->event.rows;
    view->master_tick = manager->event.master_tick;
    view->bitmap = manager->bitmap;
    return WZ_RESULT_OK;
}

wz_result_t wz_printer_manager_render(
    const wz_printer_manager_t* manager,
    wz_byte_t* output,
    size_t capacity,
    wz_printer_bitmap_t* bitmap)
{
    if (manager == 0 || !manager->available) return WZ_RESULT_INVALID_STATE;
    return wz_printer_export_bitmap(&manager->event, output, capacity, bitmap);
}

size_t wz_printer_manager_operation_count(void)
{
    return sizeof(operations) / sizeof(operations[0]);
}

const wz_printer_manager_operation_t*
wz_printer_manager_operation_at(size_t index)
{
    return index < wz_printer_manager_operation_count() ? &operations[index] : 0;
}

wz_result_t wz_printer_manager_register_commands(
    wz_command_registry_t* registry,
    wz_command_availability_fn availability,
    wz_command_handler_fn handler,
    const void* context)
{
    size_t index;
    if (registry == 0 || availability == 0 || handler == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    for (index = 0u; index < wz_printer_manager_operation_count(); ++index) {
        const wz_printer_manager_operation_t* operation =
            wz_printer_manager_operation_at(index);
        wz_command_metadata_t metadata = {
            operation->command_id,
            operation->label,
            operation->description,
            "media.zx_printer",
            operation->parameter_schema,
            "wz-command-result",
            "wz_printer_manager",
            operation->parameter_acquisition,
            0,
            operation->permission,
            availability,
            handler,
            context,
            operation->affects_machine_state,
            operation->recordable
        };
        if (wz_command_registry_register(registry, metadata) != WZ_RESULT_OK) {
            return WZ_RESULT_INVALID_STATE;
        }
    }
    return WZ_RESULT_OK;
}
