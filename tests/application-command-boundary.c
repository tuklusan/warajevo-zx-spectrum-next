/*
Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms.
*/

#include "app/wz_command_registry.h"
#include "app/wz_ui_layout.h"
#include "app/wz_telnet_keyboard_command.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef struct {
    unsigned mutations;
} test_machine_t;

typedef struct {
    wz_command_registry_t* registry;
    wz_result_t dispatch_result;
    wz_result_t bind_result;
    wz_command_result_t command_result;
} worker_context_t;

static wz_result_t reset_handler(const void* context,
                                 wz_command_arguments_t arguments,
                                 wz_command_result_t* result)
{
    test_machine_t* machine = (test_machine_t*)context;
    if (machine == NULL || arguments.size != 0u || result == NULL) {
        return WZ_RESULT_INVALID_ARGUMENT;
    }
    ++machine->mutations;
    (void)snprintf(result->message, sizeof(result->message), "reset");
    return WZ_RESULT_OK;
}

static void run_wrong_thread(worker_context_t* context)
{
    wz_command_arguments_t arguments = {NULL, 0u};
    context->bind_result = wz_command_registry_bind_owner_thread(
        context->registry);
    context->dispatch_result = wz_command_registry_dispatch(
        context->registry, "machine.reset", arguments,
        &context->command_result);
}

#if defined(_WIN32)
static DWORD WINAPI worker_entry(void* parameter)
{
    run_wrong_thread((worker_context_t*)parameter);
    return 0u;
}
#else
static void* worker_entry(void* parameter)
{
    run_wrong_thread((worker_context_t*)parameter);
    return NULL;
}
#endif

static int fail(const char* message)
{
    (void)fprintf(stderr, "FAIL application-command-boundary: %s\n", message);
    return 1;
}

int main(void)
{
    wz_command_metadata_t storage[1];
    wz_command_registry_t registry;
    wz_command_result_t result;
    wz_command_arguments_t arguments = {NULL, 0u};
    wz_command_metadata_t reset = {
        "machine.reset", "Reset", "Reset the machine", "machine",
        "NONE", "RESET", "reset_handler", "test", NULL,
        WZ_COMMAND_REMOTE_SAFE, NULL, reset_handler, NULL, true, true, NULL
    };
    test_machine_t machine = {0u};
    worker_context_t worker;
    char output[128];
    size_t output_length = 0u;
    size_t toolbar_index = 0u;
#if defined(_WIN32)
    HANDLE thread;
    DWORD wait_result;
#else
    pthread_t thread;
#endif

    reset.handler_context = &machine;
    if (wz_command_registry_init(&registry, storage, 1u) != WZ_RESULT_OK ||
        wz_command_registry_bind_owner_thread(&registry) != WZ_RESULT_OK ||
        wz_command_registry_register(&registry, reset) != WZ_RESULT_OK ||
        wz_command_registry_finalize(&registry) != WZ_RESULT_OK) {
        return fail("registry setup and owner binding");
    }

    if (wz_command_registry_dispatch(&registry, "machine.reset", arguments,
                                     &result) != WZ_RESULT_OK ||
        machine.mutations != 1u) {
        return fail("application test projection dispatch");
    }
    if (!wz_ui_layout_toolbar_hit_test(640.0f * 2.5f /
                                       (float)WZ_UI_TOOLBAR_COUNT,
                                       42.0f, 640.0f, &toolbar_index) ||
        toolbar_index != 2u ||
        wz_ui_layout_toolbar_hit_test(640.0f, 42.0f, 640.0f,
                                      &toolbar_index) ||
        wz_ui_layout_activate_toolbar(&registry, toolbar_index,
                                     arguments, &result) !=
            WZ_RESULT_OK || machine.mutations != 2u) {
        return fail("GUI toolbar hit target did not share registry dispatch");
    }
    if (!wz_telnet_do_format(&registry, "machine.reset", "", output,
                             sizeof(output), &output_length) ||
        output_length == 0u || strstr(output, "OK DO machine.reset") == NULL ||
        machine.mutations != 3u) {
        return fail("Telnet projection did not share registry dispatch");
    }

    memset(&worker, 0, sizeof(worker));
    worker.registry = &registry;
#if defined(_WIN32)
    thread = CreateThread(NULL, 0u, worker_entry, &worker, 0u, NULL);
    if (thread == NULL) return fail("could not create worker thread");
    wait_result = WaitForSingleObject(thread, INFINITE);
    (void)CloseHandle(thread);
    if (wait_result != WAIT_OBJECT_0) return fail("worker join failed");
#else
    if (pthread_create(&thread, NULL, worker_entry, &worker) != 0) {
        return fail("could not create worker thread");
    }
    if (pthread_join(thread, NULL) != 0) return fail("worker join failed");
#endif
    if (worker.bind_result != WZ_RESULT_INVALID_STATE ||
        worker.dispatch_result != WZ_RESULT_INVALID_STATE ||
        worker.command_result.reason == NULL ||
        strcmp(worker.command_result.reason, "wrong-thread") != 0 ||
        machine.mutations != 3u) {
        return fail("non-owner thread reached the machine mutation handler");
    }

    puts("PASS application-command-boundary cases=4");
    return 0;
}
