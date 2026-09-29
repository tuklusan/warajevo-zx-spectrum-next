/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#import <AppKit/AppKit.h>

#include <string.h>

#include "app/wz_file_dialog.h"

wz_file_dialog_result_t wz_file_dialog_open(char* utf8_path,
                                            size_t path_capacity)
{
    wz_file_dialog_result_t result = WZ_FILE_DIALOG_FAILED;
    if (utf8_path == NULL || path_capacity == 0u) {
        return WZ_FILE_DIALOG_FAILED;
    }
    utf8_path[0] = '\0';
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        NSArray<NSString*>* file_types = @[
            @"tap", @"tzx", @"wav", @"sna", @"z80", @"mdr"
        ];
        [panel setTitle:@"Open / Run"];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        [panel setAllowedFileTypes:file_types];
        [panel setAllowsOtherFileTypes:YES];
        if ([panel runModal] != NSModalResponseOK) {
            result = WZ_FILE_DIALOG_CANCELLED;
        } else {
            NSURL* url = [[panel URLs] firstObject];
            const char* path = [url.path UTF8String];
            size_t path_length = path == NULL ? 0u : strlen(path);
            if (path_length != 0u && path_length < path_capacity) {
                memcpy(utf8_path, path, path_length + 1u);
                result = WZ_FILE_DIALOG_SELECTED;
            }
        }
    }
    return result;
}

wz_file_dialog_result_t wz_file_dialog_save_tap(char* utf8_path,
                                                size_t path_capacity)
{
    wz_file_dialog_result_t result = WZ_FILE_DIALOG_FAILED;
    if (utf8_path == NULL || path_capacity == 0u) return WZ_FILE_DIALOG_FAILED;
    utf8_path[0] = '\0';
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        [panel setTitle:@"Save Standard TAP"];
        [panel setAllowedFileTypes:@[@"tap"]];
        [panel setNameFieldStringValue:@"tape-copy.tap"];
        [panel setCanCreateDirectories:YES];
        if ([panel runModal] != NSModalResponseOK) {
            result = WZ_FILE_DIALOG_CANCELLED;
        } else {
            const char* path = [[[panel URL] path] UTF8String];
            size_t path_length = path == NULL ? 0u : strlen(path);
            if (path_length != 0u && path_length < path_capacity) {
                memcpy(utf8_path, path, path_length + 1u);
                result = WZ_FILE_DIALOG_SELECTED;
            }
        }
    }
    return result;
}
