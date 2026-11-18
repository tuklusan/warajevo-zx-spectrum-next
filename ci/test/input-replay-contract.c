/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is licensed under the GNU General Public License, version 2 or later.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_input_arbiter.h"
#include "app/wz_input_projection.h"
#include "app/wz_input_replay.h"
#include "core/wz_machine.h"
#include "core/wz_keyboard_matrix.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define REQUIRE(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "input replay contract failed at line %d: %s\n", \
                __LINE__, #expression); \
        return 1; \
    } \
} while (0)

typedef struct {
    wz_normalized_input_transition_t entries[16];
    size_t count;
} replay_capture_t;

static bool capture_transition(
    const wz_normalized_input_transition_t* transition,
    void* context)
{
    replay_capture_t* capture = (replay_capture_t*)context;
    if (capture == 0 || transition == 0 ||
        capture->count >= sizeof(capture->entries) /
            sizeof(capture->entries[0])) {
        return false;
    }
    capture->entries[capture->count++] = *transition;
    return true;
}

static bool apply_transition(
    const wz_normalized_input_transition_t* transition,
    void* context)
{
    wz_input_arbiter_t* arbiter = (wz_input_arbiter_t*)context;
    return transition != 0 && arbiter != 0 &&
        wz_input_arbiter_record_effective(
            arbiter, transition->key, transition->pressed != 0u,
            transition->master_tick);
}

static bool traces_equal(const wz_input_trace_t* left,
                         const wz_input_trace_t* right)
{
    size_t index;
    if (left == 0 || right == 0 || left->count != right->count) {
        return false;
    }
    for (index = 0u; index < left->count; ++index) {
        const wz_normalized_input_transition_t* left_entry =
            &left->entries[index];
        const wz_normalized_input_transition_t* right_entry =
            &right->entries[index];
        if (left_entry->key != right_entry->key ||
            left_entry->pressed != right_entry->pressed ||
            left_entry->master_tick != right_entry->master_tick ||
            left_entry->sequence != right_entry->sequence) {
            return false;
        }
    }
    return true;
}

static int exercise_trace_api(void)
{
    wz_normalized_input_transition_t entries[8];
    wz_input_trace_t trace;
    wz_input_trace_state_t state;
    replay_capture_t capture = {0};
    wz_input_arbiter_t replay_arbiter;

    wz_input_trace_init(&trace, entries, sizeof(entries) / sizeof(entries[0]));
    wz_input_trace_state_init(&state);
    REQUIRE(wz_input_trace_record_effective(&trace, &state, WZ_KEY_A,
                                            true, 100u));
    REQUIRE(!wz_input_trace_record_effective(&trace, &state, WZ_KEY_A,
                                             true, 100u));
    REQUIRE(wz_input_trace_record_effective(&trace, &state, WZ_KEY_A,
                                            false, 100u));
    REQUIRE(wz_input_trace_record_effective(&trace, &state, WZ_KEY_SHIFT,
                                            true, 120u));
    REQUIRE(trace.count == 3u);
    REQUIRE(trace.entries[0].sequence == 0u &&
            trace.entries[0].key == WZ_KEY_A &&
            trace.entries[0].pressed == 1u &&
            trace.entries[0].master_tick == 100u);
    REQUIRE(trace.entries[1].sequence == 1u &&
            trace.entries[1].pressed == 0u &&
            trace.entries[1].master_tick == 100u);
    REQUIRE(trace.entries[2].sequence == 2u &&
            trace.entries[2].key == WZ_KEY_SHIFT &&
            trace.entries[2].master_tick == 120u);
    REQUIRE(wz_input_trace_replay(&trace, capture_transition, &capture));
    REQUIRE(capture.count == trace.count &&
            memcmp(capture.entries, trace.entries,
                   trace.count * sizeof(trace.entries[0])) == 0);

    wz_input_arbiter_init(&replay_arbiter);
    REQUIRE(wz_input_trace_replay(&trace, apply_transition,
                                  &replay_arbiter));
    REQUIRE(traces_equal(&trace, wz_input_arbiter_trace(&replay_arbiter)));

    trace.entries[1].sequence = 9u;
    REQUIRE(!wz_input_trace_replay(&trace, capture_transition, &capture));
    trace.entries[1].sequence = 1u;
    trace.entries[0].key = WZ_INPUT_TRACE_KEY_COUNT;
    REQUIRE(!wz_input_trace_replay(&trace, capture_transition, &capture));
    trace.entries[0].key = WZ_KEY_A;
    return 0;
}

