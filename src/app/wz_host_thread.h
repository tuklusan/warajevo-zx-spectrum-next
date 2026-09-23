/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#ifndef WZ_APP_WZ_HOST_THREAD_H
#define WZ_APP_WZ_HOST_THREAD_H

#include <stdbool.h>

#if defined(_WIN32)
typedef unsigned long wz_host_thread_id_t;
#else
#include <pthread.h>
typedef pthread_t wz_host_thread_id_t;
#endif

wz_host_thread_id_t wz_host_thread_current_id(void);
bool wz_host_thread_id_equal(wz_host_thread_id_t left,
                             wz_host_thread_id_t right);

#endif
