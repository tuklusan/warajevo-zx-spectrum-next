/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is licensed under the GNU General Public License, version 2 or later.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_HOST_MACHINE_FRAME_H
#define WZ_APP_WZ_HOST_MACHINE_FRAME_H

#include <stdbool.h>

#include "core/wz_runner.h"

/*
 * Output is called synchronously after the emulated frame has completed.
 * It must not wait for device capacity. A false result reports dropped or
 * deferred output and never changes the machine execution result.
 */
typedef bool (*wz_host_machine_frame_output_fn)(
    void* context, wz_master_tick_t start_tick,
    wz_byte_t initial_beeper_level, const wz_ay_t* initial_ay);

wz_result_t wz_host_machine_frame_execute(
    wz_headless_runner_t* runner, wz_master_tick_t frame_ticks,
    bool capture_audio, wz_host_machine_frame_output_fn output,
    void* output_context);

#endif
