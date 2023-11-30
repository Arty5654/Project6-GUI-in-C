#include <gtk/gtk.h>
#include <sys/statvfs.h>
#include <stdio.h>

void display_file_system_info(GtkWidget *info_box) {
    GtkListStore *store = gtk_list_store_new(5,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING,
                                             G_TYPE_STRING);

    FILE *mntent_file = fopen("/proc/mounts", "r");
    if (mntent_file != NULL) {
        char line[1024];
        while (fgets(line, sizeof(line), mntent_file)) {
            gchar device[256], mountpoint[256], fstype[256];
            if (sscanf(line, "%255s %255s %255s %*s %*s %*s", device, mountpoint, fstype) == 3) {
                struct statvfs vfs;
                if (statvfs(mountpoint, &vfs) == 0) {
                    gfloat total = (gfloat)vfs.f_blocks * (gfloat)vfs.f_frsize / (1024.0f * 1024.0f * 1024.0f);
                    gfloat free = (gfloat)vfs.f_bfree * (gfloat)vfs.f_frsize / (1024.0f * 1024.0f * 1024.0f);
                    gfloat used = total - free;
                    gfloat available = (gfloat)vfs.f_bavail * (gfloat)vfs.f_frsize / (1024.0f * 1024.0f * 1024.0f);

                    gtk_list_store_insert_with_values(store, NULL, -1,
                                                      0, device,
                                                      1, mountpoint,
                                                      2, fstype,
                                                      3, g_strdup_printf("%.2f GB", total),
                                                      4, g_strdup_printf("%.2f GB / %.2f GB", used, available),
                                                      -1);
                }
            }
        }
        fclose(mntent_file);
    }

    GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);

    // Add columns to the GtkTreeView
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1,
                                                "Device", renderer,
                                                "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1,
                                                "Directory", renderer,
                                                "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1,
                                                "Type", renderer,
                                                "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1,
                                                "Total", renderer,
                                                "text", 3, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view), -1,
                                                "Used / Available", renderer,
                                                "text", 4, NULL);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);

    // Remove any previous widgets in the info_box
    GList *children = gtk_container_get_children(GTK_CONTAINER(info_box));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    // Pack the scrolled window into the info_box
    gtk_box_pack_start(GTK_BOX(info_box), scrolled_window, TRUE, TRUE, 0);

    gtk_widget_show_all(info_box);
}
