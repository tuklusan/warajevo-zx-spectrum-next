/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_compatibility_tools.h"
#include "app/wz_file_open_run.h"

#include <stdio.h>
#include <string.h>

static bool available(const void* context, const char** reason)
{
    if (context == NULL) {
        if (reason != NULL) *reason = "application-unavailable";
        return false;
    }
    if (reason != NULL) *reason = NULL;
    return true;
}

static wz_result_t open_tools(const void* context,
    wz_command_arguments_t arguments, wz_command_result_t* result)
{
    wz_compatibility_tools_window_t* window =
        (wz_compatibility_tools_window_t*)context;
    if (window == NULL || arguments.size != 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    wz_compatibility_tools_window_open(window);
    if (result != NULL) result->status = WZ_COMMAND_RESULT_SUCCESS;
    return WZ_RESULT_OK;
}

int main(void)
{
    static const struct {
        const char* extension;
        wz_open_run_route_t open_run_route;
    } native_formats[] = {
        {"tap", WZ_OPEN_RUN_TAPE}, {"TZX", WZ_OPEN_RUN_TAPE},
        {"wav", WZ_OPEN_RUN_TAPE}, {"sna", WZ_OPEN_RUN_SNAPSHOT},
        {"Z80", WZ_OPEN_RUN_SNAPSHOT}, {"mdr", WZ_OPEN_RUN_MICRODRIVE}
    };
    static const char* conversion_formats[] = {
        "voc", "blk", "spc", "ltp", "zxs", "zxt", "slt", "sem",
        "sit", "snp", "scr", "dck", "trd"
    };
    static const struct {
        size_t index;
        const char* reason;
        const char* source;
        const char* destination;
    } deferred_tools[] = {
        {2u, "later-legacy-snapshot-converters-not-implemented",
         "legacy SP/VGASPEC/PRG/SNA/SEM/SNP/SLT/SIT/ZXS formats",
         "Z80/SNA/TAP"},
        {4u, "later-mdr-enlargement-not-implemented", "MDR",
         "254-sector MDR"},
        {5u, "low-priority-legacy-database-converters-not-implemented",
         "Warajevo/SpecPic/SGD/ZX Rainbow/SpecBase", "portable-data"}
    };
    wz_compatibility_tools_window_t window;
    wz_command_registry_t registry;
    wz_command_metadata_t storage[1];
    wz_command_result_t result;
    size_t index;

    wz_compatibility_tools_window_init(&window);
    if (wz_compatibility_tools_window_is_open(&window) ||
        wz_command_registry_init(&registry, storage, 1u) != WZ_RESULT_OK ||
        wz_command_registry_bind_owner_thread(&registry) != WZ_RESULT_OK ||
        wz_compatibility_tools_register_commands(&registry, available,
            open_tools, &window) != WZ_RESULT_OK ||
        wz_command_registry_finalize(&registry) != WZ_RESULT_OK) {
        return 1;
    }
    if (strcmp(wz_compatibility_tools_command_id(),
               WZ_COMPATIBILITY_TOOLS_COMMAND_ID) != 0 ||
        wz_compatibility_tools_count() != WZ_COMPATIBILITY_TOOL_COUNT ||
        wz_command_registry_state(&registry,
            WZ_COMPATIBILITY_TOOLS_COMMAND_ID, NULL) != WZ_COMMAND_ENABLED ||
        wz_command_registry_dispatch(&registry,
            WZ_COMPATIBILITY_TOOLS_COMMAND_ID,
            (wz_command_arguments_t){NULL, 0u}, &result) != WZ_RESULT_OK ||
        !wz_compatibility_tools_window_is_open(&window)) {
        return 2;
    }
    for (index = 1u; index < wz_compatibility_tools_count(); ++index) {
        const wz_compatibility_tool_t* tool =
            wz_compatibility_tools_at(index);
        const char* reason = NULL;
        const char* warning = NULL;
        if (tool == NULL || wz_compatibility_tools_is_available(index, &reason) ||
            tool->availability == WZ_COMPATIBILITY_AVAILABLE ||
            reason == NULL || reason[0] == '\0' ||
            !wz_compatibility_tools_loss_disclosure(index, &warning) ||
            warning == NULL || warning[0] == '\0' ||
            tool->warning == NULL ||
            strcmp(warning, tool->warning) != 0) {
            fputs("unavailable conversion lacks a visible loss disclosure\n",
                  stderr);
            return 3;
        }
    }
    for (index = 0u; index < sizeof(deferred_tools) /
            sizeof(deferred_tools[0]); ++index) {
        const wz_compatibility_tool_t* tool =
            wz_compatibility_tools_at(deferred_tools[index].index);
        const char* reason = NULL;
        if (tool == NULL ||
            tool->availability != WZ_COMPATIBILITY_LATER ||
            wz_compatibility_tools_is_available(
                deferred_tools[index].index, &reason) ||
            reason == NULL ||
            strcmp(reason, deferred_tools[index].reason) != 0 ||
            tool->source_format == NULL ||
            tool->destination_format == NULL ||
            tool->id == NULL || tool->label == NULL ||
            strcmp(tool->source_format, deferred_tools[index].source) != 0 ||
            strcmp(tool->destination_format,
                   deferred_tools[index].destination) != 0) {
            fputs("legacy conversion disposition changed unexpectedly\n",
                  stderr);
            return 8;
        }
    }
    for (index = 1u; index < wz_compatibility_tools_count(); ++index) {
        const wz_compatibility_tool_t* tool =
            wz_compatibility_tools_at(index);
        if (tool != NULL &&
            ((tool->id != NULL && strstr(tool->id, "dock") != NULL) ||
             (tool->label != NULL && strstr(tool->label, "Dock") != NULL))) {
            fputs("Dock conversion exposed before Timex/DCK support\n", stderr);
            return 9;
        }
    }
    for (index = 0u; index < sizeof(native_formats) /
            sizeof(native_formats[0]); ++index) {
        char path[48];
        wz_open_run_route_t route = WZ_OPEN_RUN_UNSUPPORTED;
        (void)snprintf(path, sizeof(path), "media/input.%s",
                       native_formats[index].extension);
        if (wz_compatibility_tools_route_for_format(
                native_formats[index].extension, NULL) !=
                WZ_FILE_ROUTE_NATIVE_LOAD ||
            wz_file_open_run_route(path, &route) != WZ_OPEN_RUN_OK ||
            route != native_formats[index].open_run_route) {
            fprintf(stderr, "native format was not routed for %s\n",
                    native_formats[index].extension);
            return 5;
        }
    }
    for (index = 0u; index < sizeof(conversion_formats) /
            sizeof(conversion_formats[0]); ++index) {
        char path[48];
        wz_open_run_route_t route = WZ_OPEN_RUN_UNSUPPORTED;
        (void)snprintf(path, sizeof(path), "media/input.%s",
                       conversion_formats[index]);
        if (wz_compatibility_tools_route_for_format(
                conversion_formats[index], NULL) !=
                WZ_FILE_ROUTE_EXPLICIT_CONVERSION ||
            wz_file_open_run_route(path, &route) != WZ_OPEN_RUN_OK ||
            route != WZ_OPEN_RUN_CONVERSION) {
            fprintf(stderr, "conversion format was not kept explicit for %s\n",
                    conversion_formats[index]);
            return 6;
        }
    }
    if (wz_compatibility_tools_route_for_format("BIN", NULL) !=
            WZ_FILE_ROUTE_UNSUPPORTED ||
        wz_file_open_run_route("media/input.bin",
            &(wz_open_run_route_t){WZ_OPEN_RUN_UNSUPPORTED}) !=
            WZ_OPEN_RUN_UNSUPPORTED_FORMAT) {
        fputs("unsupported formats were routed as native or conversion\n",
              stderr);
        return 7;
    }
    wz_compatibility_tools_window_close(&window);
    if (wz_compatibility_tools_window_is_open(&window)) return 4;
    puts("compatibility tools availability contract passed");
    return 0;
}
