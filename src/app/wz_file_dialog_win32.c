/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#define COBJMACROS
#include <windows.h>
#include <shobjidl.h>
#include <sokol_app.h>
#include <limits.h>

#include "app/wz_file_dialog.h"

wz_file_dialog_result_t wz_file_dialog_open(char* utf8_path,
                                            size_t path_capacity)
{
    static const COMDLG_FILTERSPEC filters[] = {
        {L"Supported media and snapshots",
         L"*.tap;*.tzx;*.wav;*.sna;*.z80;*.mdr"},
        {L"All files", L"*.*"}
    };
    IFileOpenDialog* dialog = NULL;
    IShellItem* item = NULL;
    PWSTR wide_path = NULL;
    HRESULT result;
    HRESULT apartment_result;
    DWORD options = 0u;
    int required;
    wz_file_dialog_result_t dialog_result = WZ_FILE_DIALOG_FAILED;

    if (utf8_path == NULL || path_capacity == 0u ||
        path_capacity > (size_t)INT_MAX) {
        return WZ_FILE_DIALOG_FAILED;
    }
    utf8_path[0] = '\0';
    apartment_result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(apartment_result)) {
        return WZ_FILE_DIALOG_FAILED;
    }
    result = CoCreateInstance(&CLSID_FileOpenDialog, NULL,
        CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog, (void**)&dialog);
    if (FAILED(result) || dialog == NULL) goto cleanup;
    result = IFileOpenDialog_GetOptions(dialog, &options);
    if (FAILED(result)) goto cleanup;
    result = IFileOpenDialog_SetOptions(dialog, options |
        FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST |
        FOS_NOCHANGEDIR);
    if (FAILED(result)) goto cleanup;
    result = IFileOpenDialog_SetFileTypes(dialog,
        (UINT)(sizeof(filters) / sizeof(filters[0])), filters);
    if (FAILED(result)) goto cleanup;
    result = IFileOpenDialog_SetTitle(dialog, L"Open / Run");
    if (FAILED(result)) goto cleanup;
    result = IFileOpenDialog_Show(dialog, (HWND)sapp_win32_get_hwnd());
    if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        dialog_result = WZ_FILE_DIALOG_CANCELLED;
        goto cleanup;
    }
    if (FAILED(result)) goto cleanup;
    result = IFileOpenDialog_GetResult(dialog, &item);
    if (FAILED(result) || item == NULL) goto cleanup;
    result = IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &wide_path);
    if (FAILED(result) || wide_path == NULL) goto cleanup;
    required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide_path,
                                   -1, NULL, 0, NULL, NULL);
    if (required <= 1 || (size_t)required > path_capacity ||
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide_path, -1,
                            utf8_path, (int)path_capacity, NULL, NULL) !=
            required) {
        utf8_path[0] = '\0';
        goto cleanup;
    }
    dialog_result = WZ_FILE_DIALOG_SELECTED;

cleanup:
    if (wide_path != NULL) CoTaskMemFree(wide_path);
    if (item != NULL) IShellItem_Release(item);
    if (dialog != NULL) IFileOpenDialog_Release(dialog);
    CoUninitialize();
    return dialog_result;
}

