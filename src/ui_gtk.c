#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gtk/axeiiloader_gtk.h"
#include "axeii_loader.h"

static enum {
    SEND_MODE = 0,
    RECEIVE_MODE
} mode = SEND_MODE;

static dev_info_t **devs = NULL;
static GObject *mididevs, *type, *tabs,
               *sendfile, *sendloc, *senddetail, *recdir, *recloc, *rectype,
               *messagelabel, *progressbar, *startbutton,
               *sendadjust, *recadjust;

gchar path[256];

/* Required by axeii_utils */
void progressCallback(int currentProgress) {
    char valAsString[16];
    sprintf(valAsString, "%d", currentProgress);
    if (currentProgress >= 0) {
        gtk_label_set_text(GTK_LABEL(messagelabel), "Doing transfer...");
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbar), ((gdouble)currentProgress) / 100);
    } else if (currentProgress < 0) {
        gtk_label_set_text(GTK_LABEL(messagelabel), "Trying to capture header...");
    }

    if (currentProgress == 100) {
        gtk_label_set_text(GTK_LABEL(messagelabel), "Transfer complete!");
    }
}

void nameProvider(char *name) {
    char buf[256];
    sprintf(buf, "File saved as %s", name);
    gtk_label_set_text(GTK_LABEL(messagelabel), buf);
}

void checkAndEnable() {
    char passed_checks = 1, properties;
    gchar *path, *midi, *axe_type;

    axe_type = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(type));

    /* Check file is valid */
    if (mode == SEND_MODE) {
        path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(sendfile));
    } else {
        /* Is a directory path valid for receive mode */
        path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(recdir));
    }
    if (path == NULL) passed_checks = 0;

    /* If in send mode, is the file at the given path valid? */
    /* Also constrain IR send locations */
    if (mode == SEND_MODE && passed_checks) {
        properties = detectFileProperties(path);
        if ((properties == FILE_ERROR) || !(properties & IS_VALID)) {
            gtk_widget_set_sensitive(GTK_WIDGET(sendloc), FALSE);
            gtk_label_set_text(GTK_LABEL(senddetail), "Type could not be detected");
            passed_checks = 0;
        } else if (properties & IS_PRESET) {
            gtk_widget_set_sensitive(GTK_WIDGET(sendloc), FALSE);
            gtk_label_set_text(GTK_LABEL(senddetail), "Preset File Detected");
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(sendloc), TRUE);
            gtk_label_set_text(GTK_LABEL(senddetail), "IR File Detected");
            gtk_adjustment_set_lower(GTK_ADJUSTMENT(sendadjust), 1.0);
            if (axe_type[0] == 'O') {
                gtk_adjustment_set_upper(GTK_ADJUSTMENT(sendadjust), 104.0);
            } else {
                gtk_adjustment_set_upper(GTK_ADJUSTMENT(sendadjust), 1028.0);
            }
            gtk_adjustment_set_value(GTK_ADJUSTMENT(sendadjust),
                                     gtk_adjustment_get_value(GTK_ADJUSTMENT(sendadjust)));
        }
    }

    /* Constrain receive locations */
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rectype))) {
        /* PRESET */
        gtk_adjustment_set_lower(GTK_ADJUSTMENT(recadjust), 0.0);
        if (axe_type[0] == 'O') {
            gtk_adjustment_set_upper(GTK_ADJUSTMENT(recadjust), 383.0);
        } else {
            gtk_adjustment_set_upper(GTK_ADJUSTMENT(recadjust), 767.0);
        }
    } else {
        /* IR */
        gtk_adjustment_set_lower(GTK_ADJUSTMENT(recadjust), 1.0);
        if (axe_type[0] == 'O') {
            gtk_adjustment_set_upper(GTK_ADJUSTMENT(recadjust), 100.0);
        } else {
            gtk_adjustment_set_upper(GTK_ADJUSTMENT(recadjust), 1024.0);
        }
    }
    gtk_adjustment_set_value(GTK_ADJUSTMENT(recadjust),
                             gtk_adjustment_get_value(GTK_ADJUSTMENT(recadjust)));

    /* Is MIDI device valid */
    passed_checks = 0;

    if (passed_checks) {
        gtk_widget_set_sensitive(GTK_WIDGET(startbutton), TRUE);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(startbutton), FALSE);
    }
}

/* Callbacks */

void box_cb(GtkComboBox* self, gpointer user_data) {
    /*g_print("In midi_cb\n");*/
    checkAndEnable();
}

void tabs_cb(GtkNotebook* self, GtkWidget* page, guint page_num, gpointer user_data) {
    /*g_print("In tabs_cb\n");*/
    mode = page_num;
    checkAndEnable();
}

void file_cb(GtkFileChooserButton* self, gpointer user_data) {
    /*g_print("In file_cb\n");*/
    checkAndEnable();
}

void rectype_cb(GtkToggleButton* self, gpointer user_data) {
    /*g_print("In rectype_cb\n");*/
    checkAndEnable();
}

void start_cb(GtkButton* self, gpointer user_data) {
    /*g_print("In start_cb\n");*/

    /* The library expects a trailing / on directory names */
    /*strcat(filepath, "/");*/

    gtk_widget_set_sensitive(GTK_WIDGET(startbutton), FALSE);
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rectype));
}

/* MAIN */

int main(int argc, char *argv[]) {
    GtkBuilder *builder;
    GObject *window;
    GResource *ui = axeiiloader_gtk_get_resource();
    g_resources_register(ui);

    gtk_init(&argc, &argv);
    builder = gtk_builder_new_from_resource("/m0jxd/axeiiloader/builder.ui");

    /* Setup widgets from builder */
    mididevs     = gtk_builder_get_object(builder, "mididevs");
    type         = gtk_builder_get_object(builder, "type");
    tabs         = gtk_builder_get_object(builder, "tabs");
    sendfile     = gtk_builder_get_object(builder, "sendfile");
    sendloc      = gtk_builder_get_object(builder, "sendloc");
    senddetail   = gtk_builder_get_object(builder, "senddetail");
    recdir       = gtk_builder_get_object(builder, "recdir");
    recloc       = gtk_builder_get_object(builder, "recloc");
    rectype      = gtk_builder_get_object(builder, "rectype");
    messagelabel = gtk_builder_get_object(builder, "messagelabel");
    progressbar  = gtk_builder_get_object(builder, "progressbar");
    startbutton  = gtk_builder_get_object(builder, "startbutton");
    sendadjust   = gtk_builder_get_object(builder, "sendadjust");
    recadjust    = gtk_builder_get_object(builder, "recadjust");
    window       = gtk_builder_get_object(builder, "window");

    /* Connect callbacks */
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(mididevs, "changed", G_CALLBACK(box_cb), NULL);
    g_signal_connect(type, "changed", G_CALLBACK(box_cb), NULL);
    g_signal_connect(tabs, "switch-page", G_CALLBACK(tabs_cb), NULL);
    g_signal_connect(sendfile, "file-set", G_CALLBACK(file_cb), NULL);
    g_signal_connect(recdir, "file-set", G_CALLBACK(file_cb), NULL);
    g_signal_connect(rectype, "toggled", G_CALLBACK(rectype_cb), NULL);
    g_signal_connect(startbutton, "clicked", G_CALLBACK(start_cb), NULL);

    /* NB: Using the procedural style gtk_main() is not supported in GTK4 */
    /* So I will need to upgrade to g_application_run on the GtkApplication */
    gtk_main();

    g_resources_unregister(ui);
    return 0;
}
