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
} wz_host_media_claim_t;

/* A read-only claim is explicit; a writable claim is exclusive across processes. */
bool wz_host_media_claim_acquire(const char* path,
                                 bool writable,
                                 wz_host_media_claim_t* claim);

void wz_host_media_claim_release(wz_host_media_claim_t* claim);

#endif
