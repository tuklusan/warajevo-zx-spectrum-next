/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_command_registry.h"
#include "app/wz_telnet_alias_response.h"
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
    static const char expected_tree[] =
        "ITEM file PARENT=ROOT TYPE=MENU STATE=ENABLED REMOTE=ALLOWED CLASS=REMOTE_SAFE LABEL=\"File\"\r\n"
        "ITEM machine PARENT=ROOT TYPE=MENU STATE=ENABLED REMOTE=ALLOWED CLASS=REMOTE_SAFE LABEL=\"Machine\"\r\n"
        "ITEM media PARENT=ROOT TYPE=MENU STATE=ENABLED REMOTE=ALLOWED CLASS=REMOTE_SAFE LABEL=\"Media\"\r\n"
        "ITEM view PARENT=ROOT TYPE=MENU STATE=ENABLED REMOTE=ALLOWED CLASS=REMOTE_SAFE LABEL=\"View\"\r\n"
        "ITEM tools PARENT=ROOT TYPE=MENU STATE=ENABLED REMOTE=ALLOWED CLASS=REMOTE_SAFE LABEL=\"Tools\"\r\n"
        "ITEM settings PARENT=ROOT TYPE=MENU STATE=ENABLED REMOTE=ALLOWED CLASS=REMOTE_SAFE LABEL=\"Settings\"\r\n"
        "ITEM help PARENT=ROOT TYPE=MENU STATE=ENABLED REMOTE=ALLOWED CLASS=REMOTE_SAFE LABEL=\"Help\"\r\n"
        "ITEM machine.reset PARENT=machine TYPE=COMMAND STATE=ENABLED REMOTE=ALLOWED CLASS=REMOTE_SAFE LABEL=\"Reset\"\r\n"
        "ITEM machine.model.set PARENT=machine TYPE=COMMAND STATE=ENABLED REMOTE=ALLOWED CLASS=REMOTE_SAFE LABEL=\"Set model\"\r\n"
        "ITEM host.secret.read PARENT=settings TYPE=COMMAND STATE=ENABLED REMOTE=DENIED CLASS=HOST_READ LABEL=\"Read secret\"\r\n"
        "ITEM media.write PARENT=media TYPE=COMMAND STATE=DISABLED REMOTE=DENIED CLASS=MEDIA_DESTRUCTIVE LABEL=\"Write media\" REASON=media-dirty\r\n"
        "ITEM host.file.write PARENT=settings TYPE=COMMAND STATE=ENABLED REMOTE=DENIED CLASS=HOST_WRITE LABEL=\"Write file\"\r\n"
        "ITEM media.delete PARENT=media TYPE=COMMAND STATE=ENABLED REMOTE=DENIED CLASS=MEDIA_DESTRUCTIVE LABEL=\"Delete media\"\r\n"
        "ITEM application.quit PARENT=file TYPE=COMMAND STATE=ENABLED REMOTE=DENIED CLASS=APPLICATION_CONTROL LABEL=\"Quit\"\r\n"
        "ITEM local.toggle PARENT=view TYPE=COMMAND STATE=ENABLED REMOTE=DENIED CLASS=LOCAL_ONLY LABEL=\"Local toggle\"\r\n"
        "END\r\n";
    static const struct {
        const char* alias;
        const char* command;
    } alias_routes[] = {
        {"RESET", "DO machine.reset"},
        {"PAUSE", "DO machine.pause"},
        {"RESUME", "DO machine.resume"},
        {"SCREENSHOT", "DO host.screenshot.temp"},
        {"SPEED 25", "DO machine.speed.set 25"},
        {"SPEED 50", "DO machine.speed.set 50"},
        {"SPEED 100", "DO machine.speed.set 100"},
        {"SPEED 200", "DO machine.speed.set 200"},
        {"SPEED 400", "DO machine.speed.set 400"},
        {"SPEED 800", "DO machine.speed.set 800"},
        {"SPEED UNLIMITED", "DO machine.speed.set UNLIMITED"}
    };
    wz_command_registry_t registry;
    wz_command_metadata_t storage[8];
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
         &handlers, false, false, NULL},
        {"host.file.write", "Write file", "Write a host file", "settings",
         "YES|NO", NULL, "test-host-write", "telnet", NULL,
         WZ_COMMAND_HOST_WRITE, NULL, command_handler, &handlers, false, false,
         NULL},
        {"media.delete", "Delete media", "Delete mounted media", "media",
         "ERASE", NULL, "test-media-delete", "telnet", NULL,
         WZ_COMMAND_MEDIA_DESTRUCTIVE, NULL, command_handler, &handlers, false,
         false, NULL},
        {"application.quit", "Quit", "Quit the application", "file", "NONE",
         NULL, "test-quit", "telnet", NULL,
         WZ_COMMAND_APPLICATION_CONTROL, NULL, command_handler, &handlers,
         false, false, NULL},
        {"local.toggle", "Local toggle", "Change a local setting", "view",
         "NONE", NULL, "test-local", "telnet", NULL,
         WZ_COMMAND_LOCAL_ONLY, NULL, command_handler, &handlers, false, false,
         NULL}
    };
    char output[8192];
    char id[128];
    char arguments[128];
    size_t output_length = 0u;
    wz_command_result_t dispatch_result;

    if (wz_command_registry_init(&registry, storage, 8u) != WZ_RESULT_OK ||
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
        strcmp(output, expected_tree) != 0) {
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
    if (!wz_telnet_menu_find_parse("MENU FIND C:\\Users", arguments,
                                  sizeof(arguments)) ||
        !wz_telnet_menu_find_format(&registry, arguments, output,
                                    sizeof(output), &output_length) ||
        strcmp(output, "END\r\n") != 0) {
        return fail("MENU FIND cannot disclose arbitrary host paths");
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
    {
        wz_telnet_status_snapshot_t status = {
            .control_port = 30740u,
            .ipv4_up = true,
            .ipv6_up = false,
            .client_active = true,
            .model = "48K",
            .state = "RUNNING",
            .speed = "100",
            .audio = "ENABLED",
            .networking = "NONE"
        };
        if (!wz_telnet_status_format(&status, output, sizeof(output),
                                     &output_length) ||
            contains(output, "C:\\") || contains(output, "/home/") ||
            contains(output, "PATH=")) {
            return fail("STATUS exposes no arbitrary host path");
        }
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
    if (!wz_telnet_do_format(&registry, "media.write", "NOPE", output,
                             sizeof(output), &output_length) ||
        strcmp(output, "ERR BAD_ARGUMENT\r\n") != 0 ||
        !wz_telnet_do_format(&registry, "media.write", "ERASE", output,
                             sizeof(output), &output_length) ||
        strcmp(output,
               "ERR BAD_STATE media.write media-dirty\r\n") != 0 ||
        !wz_telnet_do_format(&registry, "unknown.command", "", output,
                             sizeof(output), &output_length) ||
        strcmp(output, "ERR BAD_COMMAND_ID\r\n") != 0 ||
        handlers.calls != 1u) {
        return fail("state-disabled and unknown commands are controlled");
    }
    for (size_t index = 0u;
         index < sizeof(alias_routes) / sizeof(alias_routes[0]); ++index) {
        if (!wz_telnet_alias_to_do(alias_routes[index].alias, output,
                                   sizeof(output)) ||
            strcmp(output, alias_routes[index].command) != 0) {
            return fail("special alias maps to its shared command ID");
        }
    }
    if (!wz_telnet_model_alias_to_do("MODEL 48K", output, sizeof(output)) ||
        strcmp(output, "DO machine.model.set 48k") != 0 ||
        !wz_telnet_model_alias_to_do("MODEL 128K", output, sizeof(output)) ||
        strcmp(output, "DO machine.model.set 128k") != 0 ||
        !wz_telnet_model_alias_to_do("MODEL 16K", output, sizeof(output)) ||
        strcmp(output, "DO machine.model.set invalid") != 0) {
        return fail("model aliases map to the shared model command");
    }
    {
        static const struct {
            const char* id;
            const char* arguments;
            const char* response;
        } denied[] = {
            {"host.secret.read", "YES", "DENIED host.secret.read HOST_READ\r\n"},
            {"host.file.write", "YES", "DENIED host.file.write HOST_WRITE\r\n"},
            {"media.delete", "ERASE", "DENIED media.delete MEDIA_DESTRUCTIVE\r\n"},
            {"application.quit", "", "DENIED application.quit APPLICATION_CONTROL\r\n"},
            {"local.toggle", "", "DENIED local.toggle LOCAL_ONLY\r\n"}
        };
        for (size_t index = 0u; index < sizeof(denied) / sizeof(denied[0]); ++index) {
            if (!wz_telnet_do_format(&registry, denied[index].id,
                                     denied[index].arguments, output,
                                     sizeof(output), &output_length) ||
                strcmp(output, denied[index].response) != 0 ||
                handlers.calls != 1u) {
                return fail("unauthenticated policy denies every unsafe class");
            }
        }
    }
    puts("Telnet registry discovery and DO regression passed");
    return 0;
}
