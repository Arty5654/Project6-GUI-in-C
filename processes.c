#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <ctype.h>

void add_tree_view_column(GtkWidget *tree_view, const gchar *title, gint column_id);
void free_process_list(GList *process_list);
void refresh_process_list(GtkButton *button, gpointer user_data);
static gchar* get_process_name(pid_t pid);
static gchar* get_process_status(pid_t pid);
static gfloat get_process_memory(pid_t pid);
static gint get_process_cpu(pid_t pid);

typedef struct {
    long pid;             // Process ID
    gchar *user;          // User name
    gchar *name;          // Process name
    gchar *status;        // Process status
    gfloat memory;        // Memory usage in MiB
    // CPU usage would be a float representing percentage, but it's complex to calculate
} ProcessInfo;

gchar* get_process_user(uid_t uid);

gchar* get_username_from_uid(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        return g_strdup(pw->pw_name);
    }
    return g_strdup("Unknown");
}

gchar* read_line_from_file(const gchar* filepath) {
    gchar *line = NULL;
    size_t len = 0;
    ssize_t read;
    FILE *file = fopen(filepath, "r");

    if (file) {
        read = getline(&line, &len, file);
        if (read != -1) {
            // Strip newline character
            if (line[read - 1] == '\n') {
                line[read - 1] = '\0';
            }
        } else {
            g_free(line);
            line = NULL;
        }
        fclose(file);
    }
    return line;
}

// Function to get the user name of the process owner
gchar* get_process_user(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    if (pw != NULL) {
        return g_strdup(pw->pw_name);
    } else {
        return g_strdup("Unknown");
    }
}

gboolean get_process_details(long pid, gchar** out_name, gchar** out_status, gfloat* out_memory) {
    gchar *path = g_strdup_printf("/proc/%ld/status", pid);
    gchar *contents = NULL;
    GError *error = NULL;

    if (!g_file_get_contents(path, &contents, NULL, &error)) {
        // Handle error, e.g., print it
        g_error_free(error);
        g_free(path);
        return FALSE;
    }

    gchar **lines = g_strsplit(contents, "\n", -1);
    for (gint i = 0; lines[i] != NULL; i++) {
        if (g_str_has_prefix(lines[i], "Name:")) {
            *out_name = g_strdup(g_strstrip(lines[i] + 5));
        } else if (g_str_has_prefix(lines[i], "State:")) {
            *out_status = g_strdup(g_strstrip(lines[i] + 6));
        }
        // ... Other fields can be parsed similarly
    }
    g_strfreev(lines);
    g_free(contents);
    g_free(path);

    // Memory usage is read from /proc/[pid]/statm
    path = g_strdup_printf("/proc/%ld/statm", pid);
    if (g_file_get_contents(path, &contents, NULL, NULL)) {
        // The first number in statm is the total program size
        long page_size = sysconf(_SC_PAGESIZE); // Get system page size
        *out_memory = g_ascii_strtoll(contents, NULL, 10) * page_size / 1024.0 / 1024.0; // Convert to MiB
        g_free(contents);
    }
    g_free(path);

    return TRUE; // Name and status are successfully retrieved
}


void get_process_info(GtkListStore *store, gboolean only_user_processes) {
    DIR *dir;
    struct dirent *entry;
    uid_t user_uid = getuid();

    dir = opendir("/proc");
    if (dir == NULL) {
        perror("Failed to open /proc directory");
        return;
    }

    // Clear existing entries in the list store
    gtk_list_store_clear(store);

    while ((entry = readdir(dir)) != NULL) {
        // Only consider numeric directories
        if (entry->d_type != DT_DIR || !isdigit(entry->d_name[0])) {
            continue;
        }

        long pid = strtol(entry->d_name, NULL, 10);
        gchar *name, *status;
        gfloat memory;
        gboolean success = get_process_details(pid, &name, &status, &memory);

        // If the details were successfully retrieved, insert them into the store
        if (success) {
            struct passwd *pw = getpwuid(user_uid);
            const gchar *user_name = pw ? pw->pw_name : "Unknown";
            
            // Insert the data into the list store
            gtk_list_store_insert_with_values(store, NULL, -1,
                                              0, (gint)pid,
                                              1, name,
                                              2, status,
                                              3, memory,
                                              -1);
            g_free(name);
            g_free(status);
        }
    }

    closedir(dir);
}



// Function to get process information from the /proc filesystem
GList* get_all_processes(gboolean only_user_processes) {
    GList *process_list = NULL;
    GtkListStore *store = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT);
    
    get_process_info(store, only_user_processes);

    // Convert GtkListStore to GList
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);
    while (valid) {
        ProcessInfo *info = g_new0(ProcessInfo, 1);
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                           0, &info->pid,
                           1, &info->name,
                           2, &info->status,
                           3, &info->memory,
                           -1);
        process_list = g_list_prepend(process_list, info);
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter);
    }

    // The store is no longer needed
    g_object_unref(store);

    return g_list_reverse(process_list);
}


