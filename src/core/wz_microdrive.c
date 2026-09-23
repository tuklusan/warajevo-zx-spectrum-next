/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "core/wz_microdrive.h"

#include <string.h>
#include <stdint.h>

static wz_qword_t wz_mdr_identity(const wz_byte_t* data, size_t length)
{
    wz_qword_t identity = UINT64_C(14695981039346656037);
    for (size_t index = 0u; index < length; ++index) {
        identity ^= (wz_qword_t)data[index];
        identity *= UINT64_C(1099511628211);
    }
    return identity;
}

wz_result_t wz_mdr_image_init(wz_mdr_image_t* image,
                              const wz_byte_t* data,
                              size_t length)
{
    size_t sector_count;

    if (image == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (length % WZ_MDR_SECTOR_SIZE != 0u) {
        return WZ_RESULT_PARSE_ERROR;
    }
    sector_count = length / WZ_MDR_SECTOR_SIZE;
    if (sector_count < WZ_MDR_MIN_SECTORS || sector_count > WZ_MDR_MAX_SECTORS) {
        return WZ_RESULT_PARSE_ERROR;
    }
    image->data = data;
    image->length = length;
    image->sector_count = sector_count;
    image->identity = wz_mdr_identity(data, length);
    return WZ_RESULT_OK;
}

wz_result_t wz_mdr_image_sector(const wz_mdr_image_t* image,
                                size_t sector,
                                const wz_byte_t** data)
{
    if (image == 0 || data == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (image->data == 0 || image->sector_count < WZ_MDR_MIN_SECTORS ||
        image->sector_count > WZ_MDR_MAX_SECTORS ||
        image->length != image->sector_count * WZ_MDR_SECTOR_SIZE ||
        sector >= image->sector_count) {
        return WZ_RESULT_INVALID_STATE;
    }
    *data = image->data + sector * WZ_MDR_SECTOR_SIZE;
    return WZ_RESULT_OK;
}

void wz_mdr_transport_init(wz_mdr_transport_t* transport)
{
    if (transport != 0) {
        transport->image = 0;
        transport->image_present = 0u;
        transport->image_identity = 0u;
        transport->image_length = 0u;
        transport->image_sector_count = 0u;
        transport->sector = 0u;
        transport->offset = 0u;
        transport->active_motor = 0xffu;
        transport->write_enabled = 0u;
        transport->erase_enabled = 0u;
        transport->dirty = 0u;
        memset(transport->buffer, 0, sizeof(transport->buffer));
        transport->phase = WZ_MDR_PHASE_HEADER;
    }
}

wz_result_t wz_mdr_transport_mount(wz_mdr_transport_t* transport,
                                    const wz_mdr_image_t* image)
{
    if (transport == 0 || image == 0 || image->data == 0 ||
        image->sector_count < WZ_MDR_MIN_SECTORS ||
        image->sector_count > WZ_MDR_MAX_SECTORS ||
        image->length != image->sector_count * WZ_MDR_SECTOR_SIZE) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    transport->image = image;
    transport->image_present = 1u;
    transport->image_identity = image->identity;
    transport->image_length = image->length;
    transport->image_sector_count = image->sector_count;
    transport->sector = 0u;
    transport->offset = WZ_MDR_HEADER_OFFSET;
    transport->active_motor = 0xffu;
    transport->write_enabled = 0u;
    transport->erase_enabled = 0u;
    transport->dirty = 0u;
    memset(transport->buffer, 0, sizeof(transport->buffer));
    transport->phase = WZ_MDR_PHASE_HEADER;
    return WZ_RESULT_OK;
}

wz_result_t wz_mdr_transport_select_motor(wz_mdr_transport_t* transport,
                                           wz_byte_t motor)
{
    if (transport == 0 || motor > 7u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    transport->active_motor = motor;
    return WZ_RESULT_OK;
}

wz_result_t wz_mdr_transport_set_write_mode(wz_mdr_transport_t* transport,
                                             wz_byte_t enabled)
{
    if (transport == 0 || enabled > 1u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    transport->write_enabled = enabled;
    return WZ_RESULT_OK;
}

wz_result_t wz_mdr_transport_set_erase(wz_mdr_transport_t* transport,
                                       wz_byte_t enabled)
{
    if (transport == 0 || enabled > 1u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    transport->erase_enabled = enabled;
    return WZ_RESULT_OK;
}

wz_result_t wz_mdr_transport_read(wz_mdr_transport_t* transport,
                                  wz_byte_t* value)
{
    const wz_byte_t* sector_data;
    size_t end;

    if (transport == 0 || value == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (transport->image == 0 || transport->active_motor > 7u ||
        wz_mdr_image_sector(transport->image, transport->sector,
                            &sector_data) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    end = transport->phase == WZ_MDR_PHASE_HEADER
        ? WZ_MDR_DATA_OFFSET
        : WZ_MDR_SECTOR_SIZE;
    if (transport->offset >= end) {
        if (transport->phase == WZ_MDR_PHASE_HEADER) {
            transport->phase = WZ_MDR_PHASE_DATA;
            transport->offset = WZ_MDR_DATA_OFFSET;
        } else {
            transport->sector = (transport->sector + 1u) %
                transport->image->sector_count;
            transport->phase = WZ_MDR_PHASE_HEADER;
            transport->offset = WZ_MDR_HEADER_OFFSET;
            if (wz_mdr_image_sector(transport->image, transport->sector,
                                    &sector_data) != WZ_RESULT_OK) {
                return WZ_RESULT_INVALID_STATE;
            }
        }
    }
    *value = sector_data[transport->offset++];
    return WZ_RESULT_OK;
}

wz_result_t wz_mdr_transport_write(wz_mdr_transport_t* transport,
                                   wz_byte_t value)
{
    const wz_byte_t* sector_data;

    if (transport == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (transport->image == 0 || transport->active_motor > 7u ||
        transport->write_enabled == 0u || transport->erase_enabled != 0u ||
        wz_mdr_image_sector(transport->image, transport->sector,
                            &sector_data) != WZ_RESULT_OK) {
        return WZ_RESULT_INVALID_STATE;
    }
    if (transport->dirty == 0u) {
        memcpy(transport->buffer, sector_data, WZ_MDR_SECTOR_SIZE);
        transport->dirty = 1u;
    }
    if (transport->offset >= WZ_MDR_SECTOR_SIZE) {
        transport->offset = WZ_MDR_SECTOR_SIZE;
        return WZ_RESULT_OK;
    }
    transport->buffer[transport->offset++] = value;
    return WZ_RESULT_OK;
}

wz_byte_t wz_mdr_transport_is_dirty(const wz_mdr_transport_t* transport)
{
    return transport == 0 ? 0u : transport->dirty;
}

wz_result_t wz_mdr_transport_flush(wz_mdr_transport_t* transport,
                                    wz_mdr_flush_callback_t callback,
                                    void* context)
{
    wz_result_t result;

    if (transport == 0 || callback == 0) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    if (transport->dirty == 0u) {
        return WZ_RESULT_OK;
    }
    result = callback(transport->sector, transport->buffer,
                      WZ_MDR_SECTOR_SIZE, context);
    if (result == WZ_RESULT_OK) {
        transport->dirty = 0u;
    }
    return result;
}

void wz_mdr_transport_discard(wz_mdr_transport_t* transport)
{
    if (transport != 0) {
        transport->dirty = 0u;
        transport->offset = WZ_MDR_HEADER_OFFSET;
        transport->phase = WZ_MDR_PHASE_HEADER;
    }
}
