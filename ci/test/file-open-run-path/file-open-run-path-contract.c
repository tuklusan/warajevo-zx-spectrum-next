/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
This file is governed by the SANYALnet Labs Non-Commercial License in the
root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
for AI/ML model training are prohibited unless separately authorized.
Attribution is required: "Based on original work by Supratim Sanyal of
SANYALnet Labs." See LICENSE for full terms. */

#include "app/wz_file_open_run.h"

#include <stdio.h>

static int expect_route(const char* path, wz_open_run_route_t expected)
{
    wz_open_run_route_t actual = WZ_OPEN_RUN_UNSUPPORTED;
    const wz_open_run_result_t result = wz_file_open_run_route(path, &actual);
    const wz_open_run_result_t expected_result =
        expected == WZ_OPEN_RUN_UNSUPPORTED
            ? WZ_OPEN_RUN_UNSUPPORTED_FORMAT : WZ_OPEN_RUN_OK;

    if (result != expected_result || actual != expected) {
        (void)fprintf(stderr, "unexpected route for: %s\n", path);
        return 0;
    }
    return 1;
}

int main(void)
{
    static const struct {
        const char* path;
        wz_open_run_route_t route;
    } cases[] = {
        {"archive.v1/tape.tap", WZ_OPEN_RUN_TAPE},
        {"archive.tap/state.z80", WZ_OPEN_RUN_SNAPSHOT},
        {"C:\\Program Files\\ZX.v2\\disk.mdr", WZ_OPEN_RUN_MICRODRIVE},
        {"archive.voc\\legacy.scr", WZ_OPEN_RUN_CONVERSION},
        {"archive.tap\\without-extension", WZ_OPEN_RUN_UNSUPPORTED},
        {"folder.with.dot\\disk.data.mdr", WZ_OPEN_RUN_MICRODRIVE},
        {"folder.with.dot\\unknown.bin", WZ_OPEN_RUN_UNSUPPORTED},
        {"single.tap", WZ_OPEN_RUN_TAPE},
        {"folder.with.dot/", WZ_OPEN_RUN_UNSUPPORTED},
        {"folder/.tap", WZ_OPEN_RUN_UNSUPPORTED},
        {"folder/.hidden.z80", WZ_OPEN_RUN_SNAPSHOT}
    };
    size_t index;

    for (index = 0u; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        if (!expect_route(cases[index].path, cases[index].route)) return 1;
    }
    (void)printf("PASS file_open_run_route_path_contract cases=%zu\n",
        sizeof(cases) / sizeof(cases[0]));
    return 0;
}
