#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gtk/axeiiloader_gtk.h"
#include "axeii_loader.h"

static enum {
    SEND_MODE = 1,
    RECEIVE_MODE
} mode = SEND_MODE;

static dev_info_t **devs = NULL;
static GObject *mididevs, *type,
               *sendfile, *sendloc, *senddetail, *recdir, *recloc, *rectype,
               *messagelabel, *progressbar, *startbutton;

/* Required by axeii_utils */
void progressCallback(int currentProgress) {
    char valAsString[16];
    sprintf(valAsString, "%d", currentProgress);
    if (currentProgress >= 0) {
        /*IupSetAttribute(messagelabel, "TITLE", "Doing transfer...");*/
        /*IupSetAttribute(progressbar, "VALUE", valAsString);*/
    } else if (currentProgress < 0) {
        /*IupSetAttribute(messagelabel, "TITLE", "Trying to capture header...");*/
    }
    if (currentProgress == 100) {
        /*IupSetAttribute(messagelabel, "TITLE", "Transfer complete!");*/
    }
}

void nameProvider(char *name) {
    char buf[256];
    sprintf(buf, "File saved as %s", name);
    /*IupSetAttribute(messagelabel, "TITLE", buf);*/
}

/* Utility */
void setLocationMinimums() {
    /*  */
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rectype));
}

void canEnableStart() {

}

/* Callbacks */

void type_cb() {
    setLocationMinimums();
}

void midi_cb() {
    canEnableStart();
}

void tabs_cb() {

}

void file_cb() {

}

void dir_cb() {

}

void rectype_cb() {
    setLocationMinimums();
}

void start_cb() {
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

    /* Set globally accessed widgets */
    mididevs     = gtk_builder_get_object(builder, "mididevs");
    type         = gtk_builder_get_object(builder, "type");
    sendfile     = gtk_builder_get_object(builder, "sendfile");
    sendloc      = gtk_builder_get_object(builder, "sendloc");
    senddetail   = gtk_builder_get_object(builder, "senddetail");
    recdir       = gtk_builder_get_object(builder, "recdir");
    recloc       = gtk_builder_get_object(builder, "recloc");
    rectype      = gtk_builder_get_object(builder, "rectype");
    messagelabel = gtk_builder_get_object(builder, "messagelabel");
    progressbar  = gtk_builder_get_object(builder, "progressbar");
    startbutton  = gtk_builder_get_object(builder, "startbutton");

    /* Locally accessed widgets */
    window = gtk_builder_get_object(builder, "window");

    /* Connect callbacks */
    window = gtk_builder_get_object(builder, "window");
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    /* NB: Using the procedural style gtk_main() is not supported in GTK4 */
    gtk_main();

    g_resources_unregister(ui);
    return 0;
}
