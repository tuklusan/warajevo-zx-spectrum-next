/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_host_command_queue.h"

void wz_host_command_queue_init(wz_host_command_queue_t* queue)
{
    if (queue == NULL) return;
    queue->read_index = 0u;
    queue->write_index = 0u;
    queue->count = 0u;
}

bool wz_host_command_queue_push(wz_host_command_queue_t* queue,
                                const uint8_t* command, size_t length)
{
    size_t index;
    if (queue == NULL || (command == NULL && length != 0u) ||
        length > WZ_HOST_COMMAND_MAX_LENGTH ||
        queue->count == WZ_HOST_COMMAND_QUEUE_CAPACITY) return false;
    for (index = 0u; index < length; ++index)
        queue->commands[queue->write_index][index] = command[index];
    queue->lengths[queue->write_index] = length;
    queue->write_index = (queue->write_index + 1u) % WZ_HOST_COMMAND_QUEUE_CAPACITY;
    ++queue->count;
    return true;
}

bool wz_host_command_queue_pop(wz_host_command_queue_t* queue,
                               uint8_t* command, size_t capacity,
                               size_t* length)
{
    size_t index;
    size_t queued_length;
    if (queue == NULL || command == NULL || length == NULL ||
        queue->count == 0u) return false;
    queued_length = queue->lengths[queue->read_index];
    if (queued_length > capacity) return false;
    for (index = 0u; index < queued_length; ++index)
        command[index] = queue->commands[queue->read_index][index];
    *length = queued_length;
    queue->read_index = (queue->read_index + 1u) % WZ_HOST_COMMAND_QUEUE_CAPACITY;
    --queue->count;
    return true;
}

size_t wz_host_command_queue_count(const wz_host_command_queue_t* queue)
{
    return queue == NULL ? 0u : queue->count;
}
