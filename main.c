/*
 * Project 6: Task Manager
 * main.c
 */

#include "app.h"

#include <gtk/gtk.h>

void on_switch_page(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data);

/*
 * main function to drive the GUI
 */

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

  // Create separate label widgets for each box
  GtkWidget *label1 = gtk_label_new(NULL);
  GtkWidget *label2 = gtk_label_new(NULL);
  GtkWidget *label3 = gtk_label_new(NULL);
  GtkWidget *label4 = gtk_label_new(NULL);

  // Set the initial content for each label
  display_system_info(label1);
  display_process_info(label2);
  display_resource_usage(label3);
  display_file_system_info(label4);

  // Connect the "switch-page" signal to update the content when the tab changes
  g_signal_connect(notebook, "switch-page", G_CALLBACK(on_switch_page), label1);

  // Create four box widgets, one for each tab
  GtkWidget *box1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  GtkWidget *box2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  GtkWidget *box3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  GtkWidget *box4 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

  // Add the label widget and any other widgets to each box widget
  gtk_box_pack_start(GTK_BOX(box1), label1, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(box2), label2, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(box3), label3, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(box4), label4, FALSE, FALSE, 5);

  // Append the box widgets to the notebook pages
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box1, gtk_label_new("System"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box2, gtk_label_new("Processes"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box3, gtk_label_new("Resources"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), box4, gtk_label_new("File Systems"));

  // Add the notebook to the main window
  gtk_container_add(GTK_CONTAINER(window), notebook);

  // Connect the destroy event to the gtk_main_quit function
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  // Show the window
  gtk_widget_show_all(window);

  // Start the GTK main loop
  gtk_main();

  return 0;
} /* main() */

/*
 * Callback function to update content when the tab changes
 */

void on_switch_page(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data) {
  
  // Callback function to update content when the tab changes
  GtkWidget *info_label = GTK_WIDGET(user_data);

  switch (page_num) {
    case 0:
      display_system_info(info_label);
      break;
    case 1:
      display_process_info(info_label);
      break;
    case 2:
      display_resource_usage(info_label);
      break;
    case 3:
      display_file_system_info(info_label);
      break;
    default:
      break;
  }
} /* on_switch_page() */
