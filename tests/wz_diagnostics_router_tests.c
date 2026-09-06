/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "core/wz_trace.h"
#include "diagnostics/wz_diagnostics_router.h"

typedef struct {
    unsigned count;
    wz_trace_event_t event;
} capture_t;

static void capture_event(const wz_trace_event_t* event, void* context)
{
    capture_t* capture = (capture_t*)context;
    if (capture != 0 && event != 0) {
        capture->event = *event;
        capture->count++;
    }
}

int main(void)
{
    capture_t capture = {0};
    wz_syslog_udp_t disabled;
    wz_diagnostics_router_t router;
    wz_trace_sink_t sink;
    wz_trace_event_t event = {0};

    wz_syslog_udp_init(&disabled, false);
    wz_diagnostics_router_init(&router, capture_event, &capture, &disabled, true);
    wz_trace_sink_init(&sink, wz_diagnostics_router_emit, &router);
    event.kind = WZ_TRACE_CPU_BUS;
    event.master_tick = 99u;
    event.address = 0x1234u;
    wz_trace_emit_detail(&sink, &event);
    wz_diagnostics_router_log(&router, WZ_SYSLOG_ERROR, "router diagnostic");
    if (capture.count != 1u || capture.event.sequence != 0u ||
        capture.event.master_tick != 99u || capture.event.address != 0x1234u) {
        fputs("diagnostics router fan-out failed\n", stderr);
        return 1;
    }

    router.forwarding_enabled = false;
    wz_trace_emit(&sink, WZ_TRACE_DEVELOPER_MARKER, 100u);
    wz_diagnostics_router_log(&router, WZ_SYSLOG_ERROR, "must remain local");
    if (capture.count != 2u || capture.event.sequence != 1u) {
        fputs("diagnostics router opt-out failed\n", stderr);
        return 1;
    }
    if (strstr(WZ_SYSLOG_UDP_HOST, "sanyalnet-oracle-vps2.duckdns.org") == 0 ||
        WZ_SYSLOG_UDP_PORT != 65514u) {
        fputs("diagnostics router destination contract failed\n", stderr);
        return 1;
    }
    puts("wz_diagnostics_router contract passed");
    return 0;
}