static gchar* get_process_name(pid_t pid) {
    gchar *filepath = g_strdup_printf("/proc/%d/comm", pid);
    gchar *name = NULL;
    if (g_file_get_contents(filepath, &name, NULL, NULL)) {
        g_strchomp(name); // Remove newline character
    } else {
        name = g_strdup("Unknown");
    }
    g_free(filepath);
    return name; // This must be freed by the caller
}


static gchar* get_process_status(pid_t pid) {
    gchar *filepath = g_strdup_printf("/proc/%d/status", pid);
    gchar *contents = NULL;
    gchar *status = NULL;
    if (g_file_get_contents(filepath, &contents, NULL, NULL)) {
        gchar *status_line = g_strstr_len(contents, -1, "State:");
        status = status_line ? g_strndup(status_line + 7, 1) : g_strdup("?");
        g_free(contents);
    } else {
        status = g_strdup("Unknown");
    }
    g_free(filepath);
    return status; // This must be freed by the caller
}


static gfloat get_process_memory(pid_t pid) {
    gchar *filepath = g_strdup_printf("/proc/%d/statm", pid);
    gchar *contents = NULL;
    g_file_get_contents(filepath, &contents, NULL, NULL);
    gchar **tokens = g_strsplit(contents, " ", -1);
    gfloat memory = g_ascii_strtoll(tokens[1], NULL, 10) * (getpagesize() / 1024.0 / 1024.0);
    g_strfreev(tokens);
    g_free(contents);
    g_free(filepath);
    return memory;
}

static gint get_process_cpu(pid_t pid) {
    gchar *filepath = g_strdup_printf("/proc/%d/stat", pid);
    gchar *contents = NULL;
    g_file_get_contents(filepath, &contents, NULL, NULL);
    gchar **tokens = g_strsplit(contents, " ", -1);
    // utime is at position 13 and stime at position 14 in the stat file
    gint utime = g_ascii_strtoll(tokens[13], NULL, 10);
    gint stime = g_ascii_strtoll(tokens[14], NULL, 10);
    g_strfreev(tokens);
    g_free(contents);
    g_free(filepath);
    // This is a simplified representation of the CPU time, not usage percentage
    return utime + stime;
}

// Function to populate the GTK list store with the process information
void populate_list_store(GtkListStore *store, GList *process_list) {
    gtk_list_store_clear(store);

    for (GList *iter = process_list; iter != NULL; iter = iter->next) {
        ProcessInfo *info = iter->data;
        gtk_list_store_insert_with_values(store, NULL, -1,
                                          0, info->pid,
                                          1, info->name,
                                          2, info->status,
                                          3, info->user,
                                          4, info->memory,
                                          -1);
    }
}

// Function to create and display the tree view for process information
void display_process_info(GtkWidget *box, gboolean only_user_processes) {
    // Create the list store with appropriate columns for process information
    GtkListStore *store = gtk_list_store_new(5, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_INT);

    // Create the tree view and add it to the list store
    GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store); // The tree view has its own reference now

    // Add columns to the GtkTreeView for PID, Name, Status, Memory, and CPU
    add_tree_view_column(tree_view, "PID", 0);
    add_tree_view_column(tree_view, "Name", 1);
    add_tree_view_column(tree_view, "Status", 2);
    add_tree_view_column(tree_view, "Memory (MiB)", 3);
    add_tree_view_column(tree_view, "CPU Time", 4);

    // Populate the initial process list
    get_process_info(store, only_user_processes);

    // Create a scrolled window and add the tree view to it
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);

    // Pack the scrolled window into the box
    gtk_box_pack_start(GTK_BOX(box), scrolled_window, TRUE, TRUE, 0);

    // Add a refresh button
    GtkWidget *refresh_button = gtk_button_new_with_label("Refresh");
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(refresh_process_list), store);
    gtk_box_pack_start(GTK_BOX(box), refresh_button, FALSE, FALSE, 0);

    // Show all widgets
    gtk_widget_show_all(box);
}

// Add a column to the tree view
void add_tree_view_column(GtkWidget *tree_view, const gchar *title, gint column_id) {
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(title, renderer, "text", column_id, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
}

void refresh_process_list(GtkButton *button, gpointer user_data) {
    GtkListStore *store = GTK_LIST_STORE(user_data);
    get_process_info(store, TRUE);
}

// Free the process list and its contents
void free_process_list(GList *process_list) {
    for (GList *iter = process_list; iter != NULL; iter = iter->next) {
        ProcessInfo *info = iter->data;
        g_free(info->name);
        g_free(info->status);
        g_free(info->user);
        g_free(info);
    }
    g_list_free(process_list);
}
