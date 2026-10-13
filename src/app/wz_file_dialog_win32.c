/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <windows.h>
#include <commdlg.h>
#include <sokol_app.h>
#include <limits.h>

#include "app/wz_file_dialog.h"

static wz_file_dialog_result_t wz_file_dialog_show(
    BOOL save, const wchar_t* title, const wchar_t* filter,
    const wchar_t* default_name, const wchar_t* default_extension,
    char* utf8_path, size_t path_capacity)
{
    wchar_t wide_path[32768] = L"";
    OPENFILENAMEW dialog;
    DWORD error;
    int required;

    if (utf8_path == NULL || path_capacity == 0u ||
        path_capacity > (size_t)INT_MAX) {
        return WZ_FILE_DIALOG_FAILED;
    }
    utf8_path[0] = '\0';
    ZeroMemory(&dialog, sizeof(dialog));
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = (HWND)sapp_win32_get_hwnd();
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = wide_path;
    dialog.nMaxFile = (DWORD)(sizeof(wide_path) / sizeof(wide_path[0]));
    dialog.lpstrTitle = title;
    dialog.lpstrDefExt = default_extension;
    dialog.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_NOCHANGEDIR |
        OFN_PATHMUSTEXIST | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (default_name != NULL) {
        (void)lstrcpynW(wide_path, default_name, (int)dialog.nMaxFile);
    }

    if (!(save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog))) {
        error = CommDlgExtendedError();
        return error == 0u ? WZ_FILE_DIALOG_CANCELLED : WZ_FILE_DIALOG_FAILED;
    }
    required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide_path,
                                   -1, NULL, 0, NULL, NULL);
    if (required <= 1 || (size_t)required > path_capacity ||
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide_path, -1,
                            utf8_path, (int)path_capacity, NULL, NULL) !=
            required) {
        utf8_path[0] = '\0';
        return WZ_FILE_DIALOG_FAILED;
    }
    return WZ_FILE_DIALOG_SELECTED;
}

wz_file_dialog_result_t wz_file_dialog_open(char* utf8_path,
                                            size_t path_capacity)
{
    static const wchar_t filter[] =
        L"Supported media and snapshots\0"
        L"*.tap;*.tzx;*.wav;*.sna;*.z80;*.mdr\0"
        L"All files\0*.*\0\0";
    return wz_file_dialog_show(false, L"Open / Run", filter, NULL, NULL,
                               utf8_path, path_capacity);
}

wz_file_dialog_result_t wz_file_dialog_save_tap(char* utf8_path,
                                                size_t path_capacity)
{
    static const wchar_t filter[] =
        L"Standard TAP tape\0*.tap\0All files\0*.*\0\0";
    return wz_file_dialog_show(true, L"Save Standard TAP", filter,
                               L"tape-copy.tap", L"tap", utf8_path,
                               path_capacity);
}

wz_file_dialog_result_t wz_file_dialog_save_snapshot(char* utf8_path,
                                                     size_t path_capacity)
{
    static const wchar_t filter[] =
        L"Spectrum snapshots\0*.sna;*.z80\0All files\0*.*\0\0";
    return wz_file_dialog_show(true, L"Save Snapshot", filter,
                               L"machine.z80", L"z80", utf8_path,
                               path_capacity);
}