static int exercise_projection_equivalence(void)
{
    wz_input_arbiter_t local;
    wz_input_arbiter_t remote;
    wz_machine_t local_machine;
    wz_machine_t remote_machine;

    wz_input_arbiter_init(&local);
    wz_input_arbiter_init(&remote);
    REQUIRE(wz_machine_init(&local_machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);
    REQUIRE(wz_machine_init(&remote_machine, wz_machine_profile_48k_pal()) ==
            WZ_RESULT_OK);

    /* Resolve two-source ownership before recording the effective matrix. */
    REQUIRE(wz_input_arbiter_set(&local, WZ_INPUT_SOURCE_LOCAL,
                                 WZ_KEY_SHIFT, true));
    REQUIRE(wz_input_arbiter_set(&local, WZ_INPUT_SOURCE_TELNET,
                                 WZ_KEY_SHIFT, true));
    REQUIRE(wz_input_arbiter_set(&remote, WZ_INPUT_SOURCE_TELNET,
                                 WZ_KEY_SHIFT, true));
    local_machine.master_tick = 100u;
    remote_machine.master_tick = 100u;
    REQUIRE(wz_input_projection_apply_keyboard(&local, &local_machine));
    REQUIRE(wz_input_projection_apply_keyboard(&remote, &remote_machine));
    REQUIRE(traces_equal(wz_input_arbiter_trace(&local),
                          wz_input_arbiter_trace(&remote)));

    /* Releasing one owner leaves the effective key down in both traces. */
    REQUIRE(wz_input_arbiter_set(&local, WZ_INPUT_SOURCE_LOCAL,
                                 WZ_KEY_SHIFT, false));
    local_machine.master_tick = 120u;
    remote_machine.master_tick = 120u;
    REQUIRE(wz_input_projection_apply_keyboard(&local, &local_machine));
    REQUIRE(wz_input_projection_apply_keyboard(&remote, &remote_machine));
    REQUIRE(traces_equal(wz_input_arbiter_trace(&local),
                          wz_input_arbiter_trace(&remote)));
    REQUIRE(wz_input_arbiter_key_down(&local, WZ_KEY_SHIFT));

    REQUIRE(wz_input_arbiter_release_source(&local, WZ_INPUT_SOURCE_TELNET));
    REQUIRE(wz_input_arbiter_release_source(&remote, WZ_INPUT_SOURCE_TELNET));
    local_machine.master_tick = 140u;
    remote_machine.master_tick = 140u;
    REQUIRE(wz_input_projection_apply_keyboard(&local, &local_machine));
    REQUIRE(wz_input_projection_apply_keyboard(&remote, &remote_machine));
    REQUIRE(traces_equal(wz_input_arbiter_trace(&local),
                          wz_input_arbiter_trace(&remote)));
    REQUIRE(!wz_input_arbiter_key_down(&local, WZ_KEY_SHIFT));

    /* A reset-time tick rollback starts a new deterministic trace segment. */
    REQUIRE(wz_input_arbiter_set(&local, WZ_INPUT_SOURCE_LOCAL,
                                 WZ_KEY_A, true));
    local_machine.master_tick = 200u;
    REQUIRE(wz_input_projection_apply_keyboard(&local, &local_machine));
    REQUIRE(wz_input_arbiter_trace(&local)->count == 3u);
    REQUIRE(wz_input_arbiter_set(&local, WZ_INPUT_SOURCE_LOCAL,
                                 WZ_KEY_A, false));
    local_machine.master_tick = 0u;
    REQUIRE(wz_input_projection_apply_keyboard(&local, &local_machine));

    /* The direct API must reset the trace on a backwards master-tick jump. */
    wz_input_arbiter_t reset_arbiter;
    wz_input_arbiter_init(&reset_arbiter);
    REQUIRE(wz_input_arbiter_record_effective(
        &reset_arbiter, WZ_KEY_A, true, 200u));
    REQUIRE(wz_input_arbiter_record_effective(
        &reset_arbiter, WZ_KEY_B, true, 0u));
    REQUIRE(wz_input_arbiter_trace(&reset_arbiter)->count == 1u);
    REQUIRE(wz_input_arbiter_trace(&reset_arbiter)->entries[0].sequence == 0u);
    REQUIRE(wz_input_arbiter_trace(&reset_arbiter)->entries[0].master_tick == 0u);

    wz_machine_destroy(&remote_machine);
    wz_machine_destroy(&local_machine);
    return 0;
}

int main(void)
{
    REQUIRE(exercise_trace_api() == 0);
    REQUIRE(exercise_projection_equivalence() == 0);
    puts("PASS normalized input trace records effective matrix transitions and replays headlessly");
    return 0;
}
