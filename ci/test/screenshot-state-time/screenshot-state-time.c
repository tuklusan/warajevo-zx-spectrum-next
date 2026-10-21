/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#define WZ_SET_ENV(name, value) _putenv_s((name), (value))
#define WZ_PATH_SEPARATOR "\\"
#else
#include <sys/stat.h>
#define WZ_SET_ENV(name, value) setenv((name), (value), 1)
#define WZ_PATH_SEPARATOR "/"
#endif

#include "app/wz_telnet_keyboard_command.h"
#include "core/wz_machine.h"
#include "core/wz_presentation_snapshot.h"
#include "core/wz_state.h"

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, \
            "Screenshot state/time contract failed at line %d: %s\n", \
            __LINE__, #condition); \
        goto cleanup; \
    } \
    ++assertions; \
} while (0)

static unsigned assertions;

static int state_hash(const wz_machine_t* machine, wz_qword_t* hash)
{
    return wz_state_hash_machine(machine, hash) == WZ_RESULT_OK;
}

static int read_file(const char* path, const void* expected, size_t length)
{
    unsigned char actual[64];
    FILE* file;
    int valid;
    if (path == NULL || expected == NULL || length > sizeof(actual)) return 0;
    file = fopen(path, "rb");
    if (file == NULL) return 0;
    valid = fread(actual, 1u, length, file) == length &&
        memcmp(actual, expected, length) == 0;
    if (fclose(file) != 0) valid = 0;
    return valid;
}

int main(void)
{
    static const wz_byte_t pixels[4u] = {1u, 2u, 3u, 4u};
    static const char blocked_contents[] = "not a directory";
    wz_machine_t machine;
    wz_presentation_snapshot_t snapshot;
    wz_raster_buffer_t raster;
    wz_byte_t snapshot_pixels[4u] = {0u};
    wz_qword_t before_hash;
    wz_qword_t after_hash;
    wz_master_tick_t before_tick;
    char output_directory[1024];
    char blocker_path[1024];
    char screenshot_path[1024] = {0};
    const char* configured_directory = getenv("WZSN_SCREENSHOT_DIR");
    FILE* blocker = NULL;
    int blocker_path_length;
    int blocker_close_status;
    bool machine_initialized = false;
    int result = 1;

    if (configured_directory == NULL || configured_directory[0] == '\0' ||
        strlen(configured_directory) >= sizeof(output_directory)) {
        (void)fputs("WZSN_SCREENSHOT_DIR must name the runner build directory.\n",
                    stderr);
        return 1;
    }
    (void)snprintf(output_directory, sizeof(output_directory), "%s",
                   configured_directory);
    blocker_path_length = snprintf(blocker_path, sizeof(blocker_path),
        "%s%s%s", output_directory, WZ_PATH_SEPARATOR,
        "screenshot-state-time-blocker");
    if (blocker_path_length < 0 ||
        (size_t)blocker_path_length >= sizeof(blocker_path)) {
        (void)fputs("Screenshot blocker path is too long.\n", stderr);
        return 1;
    }
    if (wz_machine_init(&machine, wz_machine_profile_48k_pal()) !=
            WZ_RESULT_OK) {
        (void)fputs("Could not initialize the canonical machine.\n", stderr);
        return 1;
    }
    machine_initialized = true;
    REQUIRE(wz_machine_advance_master_ticks(&machine, 12345u) == WZ_RESULT_OK);
    REQUIRE(wz_raster_buffer_init(&raster, 2u, 2u,
                                  (wz_byte_t*)pixels, sizeof(pixels)) ==
            WZ_RESULT_OK);
    REQUIRE(wz_presentation_snapshot_init(&snapshot, 2u, 2u, snapshot_pixels,
            sizeof(snapshot_pixels)) == WZ_RESULT_OK);
    REQUIRE(wz_presentation_snapshot_publish(&snapshot, &raster) ==
            WZ_RESULT_OK);

    REQUIRE(state_hash(&machine, &before_hash));
    before_tick = machine.master_tick;
    REQUIRE(wz_telnet_screenshot_save(&snapshot, screenshot_path,
            sizeof(screenshot_path)) == WZ_TELNET_SCREENSHOT_OK);
    REQUIRE(screenshot_path[0] != '\0');
    REQUIRE(read_file(screenshot_path, "\x89PNG\r\n\x1a\n", 8u));
    REQUIRE(machine.master_tick == before_tick);
    REQUIRE(state_hash(&machine, &after_hash) && after_hash == before_hash);
    REQUIRE(remove(screenshot_path) == 0);
    screenshot_path[0] = '\0';

    blocker = fopen(blocker_path, "wb");
    REQUIRE(blocker != NULL);
    REQUIRE(fwrite(blocked_contents, 1u, sizeof(blocked_contents), blocker) ==
            sizeof(blocked_contents));
    blocker_close_status = fclose(blocker);
    blocker = NULL;
    REQUIRE(blocker_close_status == 0);
    REQUIRE(WZ_SET_ENV("WZSN_SCREENSHOT_DIR", blocker_path) == 0);
    REQUIRE(state_hash(&machine, &before_hash));
    before_tick = machine.master_tick;
    REQUIRE(wz_telnet_screenshot_save(&snapshot, screenshot_path,
            sizeof(screenshot_path)) != WZ_TELNET_SCREENSHOT_OK);
    REQUIRE(screenshot_path[0] == '\0');
    REQUIRE(machine.master_tick == before_tick);
    REQUIRE(state_hash(&machine, &after_hash) && after_hash == before_hash);
    REQUIRE(remove(blocker_path) == 0);
    (void)puts("PASS screenshot success and failure preserve canonical machine hash and master tick (20 assertions)");
    result = 0;

cleanup:
    if (blocker != NULL) (void)fclose(blocker);
    if (screenshot_path[0] != '\0') (void)remove(screenshot_path);
    (void)remove(blocker_path);
    if (machine_initialized) wz_machine_destroy(&machine);
    return result;
}
