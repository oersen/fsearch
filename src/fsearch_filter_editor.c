#include "fsearch_filter_editor.h"

#include <glib/gi18n.h>

#include "fsearch_string_utils.h"

struct FsearchFilterEditor {
    FsearchFilter *filter;
    GtkBuilder *builder;
    GtkWidget *dialog;
    GtkEntry *name_entry;
    GtkTextBuffer *query_text_buffer;
    GtkToggleButton *search_in_path;
    GtkToggleButton *enable_regex;
    GtkToggleButton *match_case;
    FsearchFilterEditorResponse *callback;
    gpointer data;
};

static void
fsearch_filter_editor_free(FsearchFilterEditor *editor) {
    g_clear_object(&editor->builder);
    g_clear_pointer(&editor->filter, fsearch_filter_unref);
    g_clear_pointer(&editor->dialog, gtk_widget_destroy);
    g_clear_pointer(&editor, free);
}

static void
on_editor_ui_response(GtkDialog *dialog, GtkResponseType response, gpointer user_data) {
    FsearchFilterEditor *editor = user_data;

    char *name = NULL;
    char *query = NULL;
    FsearchQueryFlags flags = 0;

    const char *name_str = gtk_entry_get_text(editor->name_entry);
    if (response == GTK_RESPONSE_OK && !fs_str_is_empty(name_str)) {
        name = g_strdup(name_str);

        GtkTextIter start, end;
        gtk_text_buffer_get_bounds(editor->query_text_buffer, &start, &end);
        query = gtk_text_buffer_get_text(editor->query_text_buffer, &start, &end, FALSE);
        const gboolean match_case = gtk_toggle_button_get_active(editor->match_case);
        const gboolean enable_regex = gtk_toggle_button_get_active(editor->enable_regex);
        const gboolean search_in_path = gtk_toggle_button_get_active(editor->search_in_path);
        if (match_case) {
            flags |= QUERY_FLAG_MATCH_CASE;
        }
        if (enable_regex) {
            flags |= QUERY_FLAG_REGEX;
        }
        if (search_in_path) {
            flags |= QUERY_FLAG_SEARCH_IN_PATH;
        }
    }

    if (editor->callback) {
        editor->callback(editor->filter, name, query, flags, editor->data);
    }

    fsearch_filter_editor_free(editor);
}

void
fsearch_filter_editor_run(const char *title,
                          GtkWindow *parent_window,
                          FsearchFilter *filter,
                          FsearchFilterEditorResponse callback,
                          gpointer data) {
    FsearchFilterEditor *editor = calloc(1, sizeof(FsearchFilterEditor));
    g_assert(editor != NULL);

    editor->filter = filter;
    editor->callback = callback;
    editor->data = data;

    editor->builder = gtk_builder_new_from_resource("/io/github/cboxdoerfer/fsearch/ui/fsearch_filter_editor.ui");
    editor->dialog = GTK_WIDGET(gtk_builder_get_object(editor->builder, "FsearchFilterEditorWindow"));
    editor->search_in_path = GTK_TOGGLE_BUTTON(gtk_builder_get_object(editor->builder, "filter_search_in_path"));
    editor->enable_regex = GTK_TOGGLE_BUTTON(gtk_builder_get_object(editor->builder, "filter_regex"));
    editor->match_case = GTK_TOGGLE_BUTTON(gtk_builder_get_object(editor->builder, "filter_match_case"));
    editor->name_entry = GTK_ENTRY(gtk_builder_get_object(editor->builder, "filter_name"));
    editor->query_text_buffer = GTK_TEXT_BUFFER(gtk_builder_get_object(editor->builder, "filter_query_buffer"));

    if (title) {
        gtk_window_set_title(GTK_WINDOW(editor->dialog), title);
    }

    if (filter) {
        gtk_entry_set_text(editor->name_entry, filter->name);
        gtk_text_buffer_set_text(editor->query_text_buffer, filter->query, -1);
        gtk_toggle_button_set_active(editor->search_in_path, filter->flags & QUERY_FLAG_SEARCH_IN_PATH ? TRUE : FALSE);
        gtk_toggle_button_set_active(editor->match_case, filter->flags & QUERY_FLAG_MATCH_CASE ? TRUE : FALSE);
        gtk_toggle_button_set_active(editor->enable_regex, filter->flags & QUERY_FLAG_REGEX ? TRUE : FALSE);
    }
    gtk_window_set_transient_for(GTK_WINDOW(editor->dialog), parent_window);
    gtk_dialog_add_button(GTK_DIALOG(editor->dialog), _("_Cancel"), GTK_RESPONSE_CANCEL);
    gtk_dialog_add_button(GTK_DIALOG(editor->dialog), _("_OK"), GTK_RESPONSE_OK);
    g_signal_connect(editor->dialog, "response", G_CALLBACK(on_editor_ui_response), editor);

    gtk_widget_show(editor->dialog);
}
