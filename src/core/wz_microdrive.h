/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_MICRODRIVE_H
#define WZ_CORE_WZ_MICRODRIVE_H

#include <stddef.h>

#include "core/wz_types.h"

#define WZ_MDR_SECTOR_SIZE 543u
#define WZ_MDR_MIN_SECTORS 9u
#define WZ_MDR_MAX_SECTORS 254u
#define WZ_MDR_HEADER_OFFSET 12u
#define WZ_MDR_HEADER_SIZE 15u
#define WZ_MDR_DATA_OFFSET (WZ_MDR_HEADER_OFFSET + WZ_MDR_HEADER_SIZE)
#define WZ_MDR_DATA_SIZE (WZ_MDR_SECTOR_SIZE - WZ_MDR_DATA_OFFSET)

typedef struct {
    const wz_byte_t* data;
    size_t length;
    size_t sector_count;
    wz_qword_t identity;
} wz_mdr_image_t;

typedef enum {
    WZ_MDR_PHASE_HEADER = 0,
    WZ_MDR_PHASE_DATA = 1
} wz_mdr_phase_t;

typedef struct {
    const wz_mdr_image_t* image;
    wz_byte_t image_present;
    wz_qword_t image_identity;
    size_t image_length;
    size_t image_sector_count;
    size_t sector;
    size_t offset;
    wz_byte_t active_motor;
    wz_byte_t write_enabled;
    wz_byte_t erase_enabled;
    wz_byte_t dirty;
    wz_byte_t buffer[WZ_MDR_SECTOR_SIZE];
    wz_mdr_phase_t phase;
} wz_mdr_transport_t;

typedef wz_result_t (*wz_mdr_flush_callback_t)(size_t sector,
                                               const wz_byte_t* data,
                                               size_t length,
                                               void* context);

wz_result_t wz_mdr_image_init(wz_mdr_image_t* image,
                              const wz_byte_t* data,
                              size_t length);
wz_result_t wz_mdr_image_sector(const wz_mdr_image_t* image,
                                size_t sector,
                                const wz_byte_t** data);

void wz_mdr_transport_init(wz_mdr_transport_t* transport);
wz_result_t wz_mdr_transport_mount(wz_mdr_transport_t* transport,
                                    const wz_mdr_image_t* image);
wz_result_t wz_mdr_transport_select_motor(wz_mdr_transport_t* transport,
                                           wz_byte_t motor);
wz_result_t wz_mdr_transport_set_write_mode(wz_mdr_transport_t* transport,
                                             wz_byte_t enabled);
wz_result_t wz_mdr_transport_set_erase(wz_mdr_transport_t* transport,
                                       wz_byte_t enabled);
wz_result_t wz_mdr_transport_read(wz_mdr_transport_t* transport,
                                  wz_byte_t* value);
wz_result_t wz_mdr_transport_write(wz_mdr_transport_t* transport,
                                   wz_byte_t value);
wz_byte_t wz_mdr_transport_is_dirty(const wz_mdr_transport_t* transport);
wz_result_t wz_mdr_transport_flush(wz_mdr_transport_t* transport,
                                    wz_mdr_flush_callback_t callback,
                                    void* context);
void wz_mdr_transport_discard(wz_mdr_transport_t* transport);

#endif
