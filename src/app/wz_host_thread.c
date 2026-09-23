/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_host_thread.h"

#include <stdatomic.h>

static atomic_uint_fast64_t next_thread_id = ATOMIC_VAR_INIT(1u);
static _Thread_local uint64_t current_thread_id;

uint64_t wz_host_thread_current_id(void)
{
    if (current_thread_id == 0u) {
        do {
            current_thread_id = (uint64_t)atomic_fetch_add_explicit(
                &next_thread_id, 1u, memory_order_relaxed);
        } while (current_thread_id == 0u);
    }
    return current_thread_id;
}
