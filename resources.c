// resources.c
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>

void display_resource_usage(GtkWidget *info_label) {
    // Create a label to display resource usage information
    GtkWidget *label = gtk_label_new(NULL);

    // Get system information
    struct sysinfo si;
    if (sysinfo(&si) != 0) {
        perror("Error getting system information");
        return;
    }

    // Format and display the information
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Uptime: %ld seconds\nTotal RAM: %lu KB\nFree RAM: %lu KB",
             si.uptime, si.totalram / 1024, si.freeram / 1024);

    gtk_label_set_text(GTK_LABEL(label), buffer);

    // Pack the label into the box
    gtk_box_pack_start(GTK_BOX(info_label), label, FALSE, FALSE, 5);

    // Show the label
    gtk_widget_show_all(info_label);
}
