/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <assert.h>
#include <string.h>

#include "app/wz_host_media_ownership.h"

int main(void)
{
    wz_host_media_claim_t claim;
    wz_host_media_claim_outcome_t outcome;

    outcome = wz_host_media_claim_acquire_with_fallback(0, true, false, &claim);
    assert(outcome == WZ_HOST_MEDIA_CLAIM_INVALID_ARGUMENT);
    assert(strcmp(claim.reason, "invalid-argument") == 0);

    outcome = wz_host_media_claim_acquire_with_fallback(
        "missing-wzsn-media-image", true, true, &claim);
    assert(outcome == WZ_HOST_MEDIA_CLAIM_READ_ONLY);
    assert(wz_host_media_claim_outcome(&claim) == WZ_HOST_MEDIA_CLAIM_READ_ONLY);
    assert(strcmp(claim.reason, "read-only-fallback") == 0);
    wz_host_media_claim_release(&claim);
    return 0;
}
