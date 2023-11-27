/*
 * Project 6: Task Manager
 * main.c
 */

#include "app.h"

#include <gtk/gtk.h>

int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create the main window
    GtkWidget *window;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Task Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    // Create a notebook to hold the tabs
    GtkWidget *notebook = gtk_notebook_new();

    // Create tabs
    GtkWidget *tab1 = gtk_label_new("System");
    GtkWidget *tab2 = gtk_label_new("Processes");
    GtkWidget *tab3 = gtk_label_new("Resources");
    GtkWidget *tab4 = gtk_label_new("File Systems");

    // Add tabs to the notebook
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab1, gtk_label_new("Deez Nuts"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab2, gtk_label_new("Processes"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab3, gtk_label_new("Resources"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab4, gtk_label_new("File Systems"));

    // Add the notebook to the main window
    gtk_container_add(GTK_CONTAINER(window), notebook);

    // Connect the destroy event to the gtk_main_quit function
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Show the window
    gtk_widget_show_all(window);

    // Start the GTK main loop
    gtk_main();

    return 0;
}
