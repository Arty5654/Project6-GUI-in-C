#include "app.h"
#include <gtk/gtk.h>

void on_switch_page(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data);

int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);

    GtkCssProvider *cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(cssProvider, "style.css", NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(cssProvider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);

    // Create the main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Task Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    // Create a notebook to hold the tabs
    GtkWidget *notebook = gtk_notebook_new();

    // Create four box widgets, one for each tab
    GtkWidget *box1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *box2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *box3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *box4 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    // Append the box widgets to the notebook pages
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box1, gtk_label_new("System"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box2, gtk_label_new("Processes"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box3, gtk_label_new("Resources"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box4, gtk_label_new("File Systems"));

    // Connect the "switch-page" signal to update the content when the tab changes
    g_signal_connect(notebook, "switch-page", G_CALLBACK(on_switch_page), notebook);

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

void on_switch_page(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data) {
    // Retrieve the box for the current page
     GtkWidget *current_box = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page_num);

     GList *children = gtk_container_get_children(GTK_CONTAINER(current_box));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    switch (page_num) {
        case 0:
            display_system_info(current_box);
            break;
        case 1:
            // Pass the box instead of the label to display_process_info
            display_process_info(current_box);
            break;
        case 2:
            display_resource_usage(current_box);
            break;
        case 3:
            display_file_system_info(current_box);
            break;
        default:
            break;
    }
}
