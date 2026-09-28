/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include "app/wz_command_registry.h"
#include "app/wz_screenshot_save_workflow.h"
#include "app/wz_telnet_alias_response.h"
#include "app/wz_telnet_keyboard_command.h"
#include "app/wz_ui_layout.h"
#include "core/wz_presentation_snapshot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "UI command equivalence failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

typedef struct {
    wz_command_registry_t* registry;
    wz_speed_policy_t speed;
    wz_ui_layout_state_t layout;
    bool paused;
    const char* model;
    unsigned resets;
} test_application_t;

static wz_result_t reset_handler(const void* context,
                                 wz_command_arguments_t arguments,
                                 wz_command_result_t* result)
{
    test_application_t* app = (test_application_t*)context;
    if (app == NULL || result == NULL || arguments.size != 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    ++app->resets;
    (void)snprintf(result->message, sizeof(result->message), "reset");
    return WZ_RESULT_OK;
}

static wz_result_t pause_handler(const void* context,
                                 wz_command_arguments_t arguments,
                                 wz_command_result_t* result)
{
    test_application_t* app = (test_application_t*)context;
    if (app == NULL || result == NULL || arguments.size != 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    app->paused = true;
    app->layout.paused = true;
    (void)snprintf(result->message, sizeof(result->message), "paused");
    return WZ_RESULT_OK;
}

static wz_result_t resume_handler(const void* context,
                                  wz_command_arguments_t arguments,
                                  wz_command_result_t* result)
{
    test_application_t* app = (test_application_t*)context;
    if (app == NULL || result == NULL || arguments.size != 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    app->paused = false;
    app->layout.paused = false;
    (void)snprintf(result->message, sizeof(result->message), "running");
    return WZ_RESULT_OK;
}

static wz_result_t pause_resume_handler(const void* context,
                                        wz_command_arguments_t arguments,
                                        wz_command_result_t* result)
{
    const test_application_t* app = (const test_application_t*)context;
    const char* id;
    if (app == NULL || result == NULL || arguments.size != 0u) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    id = app->paused ? "machine.resume" : "machine.pause";
    return wz_command_registry_dispatch(app->registry, id,
                                        (wz_command_arguments_t){NULL, 0u},
                                        result);
}

static wz_result_t speed_handler(const void* context,
                                 wz_command_arguments_t arguments,
                                 wz_command_result_t* result)
{
    test_application_t* app = (test_application_t*)context;
    char command[48];
    wz_telnet_speed_command_t speed;
    if (app == NULL || result == NULL || arguments.data == NULL ||
        arguments.size == 0u || arguments.size >= sizeof(command)) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    (void)snprintf(command, sizeof(command), "SPEED %.*s",
                   (int)arguments.size, (const char*)arguments.data);
    if (!wz_telnet_speed_parse(command, &speed) ||
        !wz_speed_policy_valid((wz_speed_policy_t)speed) ||
        !wz_ui_layout_select_speed(&app->layout, (wz_speed_policy_t)speed)) {
        return WZ_RESULT_PARSE_ERROR;
    }
    app->speed = (wz_speed_policy_t)speed;
    (void)snprintf(result->message, sizeof(result->message), "%s",
                   wz_ui_layout_speed_label((size_t)speed));
    return WZ_RESULT_OK;
}

static wz_result_t model_handler(const void* context,
                                 wz_command_arguments_t arguments,
                                 wz_command_result_t* result)
{
    test_application_t* app = (test_application_t*)context;
    const char* model;
    if (app == NULL || result == NULL || arguments.data == NULL) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    model = (const char*)arguments.data;
    if (arguments.size == 3u && memcmp(model, "48k", 3u) == 0) {
        app->model = "48k";
    } else if (arguments.size == 4u && memcmp(model, "128k", 4u) == 0) {
        app->model = "128k";
    } else {
        return WZ_RESULT_PARSE_ERROR;
    }
    (void)snprintf(result->message, sizeof(result->message), "%s", app->model);
    return WZ_RESULT_OK;
}

static bool telnet_alias_dispatch(wz_command_registry_t* registry,
                                  const char* alias,
                                  char* response, size_t response_capacity)
{
    char do_command[96];
    char id[64];
    char arguments[48];
    char dispatch_response[256];
    size_t dispatch_length = 0u;
    size_t response_length = 0u;
    bool translated = strncmp(alias, "MODEL ", 6u) == 0 ?
        wz_telnet_model_alias_to_do(alias, do_command, sizeof(do_command)) :
        wz_telnet_alias_to_do(alias, do_command, sizeof(do_command));
    return translated &&
        wz_telnet_do_parse(do_command, id, sizeof(id), arguments,
                           sizeof(arguments)) &&
        wz_telnet_do_format(registry, id, arguments, dispatch_response,
                            sizeof(dispatch_response), &dispatch_length) &&
        wz_telnet_alias_response_rewrite(
            alias, dispatch_response, response, response_capacity,
            &response_length);
}

static size_t menu_command_index(const wz_command_registry_t* registry,
                                 const char* id)
{
    const size_t count = wz_ui_layout_menu_command_count(registry, 1u);
    for (size_t index = 0u; index < count; ++index) {
        const wz_command_metadata_t* command =
            wz_ui_layout_menu_command_at(registry, 1u, index);
        if (command != NULL && strcmp(command->id, id) == 0) return index;
    }
    return (size_t)-1;
}

static bool invoke_registry(wz_command_registry_t* registry,
                            const char* id, const char* arguments,
                            wz_command_result_t* result)
{
    return wz_command_registry_dispatch(
        registry, id,
        (wz_command_arguments_t){arguments, arguments == NULL ? 0u :
            strlen(arguments)}, result) == WZ_RESULT_OK;
}

int main(void)
{
    wz_command_metadata_t storage[6];
    wz_command_registry_t registry;
    wz_command_result_t result;
    test_application_t app;
    char telnet_response[128];
    static const char* const speed_aliases[WZ_SPEED_COUNT] = {
        "SPEED 25", "SPEED 50", "SPEED 100", "SPEED 200",
        "SPEED 400", "SPEED 800", "SPEED UNLIMITED"
    };
    static const char* const speed_arguments[WZ_SPEED_COUNT] = {
        "25", "50", "100", "200", "400", "800", "unlimited"
    };
    wz_byte_t raster_samples[12u];
    wz_byte_t snapshot_samples[12u];
    wz_byte_t gui_png[256u];
    wz_byte_t telnet_png[256u];
    size_t gui_png_size = 0u;
    size_t telnet_png_size;
    char screenshot_path[WZ_SCREENSHOT_DESTINATION_CAPACITY];
    wz_raster_buffer_t raster;
    wz_presentation_snapshot_t snapshot;
    wz_screenshot_save_workflow_t save_workflow;
    FILE* screenshot_file;
    int read_ok;

    memset(&app, 0, sizeof(app));
    app.speed = WZ_SPEED_100;
    app.model = "48k";
    wz_ui_layout_state_init(&app.layout);
    REQUIRE(wz_command_registry_init(&registry, storage, 6u) == WZ_RESULT_OK);
    app.registry = &registry;
    REQUIRE(wz_command_registry_bind_owner_thread(&registry) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_register(&registry, (wz_command_metadata_t){
        "machine.pause_resume", "Pause / Resume", "Toggle emulation state",
        "machine", "NONE", NULL, "pause_resume_handler", "local", NULL,
        WZ_COMMAND_REMOTE_SAFE, NULL, pause_resume_handler, &app, true, true,
        NULL}) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_register(&registry, (wz_command_metadata_t){
        "machine.pause", "Pause", "Pause emulation", "machine", "NONE",
        NULL, "pause_handler", "shared", NULL, WZ_COMMAND_REMOTE_SAFE,
        NULL, pause_handler, &app, true, true, NULL}) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_register(&registry, (wz_command_metadata_t){
        "machine.resume", "Resume", "Resume emulation", "machine", "NONE",
        NULL, "resume_handler", "shared", NULL, WZ_COMMAND_REMOTE_SAFE,
        NULL, resume_handler, &app, true, true, NULL}) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_register(&registry, (wz_command_metadata_t){
        "machine.model.set", "Model", "Select model", "machine", "48K|128K",
        NULL, "model_handler", "menu", NULL, WZ_COMMAND_REMOTE_SAFE, NULL,
        model_handler, &app, true, true, NULL}) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_register(&registry, (wz_command_metadata_t){
        "machine.reset", "Reset", "Reset emulation", "machine", "NONE",
        NULL, "reset_handler", "shared", NULL, WZ_COMMAND_REMOTE_SAFE,
        NULL, reset_handler, &app, true, true, NULL}) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_register(&registry, (wz_command_metadata_t){
        "machine.speed.set", "Speed", "Set emulation speed", "machine",
        "PERCENT", "SPEED", "speed_handler", "menu", NULL,
        WZ_COMMAND_REMOTE_SAFE, NULL, speed_handler, &app, true, true,
        NULL}) == WZ_RESULT_OK);
    REQUIRE(wz_command_registry_finalize(&registry) == WZ_RESULT_OK);

    REQUIRE(invoke_registry(&registry, "machine.reset", NULL, &result));
    REQUIRE(app.resets == 1u);
    REQUIRE(wz_ui_layout_activate_menu_command(
        &registry, 1u, menu_command_index(&registry, "machine.reset"),
        (wz_command_arguments_t){NULL, 0u}, &result) == WZ_RESULT_OK);
    REQUIRE(app.resets == 2u);
    REQUIRE(wz_ui_layout_activate_toolbar(
        &registry, 2u, (wz_command_arguments_t){NULL, 0u}, &result) ==
        WZ_RESULT_OK);
    REQUIRE(app.resets == 3u);
    REQUIRE(telnet_alias_dispatch(&registry, "RESET", telnet_response,
                                 sizeof(telnet_response)));
    REQUIRE(strcmp(telnet_response, "OK RESET\r\n") == 0);
    REQUIRE(app.resets == 4u);

    REQUIRE(wz_ui_layout_activate_toolbar(
        &registry, 1u, (wz_command_arguments_t){NULL, 0u}, &result) ==
        WZ_RESULT_OK);
    REQUIRE(app.paused && app.layout.paused);
    REQUIRE(invoke_registry(&registry, "machine.resume", NULL, &result));
    REQUIRE(!app.paused && !app.layout.paused);
    REQUIRE(telnet_alias_dispatch(&registry, "PAUSE", telnet_response,
                                 sizeof(telnet_response)));
    REQUIRE(app.paused && strcmp(telnet_response, "OK PAUSE\r\n") == 0);
    REQUIRE(telnet_alias_dispatch(&registry, "RESUME", telnet_response,
                                 sizeof(telnet_response)));
    REQUIRE(!app.paused && strcmp(telnet_response, "OK RESUME\r\n") == 0);

    for (size_t index = 0u; index < WZ_SPEED_COUNT; ++index) {
        REQUIRE(wz_ui_layout_activate_speed(
            &registry, (wz_speed_policy_t)index, &result) == WZ_RESULT_OK);
        REQUIRE(app.speed == (wz_speed_policy_t)index);
        REQUIRE(invoke_registry(&registry, "machine.speed.set",
                                speed_arguments[index], &result));
        REQUIRE(app.speed == (wz_speed_policy_t)index);
        REQUIRE(telnet_alias_dispatch(&registry, speed_aliases[index],
                                     telnet_response, sizeof(telnet_response)));
        REQUIRE(app.speed == (wz_speed_policy_t)index);
    }
    REQUIRE(wz_ui_layout_activate_model(&registry, "128k", &result) ==
            WZ_RESULT_OK);
    REQUIRE(strcmp(app.model, "128k") == 0);
    REQUIRE(invoke_registry(&registry, "machine.model.set", "48k", &result));
    REQUIRE(strcmp(app.model, "48k") == 0);
    REQUIRE(telnet_alias_dispatch(&registry, "MODEL 128K", telnet_response,
                                 sizeof(telnet_response)));
    REQUIRE(strcmp(app.model, "128k") == 0 &&
            strcmp(telnet_response, "OK MODEL 128K\r\n") == 0);

    for (size_t index = 0u; index < sizeof(raster_samples); ++index) {
        raster_samples[index] = (wz_byte_t)(index % 16u);
    }
    REQUIRE(wz_raster_buffer_init(&raster, 4u, 3u, raster_samples,
                                  sizeof(raster_samples)) == WZ_RESULT_OK);
    wz_screenshot_save_workflow_init(&save_workflow);
    REQUIRE(wz_screenshot_save_as(
        &save_workflow, "gui-screenshot.png", &raster, gui_png,
        sizeof(gui_png), &gui_png_size) == WZ_SCREENSHOT_SAVE_OK);
    REQUIRE(wz_presentation_snapshot_init(
        &snapshot, 4u, 3u, snapshot_samples, sizeof(snapshot_samples)) ==
        WZ_RESULT_OK);
    REQUIRE(wz_presentation_snapshot_publish(&snapshot, &raster) ==
            WZ_RESULT_OK);
    REQUIRE(wz_telnet_screenshot_save(
        &snapshot, screenshot_path, sizeof(screenshot_path)) ==
        WZ_TELNET_SCREENSHOT_OK);
    screenshot_file = fopen(screenshot_path, "rb");
    REQUIRE(screenshot_file != NULL);
    read_ok = fread(telnet_png, 1u, sizeof(telnet_png), screenshot_file) > 0u;
    if (fclose(screenshot_file) != 0) read_ok = 0;
    telnet_png_size = wz_screenshot_png_required_size(&raster);
    if (remove(screenshot_path) != 0) read_ok = 0;
    REQUIRE(read_ok && gui_png_size == telnet_png_size &&
            memcmp(gui_png, telnet_png, gui_png_size) == 0);

    puts("GUI, toolbar, Telnet, and registry command paths are equivalent");
    return 0;
}
