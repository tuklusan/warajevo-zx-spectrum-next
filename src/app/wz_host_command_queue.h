/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_HOST_COMMAND_QUEUE_H
#define WZ_APP_WZ_HOST_COMMAND_QUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WZ_HOST_COMMAND_QUEUE_CAPACITY 32u
#define WZ_HOST_COMMAND_MAX_LENGTH 256u

typedef struct wz_host_command_queue {
    uint8_t commands[WZ_HOST_COMMAND_QUEUE_CAPACITY][WZ_HOST_COMMAND_MAX_LENGTH];
    size_t lengths[WZ_HOST_COMMAND_QUEUE_CAPACITY];
    size_t read_index;
    size_t write_index;
    size_t count;
} wz_host_command_queue_t;

void wz_host_command_queue_init(wz_host_command_queue_t* queue);
bool wz_host_command_queue_push(wz_host_command_queue_t* queue,
                                const uint8_t* command, size_t length);
bool wz_host_command_queue_pop(wz_host_command_queue_t* queue,
                               uint8_t* command, size_t capacity,
                               size_t* length);
size_t wz_host_command_queue_count(const wz_host_command_queue_t* queue);

#endif
