/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_command_registry.h"
#include "app/wz_telnet_keyboard_command.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned calls;
} handler_state_t;

static wz_result_t command_handler(const void* context,
                                   wz_command_arguments_t arguments,
                                   wz_command_result_t* result)
{
    handler_state_t* state = (handler_state_t*)context;
    if (state == NULL || result == NULL) return WZ_RESULT_INVALID_ARGUMENT;
    ++state->calls;
    if (arguments.size != 0u) {
        int written = snprintf(result->message, sizeof(result->message), "%.*s",
                               (int)arguments.size,
                               (const char*)arguments.data);
        if (written < 0 || (size_t)written >= sizeof(result->message)) {
            return WZ_RESULT_BUFFER_TOO_SMALL;
        }
    }
    return WZ_RESULT_OK;
}

static bool command_unavailable(const void* context, const char** reason)
{
    (void)context;
    if (reason != NULL) *reason = "media-dirty";
    return false;
}

static int fail(const char* message)
{
    fprintf(stderr, "FAIL %s\n", message);
    return 1;
}

static bool contains(const char* text, const char* fragment)
{
    return text != NULL && fragment != NULL && strstr(text, fragment) != NULL;
}

int main(void)
{
    wz_command_registry_t registry;
    wz_command_metadata_t storage[4];
    handler_state_t handlers = {0u};
    wz_command_metadata_t commands[] = {
        {"machine.reset", "Reset", "Reset the machine", "machine", "NONE",
         NULL, "test-reset", "telnet", NULL, WZ_COMMAND_REMOTE_SAFE, NULL,
         command_handler, &handlers, true, true, NULL},
        {"machine.model.set", "Set model", "Choose a certified model",
         "machine", "48K|128K", NULL, "test-model", "telnet", NULL,
         WZ_COMMAND_REMOTE_SAFE, NULL, command_handler, &handlers, true, true,
         NULL},
        {"host.secret.read", "Read secret", "Read a private host value",
         "settings", "YES|NO", NULL, "test-secret", "telnet", NULL,
         WZ_COMMAND_HOST_READ, NULL, command_handler, &handlers, false, false,
         NULL},
        {"media.write", "Write media", "Write mounted media", "media",
         "ERASE", NULL, "test-media", "telnet", NULL,
         WZ_COMMAND_MEDIA_DESTRUCTIVE, command_unavailable, command_handler,
         &handlers, false, false, NULL}
    };
    char output[8192];
    char id[128];
    char arguments[128];
    size_t output_length = 0u;
    wz_command_result_t dispatch_result;

    if (wz_command_registry_init(&registry, storage, 4u) != WZ_RESULT_OK ||
        wz_command_registry_bind_owner_thread(&registry) != WZ_RESULT_OK) {
        return fail("registry initialization and owner binding");
    }
    for (size_t index = 0u; index < sizeof(commands) / sizeof(commands[0]); ++index) {
        if (wz_command_registry_register(&registry, commands[index]) != WZ_RESULT_OK) {
            return fail("command registration");
        }
    }
    if (wz_command_registry_finalize(&registry) != WZ_RESULT_OK) {
        return fail("registry finalization");
    }

    if (!wz_telnet_menu_parse("MENU") ||
        !wz_telnet_menu_format_root(output, sizeof(output), &output_length) ||
        !contains(output, "MENU file LABEL=\"File\"\r\n") ||
        !contains(output, "MENU machine LABEL=\"Machine\"\r\n") ||
        !contains(output, "MENU help LABEL=\"Help\"\r\n") ||
        !contains(output, "END\r\n")) {
        return fail("MENU root discovery");
    }
    if (!wz_telnet_menu_tree_parse("MENU TREE") ||
        !wz_telnet_menu_tree_format(&registry, output, sizeof(output),
                                    &output_length) ||
        !contains(output, "ITEM machine.reset PARENT=machine TYPE=COMMAND") ||
        !contains(output, "ITEM host.secret.read PARENT=settings") ||
        !contains(output, "ITEM media.write PARENT=media STATE=DISABLED") ||
        !contains(output, "REASON=media-dirty\r\n") ||
        !contains(output, "END\r\n")) {
        return fail("MENU TREE fields, state, reason, and terminator");
    }
    if (!wz_telnet_menu_id_parse("MENU machine", id, sizeof(id)) ||
        strcmp(id, "machine") != 0 ||
        !wz_telnet_menu_id_format(&registry, id, output, sizeof(output),
                                  &output_length) ||
        !contains(output, "MENU machine TYPE=MENU") ||
        !contains(output, "ITEM machine.model.set PARENT=machine") ||
        !contains(output, "END\r\n")) {
        return fail("MENU node and child discovery");
    }
    if (!wz_telnet_menu_find_parse("MENU FIND rEsEt", arguments,
                                  sizeof(arguments)) ||
        !wz_telnet_menu_find_format(&registry, arguments, output,
                                    sizeof(output), &output_length) ||
        !contains(output, "ITEM machine.reset") ||
        !contains(output, "END\r\n")) {
        return fail("case-insensitive MENU FIND");
    }
    if (!wz_telnet_describe_parse("DESCRIBE machine.model.set", id,
                                 sizeof(id)) ||
        !wz_telnet_describe_format(&registry, id, output, sizeof(output),
                                   &output_length) ||
        !contains(output, "PARAMETERS=48K|128K\r\n") ||
        !contains(output, "REMOTE_ALLOWED=YES\r\n") ||
        contains(output, "C:\\") || contains(output, "/home/")) {
        return fail("DESCRIBE metadata and path privacy");
    }

    if (!wz_telnet_do_parse("DO machine.model.set 128k", id, sizeof(id),
                           arguments, sizeof(arguments)) ||
        strcmp(id, "machine.model.set") != 0 || strcmp(arguments, "128k") != 0 ||
        !wz_telnet_do_format(&registry, id, arguments, output, sizeof(output),
                             &output_length) ||
        strcmp(output, "OK DO machine.model.set 128k\r\n") != 0 ||
        handlers.calls != 1u) {
        return fail("valid DO parse, validation, permission, and dispatch");
    }
    if (!wz_telnet_do_format(&registry, "machine.model.set", "16k", output,
                             sizeof(output), &output_length) ||
        strcmp(output, "ERR BAD_ARGUMENT\r\n") != 0 || handlers.calls != 1u) {
        return fail("invalid schema argument rejected before handler");
    }
    if (!wz_telnet_do_format(&registry, "host.secret.read", "MAYBE", output,
                             sizeof(output), &output_length) ||
        strcmp(output, "ERR BAD_ARGUMENT\r\n") != 0 ||
        !wz_telnet_do_format(&registry, "host.secret.read", "YES", output,
                             sizeof(output), &output_length) ||
        strcmp(output, "DENIED host.secret.read HOST_READ\r\n") != 0 ||
        handlers.calls != 1u) {
        return fail("argument validation precedes remote permission");
    }
    if (!wz_telnet_do_format(&registry, "media.write", "ERASE", output,
                             sizeof(output), &output_length) ||
        strcmp(output,
               "ERR BAD_STATE media.write media-dirty\r\n") != 0 ||
        !wz_telnet_do_format(&registry, "unknown.command", "", output,
                             sizeof(output), &output_length) ||
        strcmp(output, "ERR BAD_COMMAND_ID\r\n") != 0 ||
        handlers.calls != 1u) {
        return fail("state-disabled and unknown commands are controlled");
    }
    puts("Telnet registry discovery and DO regression passed");
    return 0;
}
