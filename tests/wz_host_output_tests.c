/*
Warajevo ZX Spectrum Next
Copyright (c) 2026 Supratim Sanyal, SANYALnet Labs, for new original project material.
New original material is licensed under GNU GPL v2 or later (GPL-2.0-or-later), as stated in LICENSE.txt.
Upstream Warajevo and third-party material retain their applicable copyrights and licenses.
See LICENSE.txt and NOTICE.md for complete terms and provenance.
*/

#include <stdio.h>
#include <string.h>

#include "app/wz_host_output.h"

static int read_file(const char* path, char* output, size_t capacity)
{
    FILE* file = fopen(path, "rb");
    size_t size;
    if (file == NULL || capacity == 0u) {
        if (file != NULL) fclose(file);
        return 0;
    }
    size = fread(output, 1u, capacity - 1u, file);
    output[size] = '\0';
    fclose(file);
    return 1;
}

int main(void)
{
    const char* path = "wz-host-output-test.bin";
    const char first[] = "complete-first";
    const char second[] = "complete-second";
    char actual[64];

    remove(path);
    if (!wz_host_output_write_atomic(path, first, sizeof(first) - 1u) ||
        !read_file(path, actual, sizeof(actual)) ||
        strcmp(actual, first) != 0 ||
        !wz_host_output_write_atomic(path, second, sizeof(second) - 1u) ||
        !read_file(path, actual, sizeof(actual)) ||
        strcmp(actual, second) != 0 ||
        wz_host_output_write_atomic(NULL, second, sizeof(second) - 1u) ||
        wz_host_output_write_atomic(path, NULL, 1u)) {
        remove(path);
        fputs("host output atomic contract failed\n", stderr);
        return 1;
    }
    remove(path);
    puts("wz_host_output atomic contract passed");
    return 0;
}
