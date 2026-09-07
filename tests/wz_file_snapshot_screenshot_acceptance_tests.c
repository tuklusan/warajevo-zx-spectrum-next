/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "app/wz_file_open_run.h"
#include "app/wz_screenshot_save_workflow.h"
#include "app/wz_snapshot_workflow.h"
#include "core/wz_machine.h"
#include "core/wz_machine_profile.h"

typedef struct {
    unsigned calls;
} dispatch_probe_t;

static bool record_dispatch(const char* path, void* context)
{
    dispatch_probe_t* probe = (dispatch_probe_t*)context;
    (void)path;
    ++probe->calls;
    return true;
}

int main(void)
{
    dispatch_probe_t probe = {0u};
    wz_open_run_handlers_t handlers = {
        record_dispatch, record_dispatch, record_dispatch, record_dispatch, &probe
    };
    wz_snapshot_workflow_t snapshot_workflow;
    wz_byte_t snapshot[WZ_STATE_MACHINE_LENGTH];
    wz_byte_t invalid_snapshot[WZ_STATE_MACHINE_LENGTH];
    wz_machine_t machine;
    wz_state_writer_t writer;
    wz_byte_t samples[4u] = {
        WZ_PALETTE_BLACK, WZ_PALETTE_WHITE,
        WZ_RASTER_BORDER_MIN, WZ_RASTER_BLANKING
    };
    wz_byte_t original_samples[4u];
    wz_byte_t png[512u];
    wz_raster_buffer_t raster;
    wz_screenshot_save_workflow_t screenshot_workflow;
    size_t required;
    size_t written = 0u;

    if (wz_file_open_run_route("game.TZX", &(wz_open_run_route_t){0}) != WZ_OPEN_RUN_OK ||
        wz_file_open_run_route("state.Z80", &(wz_open_run_route_t){0}) != WZ_OPEN_RUN_OK ||
        wz_file_open_run_route("drive.MDR", &(wz_open_run_route_t){0}) != WZ_OPEN_RUN_OK ||
        wz_file_open_run_route("legacy.VOC", &(wz_open_run_route_t){0}) != WZ_OPEN_RUN_OK ||
        wz_file_open_run_route("unknown.bin", &(wz_open_run_route_t){0}) !=
            WZ_OPEN_RUN_UNSUPPORTED_FORMAT ||
        wz_file_open_run_dispatch("game.TAP", &handlers) != WZ_OPEN_RUN_OK ||
        wz_file_open_run_dispatch("state.SNA", &handlers) != WZ_OPEN_RUN_OK ||
        wz_file_open_run_dispatch("drive.MDR", &handlers) != WZ_OPEN_RUN_OK ||
        probe.calls != 3u) {
        fputs("file routing acceptance failed\n", stderr);
        return 1;
    }

    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) != WZ_RESULT_OK) {
        return 1;
    }
    wz_state_writer_init(&writer, snapshot, sizeof(snapshot));
    if (wz_state_serialize_machine(&machine, &writer) != WZ_RESULT_OK) {
        fputs("snapshot serialization acceptance failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    snapshot[0u] = 13u;
    memset(invalid_snapshot, 0, sizeof(invalid_snapshot));
    wz_snapshot_workflow_init(&snapshot_workflow);
    if (wz_snapshot_workflow_load(&snapshot_workflow, snapshot, writer.length) !=
            WZ_SNAPSHOT_WORKFLOW_OK ||
        wz_snapshot_workflow_commit(&snapshot_workflow) != WZ_SNAPSHOT_WORKFLOW_OK ||
        wz_snapshot_workflow_load(&snapshot_workflow, invalid_snapshot,
                                  sizeof(invalid_snapshot)) !=
            WZ_SNAPSHOT_WORKFLOW_INVALID_SNAPSHOT ||
        wz_snapshot_workflow_current(&snapshot_workflow)->data[0u] != 13u) {
        fputs("snapshot workflow acceptance failed\n", stderr);
        wz_machine_destroy(&machine);
        return 1;
    }
    wz_machine_destroy(&machine);

    if (wz_raster_buffer_init(&raster, 2u, 2u, samples, sizeof(samples)) != WZ_RESULT_OK) {
        fputs("raster initialization acceptance failed\n", stderr);
        return 1;
    }
    samples[0] = WZ_PALETTE_BLACK;
    samples[1] = WZ_PALETTE_WHITE;
    samples[2] = WZ_RASTER_BORDER_MIN;
    samples[3] = WZ_RASTER_BLANKING;
    memcpy(original_samples, samples, sizeof(samples));
    required = wz_screenshot_png_required_size(&raster);
    wz_screenshot_save_workflow_init(&screenshot_workflow);
    if (wz_screenshot_save_as(&screenshot_workflow, 0, &raster, png, sizeof(png),
                              &written) != WZ_SCREENSHOT_SAVE_CANCELLED ||
        wz_screenshot_save_as(&screenshot_workflow, "screen.png", &raster, png,
                              required - 1u, &written) !=
            WZ_SCREENSHOT_SAVE_BUFFER_TOO_SMALL ||
        wz_screenshot_save_as(&screenshot_workflow, "screen.png", &raster, png,
                              sizeof(png), &written) != WZ_SCREENSHOT_SAVE_OK ||
        written != required ||
        memcmp(samples, original_samples, sizeof(samples)) != 0 ||
        memcmp(png, "\x89PNG\r\n\x1a\n", 8u) != 0) {
        fprintf(stderr, "screenshot acceptance failed: required=%lu written=%lu destination=%s\n",
                (unsigned long)required, (unsigned long)written,
                wz_screenshot_save_current_destination(&screenshot_workflow) != 0 ?
                    wz_screenshot_save_current_destination(&screenshot_workflow) : "<none>");
        return 1;
    }
    return 0;
}
