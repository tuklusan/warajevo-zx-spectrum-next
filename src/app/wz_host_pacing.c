/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_host_pacing.h"

#include <stdint.h>

static wz_qword_t wz_saturating_multiply(wz_qword_t left, wz_qword_t right)
{
    if (left != 0u && right > UINT64_MAX / left) {
        return UINT64_MAX;
    }
    return left * right;
}

static wz_qword_t wz_fractional_nanoseconds(wz_qword_t numerator,
                                             wz_qword_t denominator)
{
    wz_qword_t quotient = 0u;
    wz_qword_t remainder = 0u;
    const wz_qword_t multiplier = UINT64_C(1000000000);

    for (int bit = 30; bit >= 0; --bit) {
        wz_qword_t doubled;
        unsigned carry;

        if (remainder >= denominator - remainder) {
            doubled = remainder - (denominator - remainder);
            carry = 1u;
        } else {
            doubled = remainder + remainder;
            carry = 0u;
        }
        if (((multiplier >> bit) & 1u) != 0u) {
            if (doubled >= denominator - numerator) {
                doubled -= denominator - numerator;
                carry += 1u;
            } else {
                doubled += numerator;
            }
        }
        quotient = quotient * 2u + (wz_qword_t)carry;
        remainder = doubled;
    }
    return quotient;
}

static wz_qword_t wz_elapsed_nanoseconds(wz_qword_t ticks,
                                         wz_qword_t ticks_per_second)
{
    wz_qword_t whole_seconds = ticks / ticks_per_second;
    wz_qword_t remainder = ticks % ticks_per_second;
    wz_qword_t result;
    wz_qword_t fractional;

    if (whole_seconds > UINT64_MAX / UINT64_C(1000000000)) {
        return UINT64_MAX;
    }
    result = whole_seconds * UINT64_C(1000000000);
    fractional = wz_fractional_nanoseconds(remainder, ticks_per_second);
    if (UINT64_MAX - result < fractional) {
        return UINT64_MAX;
    }
    return result + fractional;
}

bool wz_host_pacing_init(wz_host_pacing_t* pacing,
                         wz_qword_t master_ticks_per_second,
                         wz_speed_policy_t speed,
                         wz_qword_t host_nanoseconds,
                         wz_master_tick_t machine_tick)
{
    if (pacing == 0 || master_ticks_per_second == 0u ||
        !wz_speed_policy_valid(speed)) {
        return false;
    }
    pacing->anchor_machine_tick = machine_tick;
    pacing->anchor_host_nanoseconds = host_nanoseconds;
    pacing->master_ticks_per_second = master_ticks_per_second;
    pacing->speed = speed;
    pacing->pending_speed = speed;
    pacing->speed_change_pending = false;
    pacing->anchored = true;
    return true;
}

bool wz_host_pacing_set_speed(wz_host_pacing_t* pacing,
                              wz_speed_policy_t speed)
{
    if (pacing == 0 || !wz_speed_policy_valid(speed)) {
        return false;
    }
    pacing->pending_speed = speed;
    pacing->speed_change_pending = true;
    return true;
}

bool wz_host_pacing_wait(wz_host_pacing_t* pacing,
                         wz_qword_t host_nanoseconds,
                         wz_master_tick_t machine_tick,
                         wz_pacing_sleep_fn sleep,
                         void* context,
                         wz_qword_t* requested_sleep_nanoseconds)
{
    wz_qword_t elapsed_ticks;
    wz_qword_t target_nanoseconds;
    wz_qword_t host_elapsed;
    wz_qword_t delay;
    unsigned percent;

    if (pacing == 0 || !pacing->anchored || requested_sleep_nanoseconds == 0) {
        return false;
    }
    *requested_sleep_nanoseconds = 0u;
    if (pacing->speed_change_pending) {
        pacing->speed = pacing->pending_speed;
        pacing->anchor_machine_tick = machine_tick;
        pacing->anchor_host_nanoseconds = host_nanoseconds;
        pacing->speed_change_pending = false;
        return true;
    }
    if (wz_speed_policy_is_unlimited(pacing->speed)) {
        return true;
    }
    percent = wz_speed_policy_percent(pacing->speed);
    if (machine_tick < pacing->anchor_machine_tick ||
        host_nanoseconds < pacing->anchor_host_nanoseconds) {
        return true;
    }
    elapsed_ticks = machine_tick - pacing->anchor_machine_tick;
    target_nanoseconds = wz_elapsed_nanoseconds(elapsed_ticks,
                                                 pacing->master_ticks_per_second);
    if (percent != 100u) {
        target_nanoseconds = wz_saturating_multiply(target_nanoseconds, 100u);
        target_nanoseconds = target_nanoseconds == UINT64_MAX
                                 ? UINT64_MAX
                                 : target_nanoseconds / percent;
    }
    host_elapsed = host_nanoseconds - pacing->anchor_host_nanoseconds;
    if (host_elapsed >= target_nanoseconds) {
        return true;
    }
    delay = target_nanoseconds - host_elapsed;
    *requested_sleep_nanoseconds = delay;
    return sleep == 0 || sleep(delay, context);
}
