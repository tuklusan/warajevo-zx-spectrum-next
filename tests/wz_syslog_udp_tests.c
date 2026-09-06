/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "diagnostics/wz_syslog_udp.h"
#include <stdio.h>
#include <string.h>

int main(void)
{
    char packet[WZ_SYSLOG_UDP_MAX_PACKET];
    wz_trace_event_t event = {0};
    wz_syslog_udp_t disabled;
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
    puts("wz_syslog_udp contract passed");
    return 0;
}
