#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gtk/axeiiloader_gtk.h"
#include "axeii_loader.h"

static dev_info_t **devs = NULL;
static GObject *mididevs, *type,
               *sendfile, *sendloc, *senddetail, *recdir, *recloc,
               *messagelabel, *progressbar, *startbutton;

/* Required by axeii_utils */
void progressCallback(int currentProgress) {

}

/* Required by axeii_utils */
void nameProvider(char *name) {
    char buf[256];
    sprintf(buf, "File saved as %s", name);
}

/* Callbacks */



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
