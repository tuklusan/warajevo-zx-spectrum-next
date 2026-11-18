/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is licensed under the GNU General Public License, version 2 or later.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. */

#ifndef WZ_APP_WZ_TAPE_SAVE_TRANSACTION_H
#define WZ_APP_WZ_TAPE_SAVE_TRANSACTION_H

#include <stdbool.h>
#include <stddef.h>

#include "core/wz_machine.h"

typedef bool (*wz_tape_save_persist_fn)(void* context);

bool wz_tape_save_transaction_commit(
    wz_machine_t* machine, const wz_tape_segment_t* segments,
    size_t segment_count, wz_tape_save_persist_fn persist, void* context);

#endif
