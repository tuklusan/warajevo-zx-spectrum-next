/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. */

#include "app/wz_host_pacing.h"

#include <stdio.h>

typedef struct {
    unsigned calls;
    wz_qword_t last_duration;
} sleep_capture_t;

static bool capture_sleep(wz_qword_t nanoseconds, void* opaque)
{
    sleep_capture_t* capture = (sleep_capture_t*)opaque;
    if (capture == NULL) {
        return false;
    }
    ++capture->calls;
    capture->last_duration = nanoseconds;
    return true;
}

static bool verify_speed_ratios(void)
{
    static const struct {
        wz_speed_policy_t speed;
        wz_qword_t expected_nanoseconds;
    } cases[] = {
        {WZ_SPEED_25, UINT64_C(4000000000)},
        {WZ_SPEED_50, UINT64_C(2000000000)},
        {WZ_SPEED_100, UINT64_C(1000000000)},
        {WZ_SPEED_200, UINT64_C(500000000)},
        {WZ_SPEED_400, UINT64_C(250000000)},
        {WZ_SPEED_800, UINT64_C(125000000)},
        {WZ_SPEED_UNLIMITED, 0u},
    };

    for (size_t index = 0u; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        wz_host_pacing_t pacing;
        sleep_capture_t capture = {0u, 0u};
        wz_qword_t requested = UINT64_MAX;
        if (!wz_host_pacing_init(&pacing, UINT64_C(1000000), cases[index].speed,
                                 UINT64_C(50000000), 100u) ||
            !wz_host_pacing_wait(&pacing, UINT64_C(50000000),
                                 UINT64_C(1000100), capture_sleep, &capture,
                                 &requested) ||
            requested != cases[index].expected_nanoseconds ||
            capture.calls != (cases[index].speed == WZ_SPEED_UNLIMITED ? 0u : 1u) ||
            (capture.calls != 0u && capture.last_duration != requested)) {
            fprintf(stderr, "incorrect host pacing ratio for speed index %u\n",
                    (unsigned)cases[index].speed);
            return false;
        }
    }
    return true;
}

static bool verify_speed_change_preserves_tick_anchor(void)
{
    wz_host_pacing_t pacing;
    sleep_capture_t capture = {0u, 0u};
    wz_qword_t requested = UINT64_MAX;

    if (!wz_host_pacing_init(&pacing, UINT64_C(1000000), WZ_SPEED_100,
                             UINT64_C(1000000000), 500u) ||
        !wz_host_pacing_set_speed(&pacing, WZ_SPEED_200) ||
        !wz_host_pacing_wait(&pacing, UINT64_C(1200000000), 250500u,
                             capture_sleep, &capture, &requested) ||
        pacing.speed != WZ_SPEED_200 || pacing.speed_change_pending ||
        pacing.anchor_machine_tick != 250500u ||
        pacing.anchor_host_nanoseconds != UINT64_C(1200000000) ||
        requested != 0u || capture.calls != 0u) {
        fputs("speed change did not re-anchor pacing at the current machine tick\n",
              stderr);
        return false;
    }
    if (!wz_host_pacing_wait(&pacing, UINT64_C(1220000000), 450500u,
                             capture_sleep, &capture, &requested) ||
        requested != UINT64_C(80000000) || capture.calls != 1u ||
        capture.last_duration != UINT64_C(80000000) ||
        pacing.anchor_machine_tick != 250500u) {
        fputs("post-change pacing ratio lost or reset the machine tick anchor\n",
              stderr);
        return false;
    }
    return true;
}

static bool verify_clock_regression_reanchors(void)
{
    wz_host_pacing_t pacing;
    wz_qword_t requested = UINT64_MAX;
    if (!wz_host_pacing_init(&pacing, UINT64_C(1000000), WZ_SPEED_100,
                             UINT64_C(1000000000), 1000u) ||
        !wz_host_pacing_wait(&pacing, UINT64_C(900000000), 900u, NULL, NULL,
                             &requested) ||
        requested != 0u || pacing.anchor_machine_tick != 900u ||
        pacing.anchor_host_nanoseconds != UINT64_C(900000000)) {
        fputs("host clock regression did not safely re-anchor pacing\n", stderr);
        return false;
    }
    return true;
}

int main(void)
{
    if (!verify_speed_ratios() || !verify_speed_change_preserves_tick_anchor() ||
        !verify_clock_regression_reanchors()) {
        return 1;
    }
    puts("PASS host_pacing_speed_ratios_and_tick_continuity");
    return 0;
}
