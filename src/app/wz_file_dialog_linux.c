/* Copyright (c) 2026 Supratim Sanyal of SANYALnet Labs.
 * This file is governed by the SANYALnet Labs Non-Commercial License in the
 * root LICENSE file. Non-Commercial use is permitted; Commercial Use and use
 * for AI/ML model training are prohibited unless separately authorized.
 * Attribution is required: "Based on original work by Supratim Sanyal of
 * SANYALnet Labs." See LICENSE for full terms.
 */

#include <gtk/gtk.h>

#include <string.h>

#include "app/wz_file_dialog.h"

typedef struct {
    GMainLoop* loop;
    gint response;
} wz_file_dialog_wait_t;

static void wz_file_dialog_on_response(GtkNativeDialog* dialog,
                                       gint response,
                                       gpointer user_data)
{
    wz_file_dialog_wait_t* wait = (wz_file_dialog_wait_t*)user_data;
    (void)dialog;
    if (wait == NULL) return;
    wait->response = response;
    g_main_loop_quit(wait->loop);
}

wz_file_dialog_result_t wz_file_dialog_open(char* utf8_path,
                                            size_t path_capacity)
{
static const char* const patterns[] = {
        "*.tap", "*.TAP", "*.tzx", "*.TZX", "*.wav", "*.WAV",
        "*.sna", "*.SNA", "*.z80", "*.Z80", "*.mdr", "*.MDR"
    };
    GtkFileChooserNative* dialog = NULL;
    GtkFileFilter* supported_filter = NULL;
    GtkFileFilter* all_filter = NULL;
    wz_file_dialog_wait_t wait = {NULL, GTK_RESPONSE_NONE};
    char* filename = NULL;
    char* utf8_filename = NULL;
    GError* error = NULL;
    gsize utf8_length = 0u;
    size_t index;
    wz_file_dialog_result_t result = WZ_FILE_DIALOG_FAILED;

    if (utf8_path == NULL || path_capacity == 0u) {
        return WZ_FILE_DIALOG_FAILED;
    }
    utf8_path[0] = '\0';
    if (!gtk_init_check(NULL, NULL)) return WZ_FILE_DIALOG_FAILED;
    dialog = gtk_file_chooser_native_new("Open / Run", NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN, "Open", "Cancel");
    if (dialog == NULL) goto cleanup;
    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(dialog), TRUE);
    supported_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(supported_filter,
                             "Supported media and snapshots");
    for (index = 0u; index < sizeof(patterns) / sizeof(patterns[0]); ++index) {
        gtk_file_filter_add_pattern(supported_filter, patterns[index]);
    }
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), supported_filter);
    all_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(all_filter, "All files");
    gtk_file_filter_add_pattern(all_filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), all_filter);

    wait.loop = g_main_loop_new(NULL, FALSE);
    if (wait.loop == NULL) goto cleanup;
    wait.response = GTK_RESPONSE_NONE;
    g_signal_connect(dialog, "response",
                     G_CALLBACK(wz_file_dialog_on_response), &wait);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
    g_main_loop_run(wait.loop);
    if (wait.response != GTK_RESPONSE_ACCEPT) {
        result = WZ_FILE_DIALOG_CANCELLED;
    } else {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename != NULL) {
            utf8_filename = g_filename_to_utf8(filename, -1, NULL,
                                               &utf8_length, &error);
        }
        if (utf8_filename != NULL && utf8_length < path_capacity) {
            memcpy(utf8_path, utf8_filename, utf8_length);
            utf8_path[utf8_length] = '\0';
            result = WZ_FILE_DIALOG_SELECTED;
        }
    }
    g_main_loop_unref(wait.loop);
    wait.loop = NULL;

cleanup:
    if (wait.loop != NULL) g_main_loop_unref(wait.loop);
    if (error != NULL) g_error_free(error);
    g_free(utf8_filename);
    g_free(filename);
    if (supported_filter != NULL) g_object_unref(supported_filter);
    if (all_filter != NULL) g_object_unref(all_filter);
    if (dialog != NULL) g_object_unref(dialog);
    return result;
}

