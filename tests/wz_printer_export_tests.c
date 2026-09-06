/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include "app/wz_printer_export.h"

int main(void)
{
    wz_printer_flush_event_t event = {0};
    wz_printer_bitmap_t bitmap;
    wz_byte_t output[4096];
    event.mode = WZ_PRINTER_MODE_EPSON;
    event.length = 2u;
    event.data[0] = 0x81u;
    event.data[1] = 0x24u;
    if (wz_printer_export_bitmap(&event, output, sizeof(output), &bitmap) != WZ_RESULT_OK ||
        bitmap.width != 2u || bitmap.height != 8u || bitmap.stride != 2u ||
        bitmap.bytes != 16u || output[0] != 0xffu || output[1] != 0u ||
        output[2] != 0u || output[3] != 0u || output[5] != 0xffu ||
        output[14] != 0xffu || output[15] != 0u) return 1;
    event.mode = WZ_PRINTER_MODE_HP;
    event.length = 1u;
    event.data[0] = 0xa0u;
    if (wz_printer_export_bitmap(&event, output, sizeof(output), &bitmap) != WZ_RESULT_OK ||
        bitmap.width != 8u || bitmap.height != 1u || output[0] != 0xffu ||
        output[1] != 0u || output[2] != 0xffu || output[7] != 0u) return 1;
    if (wz_printer_export_bitmap(&event, output, 7u, &bitmap) !=
        WZ_RESULT_BUFFER_TOO_SMALL) return 1;
    event.mode = WZ_PRINTER_MODE_NONE;
    if (wz_printer_export_bitmap(&event, output, sizeof(output), &bitmap) !=
        WZ_RESULT_INVALID_ARGUMENT) return 1;
    return 0;
}
