/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#import <AppKit/AppKit.h>
#import <dispatch/dispatch.h>

void wz_native_dialog_prepare_cancel(void)
{
    (void)[NSApplication sharedApplication];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC),
                   dispatch_get_main_queue(), ^{
        for (NSWindow* window in [NSApp windows]) {
            if ([window isKindOfClass:[NSOpenPanel class]]) {
                [(NSOpenPanel*)window cancel:nil];
                return;
            }
        }
    });
}
