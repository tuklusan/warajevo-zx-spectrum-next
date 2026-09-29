/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_host_machine_frame.h"

wz_result_t wz_host_machine_frame_execute(
    wz_headless_runner_t* runner, wz_master_tick_t frame_ticks,
    bool capture_audio, wz_host_machine_frame_output_fn output,
    void* output_context)
{
    wz_machine_t* machine;
    wz_master_tick_t start_tick;
    wz_byte_t initial_beeper_level;
    wz_ay_t initial_ay;
    const wz_ay_t* initial_ay_state = NULL;
    wz_result_t result;

    if (runner == NULL || runner->machine == NULL || frame_ticks == 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    machine = runner->machine;
    start_tick = machine->master_tick;
    initial_beeper_level = machine->beeper.level;
    if (capture_audio && output != NULL) {
        initial_ay = machine->ay;
        initial_ay_state = &initial_ay;
    }

    result = wz_headless_runner_execute(runner, frame_ticks);
    if (result != WZ_RESULT_OK) {
        return result;
    }
    if (output != NULL) {
        (void)output(output_context, start_tick, initial_beeper_level,
                     initial_ay_state);
    }
    return WZ_RESULT_OK;
}
