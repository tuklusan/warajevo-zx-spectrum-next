/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "diagnostics/wz_syslog_udp.h"
#include "diagnostics/wz_trace_file.h"
#include <stdio.h>
#include <string.h>

static void count_appends(const wz_trace_event_t* event, void* context)
{
    (void)event;
    (*(unsigned*)context)++;
}

int main(void)
{
    char packet[WZ_SYSLOG_UDP_MAX_PACKET];
    wz_trace_event_t event = {0};
    wz_syslog_udp_t disabled;
    wz_trace_file_t trace;
    unsigned appends = 0u;
    const char* trace_path = "wz-syslog-trace-test.bin";
    size_t length;
    event.kind = WZ_TRACE_CPU_STATE_SYNC;
    event.sequence = 42u;
    event.master_tick = 1234u;
    event.address = 0x5678u;
    event.register_snapshot = UINT64_C(0xdeadbeef);
    length = wz_syslog_udp_format_trace(packet, sizeof(packet), &event);
    if (length == 0u || strstr(packet, "<14>1 - wzsn - - -") == NULL ||
        strstr(packet, "seq=\"42\"") == NULL || strstr(packet, "tick=\"1234\"") == NULL ||
        strstr(packet, "deadbeef") != NULL || strstr(packet, "5678") != NULL) return 1;
    wz_syslog_udp_init(&disabled, false);
    if (wz_syslog_udp_open(&disabled) != WZ_RESULT_OK) return 1;
    wz_syslog_udp_emit(&event, &disabled);
    wz_syslog_udp_log(&disabled, WZ_SYSLOG_ERROR, "safe diagnostic\nmessage");
    wz_syslog_udp_close(&disabled);
    if (wz_trace_file_create(&trace, trace_path, 1u, 0u, 1u, UINT32_MAX) != WZ_RESULT_OK) return 1;
    wz_trace_file_set_forwarder(&trace, count_appends, &appends);
    wz_trace_file_emit(&event, &trace);
    wz_trace_file_close(&trace);
    remove(trace_path);
    if (appends != 1u) return 1;
    puts("wz_syslog_udp contract passed");
    return 0;
}
