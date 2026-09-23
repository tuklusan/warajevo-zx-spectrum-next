/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_PRINTER_MANAGER_H
#define WZ_APP_WZ_PRINTER_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

#include "app/wz_command_registry.h"
#include "app/wz_printer_export.h"

typedef struct {
    bool available;
    wz_printer_mode_t mode;
    wz_byte_t rows;
    wz_master_tick_t master_tick;
    wz_printer_bitmap_t bitmap;
} wz_printer_manager_view_t;

typedef struct {
    wz_printer_flush_event_t event;
    wz_printer_bitmap_t bitmap;
    bool available;
} wz_printer_manager_t;

typedef enum {
    WZ_PRINTER_MANAGER_OPEN = 0,
    WZ_PRINTER_MANAGER_EXPORT
} wz_printer_manager_operation_kind_t;

typedef struct {
    wz_printer_manager_operation_kind_t kind;
    const char* command_id;
    const char* label;
    const char* description;
    const char* parameter_schema;
    const char* parameter_acquisition;
    wz_command_permission_t permission;
    bool affects_machine_state;
    bool recordable;
} wz_printer_manager_operation_t;

void wz_printer_manager_init(wz_printer_manager_t* manager);
wz_result_t wz_printer_manager_accept_flush(
    wz_printer_manager_t* manager,
    const wz_printer_flush_event_t* event);
wz_result_t wz_printer_manager_view(
    const wz_printer_manager_t* manager,
    wz_printer_manager_view_t* view);
wz_result_t wz_printer_manager_render(
    const wz_printer_manager_t* manager,
    wz_byte_t* output,
    size_t capacity,
    wz_printer_bitmap_t* bitmap);

size_t wz_printer_manager_operation_count(void);
const wz_printer_manager_operation_t*
wz_printer_manager_operation_at(size_t index);
wz_result_t wz_printer_manager_register_commands(
    wz_command_registry_t* registry,
    wz_command_availability_fn availability,
    wz_command_handler_fn handler,
    const void* context);

#endif
