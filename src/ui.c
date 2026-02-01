/*    ui.c - GTK GUI interface to send/receive data from an Axe-FX II
 *    Copyright (C) 2025-2026  Jamie Drinkell
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, see <https://www.gnu.org/licenses/>.
 */

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
static int amount = 0;
static GObject *mididevs, *type, *tabs,
               *sendfile, *sendloc, *senddetail, *recdir, *recloc, *rectype,
               *messagelabel, *progressbar, *startbutton,
               *sendadjust, *recadjust;

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

    while (gtk_events_pending()) {
        gtk_main_iteration_do(FALSE);
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
    gchar *path, *axe_type;

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
    if (devs == NULL) {
        passed_checks = 0;
    }

    if (passed_checks) {
        gtk_widget_set_sensitive(GTK_WIDGET(startbutton), TRUE);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(startbutton), FALSE);
    }
}

/* Callbacks */
void box_cb(GtkComboBox* self, gpointer user_data) {
    checkAndEnable();
}

void tabs_cb(GtkNotebook* self, GtkWidget* page, guint page_num, gpointer user_data) {
    mode = page_num;
    checkAndEnable();
}

void file_cb(GtkFileChooserButton* self, gpointer user_data) {
    checkAndEnable();
}

void rectype_cb(GtkToggleButton* self, gpointer user_data) {
    checkAndEnable();
}

void start_cb(GtkButton* self, gpointer user_data) {
    char properties = 0, ret, *str, path[256];
    int location;
    gtk_widget_set_sensitive(GTK_WIDGET(startbutton), FALSE);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbar), 0.0);

    str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(type));

    properties |= IS_OG_UNIT;
    switch (str[0]) {
        case 'O':
            properties |= IS_OG_UNIT;
        break;
        case 'X':
            if (strlen(str) > 5) {
                properties |= IS_XLP_UNIT;
            } else {
                properties |= IS_XL_UNIT;
            }
        break;
    }

    gtk_label_set_text(GTK_LABEL(messagelabel), "Starting Transfer...");

    int midiIndex;
    str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(mididevs));
    for (midiIndex = 0; midiIndex < amount; midiIndex++) {
        if(!strcmp(str, devs[midiIndex]->hw_name)) {
            break;
        }
    }

    while (gtk_events_pending()) {
        gtk_main_iteration_do(FALSE);
    }

    initMIDI(devs[midiIndex]->hw_string);
    if (mode == SEND_MODE) {
        strcpy(path, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(sendfile)));
        location = gtk_adjustment_get_value(GTK_ADJUSTMENT(sendadjust));
        properties = detectFileProperties(path) | (properties & CLEAR_FILE);
        ret = sendFile(path, properties, location);
    } else {
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rectype));
        strcpy(path, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(recdir)));
        strcat(path, "/");  /* The library expects a trailing / on directory names */
        location = gtk_adjustment_get_value(GTK_ADJUSTMENT(recadjust));
        properties |= IS_VALID;
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rectype))) {
            properties |= IS_PRESET;
        } else {
            properties &= SET_IR;
        }
        ret = getFile(path, properties, location);
    }
    closeMIDI();

    if (ret == FILE_ERROR) {
        gtk_label_set_text(GTK_LABEL(messagelabel), "Couldn't open file!");
    } else if (ret == DESTINATION_UNIT_INVALID) {
        gtk_label_set_text(GTK_LABEL(messagelabel), "Can't send XL/XL+ file to OG/MKII!");
    } else if (ret == HEADER_LOCK_ISSUE) {
        gtk_label_set_text(GTK_LABEL(messagelabel), "Couldn't lock onto header!");
    } else if (ret == PROPERTIES_INVALID) {
        gtk_label_set_text(GTK_LABEL(messagelabel), "File and/or values are not valid!");
    }

    checkAndEnable();
}

/* MAIN */

int main(int argc, char *argv[]) {
    int index = 0;
    GtkBuilder *builder;
    GObject *window;
    GResource *ui = axeiiloader_gtk_get_resource();
    g_resources_register(ui);

    if (devs != NULL) freeAxeMidiDevs(devs);
    devs = getAxeMidiDevs(&amount, &index);

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

    /* Add midi devices to list */
    if (amount > 0) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mididevs), devs[index]->hw_name);
        if (amount > 1) {
            for (int i = 0; i < amount; i++) {
                if (amount != index) {
                    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(mididevs), devs[i]->hw_name);
                }
            }
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(mididevs), 0);
    }

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
    freeAxeMidiDevs(devs);
    devs = NULL;
    g_resources_unregister(ui);
    return 0;
}
