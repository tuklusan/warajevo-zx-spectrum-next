/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_APP_WZ_HOST_MEDIA_OWNERSHIP_H
#define WZ_APP_WZ_HOST_MEDIA_OWNERSHIP_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool held;
    bool writable;
    intptr_t native_handle;
    const char* reason;
} wz_host_media_claim_t;

typedef enum {
    WZ_HOST_MEDIA_CLAIM_WRITABLE = 0,
    WZ_HOST_MEDIA_CLAIM_READ_ONLY,
    WZ_HOST_MEDIA_CLAIM_INVALID_ARGUMENT,
    WZ_HOST_MEDIA_CLAIM_UNAVAILABLE
} wz_host_media_claim_outcome_t;

/* A read-only claim is explicit; a writable claim is exclusive across processes. */
bool wz_host_media_claim_acquire(const char* path,
                                 bool writable,
                                 wz_host_media_claim_t* claim);

/* Read-only fallback is explicit and visible in the returned outcome. */
wz_host_media_claim_outcome_t wz_host_media_claim_acquire_with_fallback(
    const char* path, bool writable, bool allow_read_only,
    wz_host_media_claim_t* claim);

wz_host_media_claim_outcome_t wz_host_media_claim_outcome(
    const wz_host_media_claim_t* claim);

void wz_host_media_claim_release(wz_host_media_claim_t* claim);

#endif