wz_file_dialog_result_t wz_file_dialog_save_tap(char* utf8_path,
                                                size_t path_capacity)
{
    static const COMDLG_FILTERSPEC filters[] = {
        {L"Standard TAP tape", L"*.tap"}, {L"All files", L"*.*"}
    };
    IFileSaveDialog* dialog = NULL;
    IShellItem* item = NULL;
    PWSTR wide_path = NULL;
    HRESULT result;
    HRESULT apartment_result;
    DWORD options = 0u;
    int required;
    wz_file_dialog_result_t dialog_result = WZ_FILE_DIALOG_FAILED;
    if (utf8_path == NULL || path_capacity == 0u ||
        path_capacity > (size_t)INT_MAX) return WZ_FILE_DIALOG_FAILED;
    utf8_path[0] = '\0';
    apartment_result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(apartment_result)) return WZ_FILE_DIALOG_FAILED;
    result = CoCreateInstance(&CLSID_FileSaveDialog, NULL,
        CLSCTX_INPROC_SERVER, &IID_IFileSaveDialog, (void**)&dialog);
    if (FAILED(result) || dialog == NULL) goto cleanup;
    result = IFileSaveDialog_GetOptions(dialog, &options);
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_SetOptions(dialog, options |
        FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT |
        FOS_NOCHANGEDIR);
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_SetFileTypes(dialog,
        (UINT)(sizeof(filters) / sizeof(filters[0])), filters);
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_SetDefaultExtension(dialog, L"tap");
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_SetFileName(dialog, L"tape-copy.tap");
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_SetTitle(dialog, L"Save Standard TAP");
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_Show(dialog, (HWND)sapp_win32_get_hwnd());
    if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        dialog_result = WZ_FILE_DIALOG_CANCELLED;
        goto cleanup;
    }
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_GetResult(dialog, &item);
    if (FAILED(result) || item == NULL) goto cleanup;
    result = IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &wide_path);
    if (FAILED(result) || wide_path == NULL) goto cleanup;
    required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide_path,
                                   -1, NULL, 0, NULL, NULL);
    if (required <= 1 || (size_t)required > path_capacity ||
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide_path, -1,
                            utf8_path, (int)path_capacity, NULL, NULL) !=
            required) {
        utf8_path[0] = '\0';
        goto cleanup;
    }
    dialog_result = WZ_FILE_DIALOG_SELECTED;
cleanup:
    if (wide_path != NULL) CoTaskMemFree(wide_path);
    if (item != NULL) IShellItem_Release(item);
    if (dialog != NULL) IFileSaveDialog_Release(dialog);
    CoUninitialize();
    return dialog_result;
}

wz_file_dialog_result_t wz_file_dialog_save_snapshot(char* utf8_path,
                                                      size_t path_capacity)
{
    static const COMDLG_FILTERSPEC filters[] = {
        {L"Spectrum snapshots", L"*.sna;*.z80"}, {L"All files", L"*.*"}
    };
    IFileSaveDialog* dialog = NULL;
    IShellItem* item = NULL;
    PWSTR wide_path = NULL;
    HRESULT result;
    HRESULT apartment_result;
    DWORD options = 0u;
    int required;
    wz_file_dialog_result_t dialog_result = WZ_FILE_DIALOG_FAILED;
    if (utf8_path == NULL || path_capacity == 0u ||
        path_capacity > (size_t)INT_MAX) return WZ_FILE_DIALOG_FAILED;
    utf8_path[0] = '\0';
    apartment_result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(apartment_result)) return WZ_FILE_DIALOG_FAILED;
    result = CoCreateInstance(&CLSID_FileSaveDialog, NULL,
        CLSCTX_INPROC_SERVER, &IID_IFileSaveDialog, (void**)&dialog);
    if (FAILED(result) || dialog == NULL) goto cleanup;
    result = IFileSaveDialog_GetOptions(dialog, &options);
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_SetOptions(dialog, options |
        FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT |
        FOS_NOCHANGEDIR);
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_SetFileTypes(dialog,
        (UINT)(sizeof(filters) / sizeof(filters[0])), filters);
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_SetDefaultExtension(dialog, L"z80");
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_SetFileName(dialog, L"machine.z80");
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_SetTitle(dialog, L"Save Snapshot");
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_Show(dialog, (HWND)sapp_win32_get_hwnd());
    if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        dialog_result = WZ_FILE_DIALOG_CANCELLED;
        goto cleanup;
    }
    if (FAILED(result)) goto cleanup;
    result = IFileSaveDialog_GetResult(dialog, &item);
    if (FAILED(result) || item == NULL) goto cleanup;
    result = IShellItem_GetDisplayName(item, SIGDN_FILESYSPATH, &wide_path);
    if (FAILED(result) || wide_path == NULL) goto cleanup;
    required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide_path,
                                   -1, NULL, 0, NULL, NULL);
    if (required <= 1 || (size_t)required > path_capacity ||
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide_path, -1,
                            utf8_path, (int)path_capacity, NULL, NULL) !=
            required) {
        utf8_path[0] = '\0';
        goto cleanup;
    }
    dialog_result = WZ_FILE_DIALOG_SELECTED;
cleanup:
    if (wide_path != NULL) CoTaskMemFree(wide_path);
    if (item != NULL) IShellItem_Release(item);
    if (dialog != NULL) IFileSaveDialog_Release(dialog);
    CoUninitialize();
    return dialog_result;
}
