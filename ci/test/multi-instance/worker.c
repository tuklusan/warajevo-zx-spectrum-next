/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <io.h>
#include <windows.h>
#define path_exists(path) (_access((path), 0) == 0)
#define path_separator '\\'
#else
#include <unistd.h>
#define path_exists(path) (access((path), F_OK) == 0)
#define path_separator '/'
#endif

#include "app/wz_control_port.h"
#include "app/wz_host_config.h"
#include "app/wz_host_media_ownership.h"
#include "app/wz_telnet_keyboard_command.h"
#include "core/wz_presentation_snapshot.h"

#define BARRIER_TIMEOUT_SECONDS 30
#define CONFIG_PAYLOAD_SIZE (4u * 1024u * 1024u)

static bool make_path(char* output, size_t capacity,
                      const char* directory, const char* name)
{
    int written = snprintf(output, capacity, "%s%c%s", directory,
                           path_separator, name);
    return written >= 0 && (size_t)written < capacity;
}

static void sleep_milliseconds(unsigned milliseconds)
{
#if defined(_WIN32)
    Sleep(milliseconds);
#else
    struct timespec duration;
    duration.tv_sec = (time_t)(milliseconds / 1000u);
    duration.tv_nsec = (long)(milliseconds % 1000u) * 1000000L;
    (void)nanosleep(&duration, NULL);
#endif
}

static bool wait_for_barrier(const char* directory, unsigned index)
{
    char ready_name[64];
    char ready_path[1024];
    char go_path[1024];
    FILE* ready;
    time_t deadline = time(NULL) + BARRIER_TIMEOUT_SECONDS;

    if (snprintf(ready_name, sizeof(ready_name), "ready-%u", index) < 0 ||
        !make_path(ready_path, sizeof(ready_path), directory, ready_name) ||
        !make_path(go_path, sizeof(go_path), directory, "go")) {
        return false;
    }
    ready = fopen(ready_path, "wb");
    if (ready == NULL) return false;
    {
        bool success = fputs("ready", ready) >= 0;
        if (fclose(ready) != 0) success = false;
        if (!success) return false;
    }
    while (!path_exists(go_path)) {
        if (time(NULL) >= deadline) return false;
        sleep_milliseconds(10u);
    }
    return true;
}

static int run_control_port(const char* directory, unsigned index,
                            unsigned hold_milliseconds)
{
    wz_control_port_owner_t owner;
    wz_control_port_probe_result_t result;
    if (!wait_for_barrier(directory, index) || !wz_host_socket_system_init()) {
        return 1;
    }
    wz_control_port_owner_init(&owner);
    result = wz_control_port_probe(&owner);
    if (result != WZ_CONTROL_PORT_PROBE_AVAILABLE || owner.selected_port == 0u) {
        wz_host_socket_system_shutdown();
        return 2;
    }
    printf("CONTROL %u\n", (unsigned)owner.selected_port);
    fflush(stdout);
    sleep_milliseconds(hold_milliseconds);
    wz_control_port_owner_close(&owner);
    wz_host_socket_system_shutdown();
    return 0;
}

static int run_settings(const char* directory, unsigned index)
{
    char settings_path[1024];
    const char* shared_directory = getenv("WZSN_SETTINGS_DIR");
    unsigned char* payload;
    bool written;
    if (!wait_for_barrier(directory, index) ||
        !make_path(settings_path, sizeof(settings_path),
                   shared_directory != NULL && shared_directory[0] != '\0'
                       ? shared_directory : directory,
                   "settings.ini")) {
        return 1;
    }
    payload = (unsigned char*)malloc(CONFIG_PAYLOAD_SIZE);
    if (payload == NULL) return 2;
    memset(payload, (int)('A' + index % 26u), CONFIG_PAYLOAD_SIZE);
    written = wz_host_config_write_atomic(settings_path, payload,
                                           CONFIG_PAYLOAD_SIZE);
    free(payload);
    puts(written ? "CONFIG OK" : "CONFIG BUSY");
    return 0;
}

static int run_media_claim(const char* directory, unsigned index,
                           unsigned hold_milliseconds)
{
    char media_path[1024];
    wz_host_media_claim_t claim;
    bool acquired;
    if (!wait_for_barrier(directory, index) ||
        !make_path(media_path, sizeof(media_path), directory, "media.img")) {
        return 1;
    }
    acquired = wz_host_media_claim_acquire(media_path, true, &claim);
    puts(acquired ? "MEDIA OK" : "MEDIA BUSY");
    fflush(stdout);
    if (acquired) {
        sleep_milliseconds(hold_milliseconds);
        wz_host_media_claim_release(&claim);
    }
    return 0;
}

static int run_screenshot(const char* directory, unsigned index)
{
    const size_t pixel_count =
        (size_t)WZ_RASTER_CANONICAL_WIDTH * WZ_RASTER_CANONICAL_HEIGHT;
    wz_byte_t* pixels = (wz_byte_t*)malloc(pixel_count * 2u);
    wz_presentation_snapshot_t snapshot;
    wz_raster_buffer_t source;
    char output_path[1024];
    wz_telnet_screenshot_result_t result;

    memset(&snapshot, 0, sizeof(snapshot));
    if (pixels == NULL ||
        wz_raster_buffer_init(&source, WZ_RASTER_CANONICAL_WIDTH,
                              WZ_RASTER_CANONICAL_HEIGHT, pixels,
                              pixel_count) != WZ_RESULT_OK ||
        wz_presentation_snapshot_init(&snapshot, WZ_RASTER_CANONICAL_WIDTH,
                                      WZ_RASTER_CANONICAL_HEIGHT,
                                      pixels + pixel_count,
                                      pixel_count) != WZ_RESULT_OK) {
        free(pixels);
        return 1;
    }
    for (size_t y = 0u; y < source.height; ++y) {
        for (size_t x = 0u; x < source.width; ++x) {
            pixels[y * source.width + x] = (wz_byte_t)((x + y + index) % 16u);
        }
    }
    if (wz_presentation_snapshot_publish(&snapshot, &source) != WZ_RESULT_OK ||
        !wait_for_barrier(directory, index)) {
        free(pixels);
        return 2;
    }
    result = wz_telnet_screenshot_save(&snapshot, output_path,
                                       sizeof(output_path));
    free(pixels);
    if (result != WZ_TELNET_SCREENSHOT_OK) return 3;
    printf("SCREENSHOT %s\n", output_path);
    return 0;
}

int main(int argc, char** argv)
{
    char* end = NULL;
    unsigned long index;
    unsigned long hold_milliseconds;
    if (argc != 5) return 64;
    index = strtoul(argv[3], &end, 10);
    if (end == argv[3] || *end != '\0' || index > 10000u) return 64;
    hold_milliseconds = strtoul(argv[4], &end, 10);
    if (end == argv[4] || *end != '\0' || hold_milliseconds > 30000u) return 64;

    if (strcmp(argv[1], "control") == 0) {
        return run_control_port(argv[2], (unsigned)index,
                                (unsigned)hold_milliseconds);
    }
    if (strcmp(argv[1], "settings") == 0) {
        return run_settings(argv[2], (unsigned)index);
    }
    if (strcmp(argv[1], "media") == 0) {
        return run_media_claim(argv[2], (unsigned)index,
                               (unsigned)hold_milliseconds);
    }
    if (strcmp(argv[1], "screenshot") == 0) {
        return run_screenshot(argv[2], (unsigned)index);
    }
    return 64;
}
