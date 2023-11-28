// system_info.c
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

void display_system_info(GtkWidget *info_label) {
    // Read /etc/os-release file
    FILE *file = fopen("/etc/os-release", "r");
    if (file != NULL) {
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), file) != NULL) {
            if (strstr(buffer, "PRETTY_NAME=") != NULL) {
                // Extract and display the OS name
                gtk_label_set_text(GTK_LABEL(info_label), buffer + sizeof("PRETTY_NAME=") - 1);
                break;
            }
        }
        fclose(file);
    } else {
        printf("Couldn't read file\n");
    }
}
