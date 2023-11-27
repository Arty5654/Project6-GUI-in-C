// app.h
#ifndef APP_H
#define APP_H

#include <gtk/gtk.h>

void display_system_info(GtkWidget *info_label);
void display_file_system_info(GtkWidget *info_label);
void display_resource_usage(GtkWidget *info_label);
void display_process_info(GtkWidget *info_label);

#endif