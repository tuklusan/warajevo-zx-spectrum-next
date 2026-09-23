/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "diagnostics/wz_diagnostics_router.h"

void wz_diagnostics_router_init(wz_diagnostics_router_t* router,
                                wz_trace_emit_fn local_emit,
                                void* local_context,
                                wz_syslog_udp_t* syslog,
                                bool forwarding_enabled)
{
    if (router == 0) return;
    router->local_emit = local_emit;
    router->local_context = local_context;
    router->syslog = syslog;
    router->forwarding_enabled = forwarding_enabled;
}

void wz_diagnostics_router_emit(const wz_trace_event_t* event, void* context)
{
    wz_diagnostics_router_t* router = (wz_diagnostics_router_t*)context;
    if (router == 0 || event == 0) return;
    if (router->local_emit != 0) {
        router->local_emit(event, router->local_context);
    }
    if (router->forwarding_enabled && router->syslog != 0) {
        wz_syslog_udp_emit(event, router->syslog);
    }
}

void wz_diagnostics_router_log(wz_diagnostics_router_t* router,
                               wz_syslog_severity_t severity,
                               const char* message)
{
    if (router == 0 || !router->forwarding_enabled || router->syslog == 0) return;
    wz_syslog_udp_log(router->syslog, severity, message);
}
