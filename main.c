/*
 * Project 6: Task Manager
 * main.c
 */

#include <gtk/gtk.h>

int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create the main window
    GtkWidget *window;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Task Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);


    // Connect the destroy event to the gtk_main_quit function
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Show the window
    gtk_widget_show_all(window);

    // Start the GTK main loop
    gtk_main();

    return 0;
}