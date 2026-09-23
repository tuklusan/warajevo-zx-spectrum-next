/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_RASTER_EVIDENCE_H
#define WZ_CORE_WZ_RASTER_EVIDENCE_H

#include <stddef.h>

#include "core/wz_raster.h"
#include "core/wz_trace.h"

wz_result_t wz_raster_buffer_hash(const wz_raster_buffer_t* buffer,
                                  wz_qword_t* hash);
wz_result_t wz_trace_events_hash(const wz_trace_event_t* events,
                                 size_t count, wz_qword_t* hash);

#endif
