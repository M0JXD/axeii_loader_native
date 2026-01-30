/* COMPILE
 * cc `pkg-config --cflags gtk+-3.0` -o gtk_test ui_gtk.c `pkg-config --libs gtk+-3.0`
 */

#include <gtk/gtk.h>
#include "gtk/axeiiloader_gtk.h"

int main(int argc, char *argv[]) {
    GtkBuilder *builder;
    GObject *window;
    GObject *button;
    GError *error = NULL;
    GResource *ui = axeiiloader_gtk_get_resource();
    g_resources_register(ui);

    gtk_init (&argc, &argv);
    builder = gtk_builder_new_from_resource ("/m0jxd/axeiiloader/builder.ui");

    /* Connect signal handlers to the constructed widgets. */
    window = gtk_builder_get_object(builder, "window");
    g_signal_connect(window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    gtk_main();

    g_resources_unregister(ui);
    return 0;
}
