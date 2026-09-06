/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "app/wz_command_registry.h"

static wz_result_t handler(
    const void* context,
    wz_command_arguments_t arguments,
    wz_command_result_t* result)
{
    const char* expected = (const char*)context;
    if (arguments.size != strlen(expected) ||
        memcmp(arguments.data, expected, arguments.size) != 0) {
        result->status = WZ_COMMAND_RESULT_FAILED;
        result->reason = "bad-arguments";
        return WZ_RESULT_PARSE_ERROR;
    }
    result->status = WZ_COMMAND_RESULT_SUCCESS;
    result->reason = 0;
    return WZ_RESULT_OK;
}

static bool unavailable(const void* context, const char** reason)
{
    (void)context;
    *reason = "requires-mounted-tape";
    return false;
}

static wz_command_metadata_t metadata(
    const char* id,
    wz_command_handler_fn callback,
    const void* context)
{
    wz_command_metadata_t value;
    memset(&value, 0, sizeof(value));
    value.id = id;
    value.label = "Test command";
    value.description = "Registry test command";
    value.menu_group = "test";
    value.parameter_schema = "bytes";
    value.result_schema = "status";
    value.handler_identity = "test.handler";
    value.parameter_acquisition = "test";
    value.permission = WZ_COMMAND_REMOTE_SAFE;
    value.handler = callback;
    value.handler_context = context;
    value.recordable = true;
    return value;
}

int main(void)
{
    wz_command_metadata_t storage[3];
    wz_command_registry_t registry;
    wz_command_result_t result;
    const wz_command_metadata_t* found;
    const char* payload = "ok";
    wz_command_metadata_t unavailable_command;

    if (wz_command_registry_init(&registry, storage, 3u) != WZ_RESULT_OK ||
        wz_command_registry_register(&registry, metadata("machine.reset",
                                                          handler, payload)) != WZ_RESULT_OK ||
        wz_command_registry_register(&registry, metadata("machine.reset",
                                                          handler, payload)) != WZ_RESULT_INVALID_ARGUMENT ||
        wz_command_registry_register(&registry, metadata("Machine.bad",
                                                          handler, payload)) != WZ_RESULT_INVALID_ARGUMENT) {
        fputs("command registry validation failed\n", stderr);
        return 1;
    }

    unavailable_command = metadata("media.tape.eject", handler, payload);
    unavailable_command.availability = unavailable;
    if (wz_command_registry_register(&registry, unavailable_command) != WZ_RESULT_OK ||
        wz_command_registry_finalize(&registry) != WZ_RESULT_OK ||
        wz_command_registry_count(&registry) != 2u ||
        wz_command_registry_register(&registry, metadata("help.about", handler,
                                                          payload)) != WZ_RESULT_INVALID_STATE) {
        fputs("command registry finalization failed\n", stderr);
        return 1;
    }

    found = wz_command_registry_find(&registry, "machine.reset");
    if (found == 0 || wz_command_registry_at(&registry, 1u) == 0 ||
        wz_command_registry_at(&registry, 2u) != 0 ||
        wz_command_registry_dispatch(&registry, "machine.reset",
                                     (wz_command_arguments_t){payload, 2u},
                                     &result) != WZ_RESULT_OK ||
        result.status != WZ_COMMAND_RESULT_SUCCESS ||
        wz_command_registry_dispatch(&registry, "missing.command",
                                     (wz_command_arguments_t){0, 0u},
                                     &result) != WZ_RESULT_NOT_FOUND ||
        result.status != WZ_COMMAND_RESULT_REJECTED ||
        wz_command_registry_dispatch(&registry, "media.tape.eject",
                                     (wz_command_arguments_t){0, 0u},
                                     &result) != WZ_RESULT_INVALID_STATE ||
        result.status != WZ_COMMAND_RESULT_UNAVAILABLE ||
        strcmp(result.reason, "requires-mounted-tape") != 0) {
        fputs("command registry dispatch failed\n", stderr);
        return 1;
    }

    puts("wz_command_registry contract passed");
    return 0;
}
