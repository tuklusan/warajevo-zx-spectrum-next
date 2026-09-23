/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_host_thread.h"

#if defined(_WIN32)
#include <windows.h>
#endif

wz_host_thread_id_t wz_host_thread_current_id(void)
{
#if defined(_WIN32)
    return (wz_host_thread_id_t)GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

bool wz_host_thread_id_equal(wz_host_thread_id_t left,
                             wz_host_thread_id_t right)
{
#if defined(_WIN32)
    return left == right;
#else
    return pthread_equal(left, right) != 0;
#endif
}
