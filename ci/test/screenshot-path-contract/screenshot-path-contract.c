/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app/wz_telnet_keyboard_command.h"
#include "core/wz_presentation_snapshot.h"

int wz_screenshot_test_clock(struct timespec* value, int base)
{
    if (value == NULL || base != TIME_UTC) return 0;
    value->tv_sec = (time_t)1760000000;
    value->tv_nsec = 123000000L;
    return base;
}

#define REQUIRE(condition) do { \
    if (!(condition)) { \
        (void)fprintf(stderr, "Screenshot path contract failed at line %d: %s\n", \
                      __LINE__, #condition); \
        goto cleanup; \
    } \
} while (0)

static int png_signature(const char* path)
{
    static const unsigned char signature[8] = {
        0x89u, 0x50u, 0x4eu, 0x47u, 0x0du, 0x0au, 0x1au, 0x0au
    };
    unsigned char actual[sizeof(signature)];
    FILE* file = fopen(path, "rb");
    int valid;
    if (file == NULL) return 0;
    valid = fread(actual, 1u, sizeof(actual), file) == sizeof(actual) &&
        memcmp(actual, signature, sizeof(signature)) == 0;
    if (fclose(file) != 0) valid = 0;
    return valid;
}

static int expected_path(char* output, size_t capacity,
                         const char* directory, const char* filename)
{
    size_t directory_length;
    const char* separator;
    int written;
    if (output == NULL || directory == NULL || filename == NULL ||
        directory[0] == '\0') return 0;
    directory_length = strlen(directory);
    separator = directory[directory_length - 1u] == '/' ? "" : "/";
    written = snprintf(output, capacity, "%s%s%s", directory, separator,
                       filename);
    return written > 0 && (size_t)written < capacity;
}

int main(int argc, char** argv)
{
    static const unsigned char sentinel[] = "preserve-existing-output";
    wz_byte_t source_pixels[4u] = {1u, 2u, 3u, 4u};
    wz_byte_t snapshot_pixels[4u] = {0u};
    wz_raster_buffer_t raster;
    wz_presentation_snapshot_t snapshot;
    char first_path[1024] = {0};
    char second_path[1024] = {0};
    char expected_first_path[1024];
    char expected_second_path[1024];
    char expected_name[64];
    char expected_collision_name[72];
    char date[16];
    struct tm local_time;
    time_t fixed_time = (time_t)1760000000;
    FILE* file;
    unsigned char preserved[sizeof(sentinel)];
    int result = 1;

    REQUIRE(argc == 2);
    REQUIRE(setenv("WZSN_SCREENSHOT_DIR", argv[1], 1) == 0);
    REQUIRE(localtime_r(&fixed_time, &local_time) != NULL);
    REQUIRE(strftime(date, sizeof(date), "%Y%m%d%H%M%S", &local_time) == 14u);
    REQUIRE(snprintf(expected_name, sizeof(expected_name),
                     "ZX-Screen-%s123.png", date) > 0);
    REQUIRE(snprintf(expected_collision_name, sizeof(expected_collision_name),
                     "ZX-Screen-%s123-1.png", date) > 0);
    REQUIRE(expected_path(expected_first_path, sizeof(expected_first_path),
                          argv[1], expected_name));
    REQUIRE(expected_path(expected_second_path, sizeof(expected_second_path),
                          argv[1], expected_collision_name));
    REQUIRE(wz_raster_buffer_init(&raster, 2u, 2u, source_pixels,
                                  sizeof(source_pixels)) == WZ_RESULT_OK);
    REQUIRE(wz_presentation_snapshot_init(&snapshot, 2u, 2u,
        snapshot_pixels, sizeof(snapshot_pixels)) == WZ_RESULT_OK);
    REQUIRE(wz_presentation_snapshot_publish(&snapshot, &raster) ==
            WZ_RESULT_OK);

    REQUIRE(wz_telnet_screenshot_save(&snapshot, first_path,
        sizeof(first_path)) == WZ_TELNET_SCREENSHOT_OK);
    REQUIRE(strcmp(first_path, expected_first_path) == 0);
    REQUIRE(png_signature(first_path));

    file = fopen(first_path, "wb");
    REQUIRE(file != NULL);
    REQUIRE(fwrite(sentinel, 1u, sizeof(sentinel), file) == sizeof(sentinel));
    REQUIRE(fclose(file) == 0);

    REQUIRE(wz_telnet_screenshot_save(&snapshot, second_path,
        sizeof(second_path)) == WZ_TELNET_SCREENSHOT_OK);
    REQUIRE(strcmp(second_path, expected_second_path) == 0);
    REQUIRE(strcmp(first_path, second_path) != 0);
    REQUIRE(png_signature(second_path));

    file = fopen(first_path, "rb");
    REQUIRE(file != NULL);
    REQUIRE(fread(preserved, 1u, sizeof(preserved), file) == sizeof(preserved));
    REQUIRE(fclose(file) == 0);
    REQUIRE(memcmp(preserved, sentinel, sizeof(sentinel)) == 0);
    REQUIRE(remove(first_path) == 0);
    first_path[0] = '\0';
    REQUIRE(remove(second_path) == 0);
    second_path[0] = '\0';
    (void)puts("PASS screenshot destination, filename, and no-clobber collision contract");
    result = 0;

cleanup:
    if (first_path[0] != '\0') (void)remove(first_path);
    if (second_path[0] != '\0') (void)remove(second_path);
    return result;
}