wz_file_dialog_result_t wz_file_dialog_save_tap(char* utf8_path,
                                                size_t path_capacity)
{
    GtkFileChooserNative* dialog = NULL;
    GtkFileFilter* filter = NULL;
    wz_file_dialog_wait_t wait = {NULL, GTK_RESPONSE_NONE};
    char* filename = NULL;
    char* utf8_filename = NULL;
    GError* error = NULL;
    gsize utf8_length = 0u;
    wz_file_dialog_result_t result = WZ_FILE_DIALOG_FAILED;
    if (utf8_path == NULL || path_capacity == 0u) return WZ_FILE_DIALOG_FAILED;
    utf8_path[0] = '\0';
    if (!gtk_init_check(NULL, NULL)) return WZ_FILE_DIALOG_FAILED;
    dialog = gtk_file_chooser_native_new("Save Standard TAP", NULL,
        GTK_FILE_CHOOSER_ACTION_SAVE, "Save", "Cancel");
    if (dialog == NULL) goto cleanup;
    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(dialog), TRUE);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "tape-copy.tap");
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Standard TAP tape (*.tap)");
    gtk_file_filter_add_pattern(filter, "*.tap");
    gtk_file_filter_add_pattern(filter, "*.TAP");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    wait.loop = g_main_loop_new(NULL, FALSE);
    if (wait.loop == NULL) goto cleanup;
    g_signal_connect(dialog, "response",
                     G_CALLBACK(wz_file_dialog_on_response), &wait);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
    g_main_loop_run(wait.loop);
    if (wait.response != GTK_RESPONSE_ACCEPT) {
        result = WZ_FILE_DIALOG_CANCELLED;
    } else {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename != NULL) utf8_filename = g_filename_to_utf8(
            filename, -1, NULL, &utf8_length, &error);
        if (utf8_filename != NULL && utf8_length < path_capacity) {
            memcpy(utf8_path, utf8_filename, utf8_length);
            utf8_path[utf8_length] = '\0';
            result = WZ_FILE_DIALOG_SELECTED;
        }
    }
cleanup:
    if (wait.loop != NULL) g_main_loop_unref(wait.loop);
    if (error != NULL) g_error_free(error);
    g_free(utf8_filename);
    g_free(filename);
    if (filter != NULL) g_object_unref(filter);
    if (dialog != NULL) g_object_unref(dialog);
    return result;
}

wz_file_dialog_result_t wz_file_dialog_save_snapshot(char* utf8_path,
                                                      size_t path_capacity)
{
    GtkFileChooserNative* dialog = NULL;
    GtkFileFilter* filter = NULL;
    wz_file_dialog_wait_t wait = {NULL, GTK_RESPONSE_NONE};
    char* filename = NULL;
    char* utf8_filename = NULL;
    GError* error = NULL;
    gsize utf8_length = 0u;
    wz_file_dialog_result_t result = WZ_FILE_DIALOG_FAILED;
    if (utf8_path == NULL || path_capacity == 0u) return WZ_FILE_DIALOG_FAILED;
    utf8_path[0] = '\0';
    if (!gtk_init_check(NULL, NULL)) return WZ_FILE_DIALOG_FAILED;
    dialog = gtk_file_chooser_native_new("Save Snapshot", NULL,
        GTK_FILE_CHOOSER_ACTION_SAVE, "Save", "Cancel");
    if (dialog == NULL) goto cleanup;
    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(dialog), TRUE);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "machine.z80");
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Spectrum snapshots (*.sna, *.z80)");
    gtk_file_filter_add_pattern(filter, "*.sna");
    gtk_file_filter_add_pattern(filter, "*.SNA");
    gtk_file_filter_add_pattern(filter, "*.z80");
    gtk_file_filter_add_pattern(filter, "*.Z80");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    wait.loop = g_main_loop_new(NULL, FALSE);
    if (wait.loop == NULL) goto cleanup;
    g_signal_connect(dialog, "response",
                     G_CALLBACK(wz_file_dialog_on_response), &wait);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(dialog));
    g_main_loop_run(wait.loop);
    if (wait.response != GTK_RESPONSE_ACCEPT) {
        result = WZ_FILE_DIALOG_CANCELLED;
    } else {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename != NULL) utf8_filename = g_filename_to_utf8(
            filename, -1, NULL, &utf8_length, &error);
        if (utf8_filename != NULL && utf8_length < path_capacity) {
            memcpy(utf8_path, utf8_filename, utf8_length);
            utf8_path[utf8_length] = '\0';
            result = WZ_FILE_DIALOG_SELECTED;
        }
    }
cleanup:
    if (wait.loop != NULL) g_main_loop_unref(wait.loop);
    if (error != NULL) g_error_free(error);
    g_free(utf8_filename);
    g_free(filename);
    if (filter != NULL) g_object_unref(filter);
    if (dialog != NULL) g_object_unref(dialog);
    return result;
}
