/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#ifndef WZ_CORE_WZ_STATE_H
#define WZ_CORE_WZ_STATE_H

#include <stddef.h>

#include "core/wz_types.h"

typedef struct wz_machine wz_machine_t;

#define WZ_STATE_SNAPSHOT_CAPACITY (65536u + 73u)
#define WZ_SNA_48K_LENGTH (27u + 49152u)
#define WZ_SNA_128K_PAGE_SIZE 16384u
#define WZ_SNA_128K_PAGE_COUNT 8u
#define WZ_SNA_128K_LENGTH (27u + 49152u + 4u + (5u * WZ_SNA_128K_PAGE_SIZE))
#define WZ_Z80_V1_HEADER_LENGTH 30u
#define WZ_Z80_V1_MEMORY_LENGTH 49152u

typedef enum {
    WZ_HISTORICAL_FORMAT_SNA = 0,
    WZ_HISTORICAL_FORMAT_Z80
} wz_historical_state_format_t;

typedef struct {
    wz_byte_t* data;
    size_t capacity;
    size_t length;
} wz_state_writer_t;

typedef struct {
    wz_byte_t data[WZ_STATE_SNAPSHOT_CAPACITY];
    size_t length;
} wz_snapshot_state_t;

/* Isolated historical image; live 128K paging remains a Phase-9 concern. */
typedef struct {
    wz_byte_t header[27u];
    wz_word_t program_counter;
    wz_byte_t paging_7ffd;
    wz_byte_t trdos_active;
    wz_byte_t page_present[WZ_SNA_128K_PAGE_COUNT];
    wz_byte_t pages[WZ_SNA_128K_PAGE_COUNT][WZ_SNA_128K_PAGE_SIZE];
} wz_sna_128k_image_t;

void wz_state_writer_init(wz_state_writer_t* writer,
                          wz_byte_t* data,
                          size_t capacity);
wz_result_t wz_state_serialize_machine(const wz_machine_t* machine,
                                       wz_state_writer_t* writer);
wz_result_t wz_state_deserialize_machine(wz_machine_t* machine,
                                         const wz_byte_t* data,
                                         size_t length);
wz_result_t wz_state_hash_machine(const wz_machine_t* machine,
                                  wz_qword_t* hash);
wz_result_t wz_state_validate_historical_representability(
    const wz_machine_t* machine,
    wz_historical_state_format_t format);
void wz_snapshot_state_init(wz_snapshot_state_t* snapshot);
wz_result_t wz_snapshot_state_capture(wz_snapshot_state_t* snapshot,
                                      const wz_machine_t* machine);
wz_result_t wz_snapshot_state_load(wz_snapshot_state_t* snapshot,
                                   const wz_byte_t* data,
                                   size_t length);
wz_result_t wz_snapshot_state_load_sna_48k(wz_snapshot_state_t* snapshot,
                                           const wz_byte_t* data,
                                           size_t length);
wz_result_t wz_state_save_sna_48k(const wz_machine_t* machine,
                                  wz_byte_t* data,
                                  size_t capacity);
wz_result_t wz_sna_128k_image_init(wz_sna_128k_image_t* image);
wz_result_t wz_sna_128k_image_load(wz_sna_128k_image_t* image,
                                    const wz_byte_t* data,
                                    size_t length);
wz_result_t wz_sna_128k_image_save(const wz_sna_128k_image_t* image,
                                    wz_byte_t* data,
                                    size_t capacity);
wz_result_t wz_snapshot_state_load_z80_v1(wz_snapshot_state_t* snapshot,
                                           const wz_byte_t* data,
                                           size_t length);
const wz_byte_t* wz_snapshot_state_data(const wz_snapshot_state_t* snapshot);
size_t wz_snapshot_state_length(const wz_snapshot_state_t* snapshot);

#endif
