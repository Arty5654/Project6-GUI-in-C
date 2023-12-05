#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <pwd.h>
#include <ctype.h>

#ifndef GTK_RESPONSE_USER_START
#define GTK_RESPONSE_USER_START (GTK_RESPONSE_DELETE_EVENT + 1)
#endif

#define RESPONSE_STOP (GTK_RESPONSE_USER_START + 1)
#define RESPONSE_CONTINUE (GTK_RESPONSE_USER_START + 2)
#define RESPONSE_KILL (GTK_RESPONSE_USER_START + 3)
#define RESPONSE_LIST_MEMORY_MAPS (GTK_RESPONSE_USER_START + 4)
#define RESPONSE_LIST_OPEN_FILES (GTK_RESPONSE_USER_START + 5)

#define COLUMN_NAME 0
#define COLUMN_STATUS 1
#define COLUMN_PID 2
#define COLUMN_MEMORY 3
#define COLUMN_USER 4

void add_tree_view_column(GtkWidget *tree_view, const gchar *title, gint column_id);
void free_process_list(GList *process_list);
void refresh_process_list(GtkButton *button, gpointer user_data);
static gchar* get_process_name(pid_t pid);
static gchar* get_process_status(pid_t pid);
static gfloat get_process_memory(pid_t pid);
static gint get_process_cpu(pid_t pid);
static GtkTreeModelFilter *filter_model = NULL;

typedef struct {
    long pid;             
    gchar *user;          
    gchar *name;          
    gchar *status;        
    gfloat memory;        
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

gboolean get_process_details(long pid, gchar** out_name, gchar** out_status, gfloat* out_memory, gchar** out_user) {
    gchar *status_path = g_strdup_printf("/proc/%ld/status", pid);
    gchar *status_contents = NULL;
    GError *error = NULL;

    // Read the status file for name and status
    if (!g_file_get_contents(status_path, &status_contents, NULL, &error)) {
        g_error_free(error);
        g_free(status_path);
        return FALSE;
    }

    gchar **lines = g_strsplit(status_contents, "\n", -1);
    uid_t uid = -1;
    for (gint i = 0; lines[i] != NULL; i++) {
        if (g_str_has_prefix(lines[i], "Name:")) {
            *out_name = g_strdup(g_strstrip(lines[i] + 5));
        } else if (g_str_has_prefix(lines[i], "State:")) {
            *out_status = g_strdup(g_strstrip(lines[i] + 6));
        } else if (g_str_has_prefix(lines[i], "Uid:")) {
            gchar **tokens = g_strsplit(lines[i], "\t", -1);
            uid = (uid_t)g_ascii_strtoull(tokens[1], NULL, 10);
            g_strfreev(tokens);
        }
    }
    g_strfreev(lines);
    g_free(status_contents);
    g_free(status_path);

    // Get the username from UID
    if (uid != -1) {
        *out_user = get_username_from_uid(uid);
    } else {
        *out_user = g_strdup("Unknown");
    }

    // Memory usage is read from /proc/[pid]/statm
    gchar *mem_path = g_strdup_printf("/proc/%ld/statm", pid);
    gchar *mem_contents = NULL;

    if (g_file_get_contents(mem_path, &mem_contents, NULL, NULL)) {
        long page_size = sysconf(_SC_PAGESIZE); // Get system page size
        *out_memory = g_ascii_strtoll(mem_contents, NULL, 10) * page_size / 1024.0 / 1024.0; // Convert to MiB
        g_free(mem_contents);
    } else {
        *out_memory = 0.0; // In case of failure to read memory info
    }
    g_free(mem_path);

    return TRUE;
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
        gchar *name, *status, *user;
        gfloat memory;
        gboolean success = get_process_details(pid, &name, &status, &memory, &user); // Updated to pass user

        // If the details were successfully retrieved, insert them into the store
        if (success) {
            gtk_list_store_insert_with_values(store, NULL, -1,
                                              0, name,
                                              1, status,
                                              2, (gint)pid,
                                              3, memory,
                                              4, user, // Include user data in the list store
                                              -1);
            g_free(name);
            g_free(status);
            g_free(user); // Free the user string
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
                           0, &info->name,
                           1, &info->status,
                           2, &info->pid,
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
    return name; 
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
    //gfloat memory = g_ascii_strtoll(tokens[1], NULL, 10) * (getpagesize() / 1024.0 / 1024.0);
    gfloat memory = g_ascii_strtoll(tokens[1], NULL, 10) * (getpagesize() / 1024.0);
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

void show_process_dialog(pid_t pid) {
    // Placeholder for actual implementation
    printf("Dialog for PID %d would be shown here.\n", pid);
}

// Function to stop a process
void stop_process(pid_t pid) {
    if (kill(pid, SIGSTOP) == -1) {
        perror("Error stopping process");
    } else {
        printf("Process %d stopped\n", pid);
    }
}

// Function to continue a process
void continue_process(pid_t pid) {
    if (kill(pid, SIGCONT) == -1) {
        perror("Error continuing process");
    } else {
        printf("Process %d continued\n", pid);
    }
}

// Function to kill a process
void kill_process(pid_t pid) {
    if (kill(pid, SIGKILL) == -1) {
        perror("Error killing process");
    } else {
        printf("Process %d killed\n", pid);
    }
}

// Helper function to read a line from a file
static gchar* read_first_line(const gchar* filepath) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        return NULL;
    }
    gchar buffer[256];
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        fclose(file);
        return NULL;
    }
    fclose(file);
    return g_strdup(buffer);
}

// Function to get virtual memory
static gchar* get_virtual_memory(pid_t pid) {
    gchar *filepath = g_strdup_printf("/proc/%d/statm", pid);
    gchar *line = read_first_line(filepath);
    gchar **tokens = g_strsplit(line, " ", -1);
    gchar *vm = g_strdup(tokens[0]); 
    g_free(filepath);
    g_free(line);
    g_strfreev(tokens);
    return vm;
}

// Function to get resident memory
static gchar* get_resident_memory(pid_t pid) {
    gchar *filepath = g_strdup_printf("/proc/%d/statm", pid);
    gchar *line = read_first_line(filepath);
    gchar **tokens = g_strsplit(line, " ", -1);
    gchar *rm = g_strdup(tokens[1]); 
    g_free(filepath);
    g_free(line);
    g_strfreev(tokens);
    return rm;
}

// Function to get shared memory
static gchar* get_shared_memory(pid_t pid) {
    gchar *filepath = g_strdup_printf("/proc/%d/statm", pid);
    gchar *line = read_first_line(filepath);
    gchar **tokens = g_strsplit(line, " ", -1);
    gchar *sm = g_strdup(tokens[2]); 
    g_free(filepath);
    g_free(line);
    g_strfreev(tokens);
    return sm;
}

// Function to get CPU time
static gchar* get_cpu_time(pid_t pid) {
    gchar *filepath = g_strdup_printf("/proc/%d/stat", pid);
    gchar *line = read_first_line(filepath);
    gchar **tokens = g_strsplit(line, " ", -1);
    long utime = atol(tokens[13]);
    long stime = atol(tokens[14]);
    gchar *cpu_time = g_strdup_printf("%ld", utime + stime); 
    g_free(filepath);
    g_free(line);
    g_strfreev(tokens);
    return cpu_time;
}

// Function to get start time
static gchar* get_start_time(pid_t pid) {
    gchar *filepath = g_strdup_printf("/proc/%d/stat", pid);
    gchar *line = read_first_line(filepath);
    gchar **tokens = g_strsplit(line, " ", -1);
    long start_time = atol(tokens[21]); 
    g_free(filepath);
    g_free(line);
    g_strfreev(tokens);
    return g_strdup_printf("%ld", start_time);
}




void show_process_details(GtkTreeModel *model, GtkTreeIter *iter) {
    gchar *name, *status, *user;
    gint pid;
    gfloat memory;
    gtk_tree_model_get(model, iter, COLUMN_NAME, &name, COLUMN_STATUS, &status, COLUMN_PID, &pid, COLUMN_MEMORY, &memory, COLUMN_USER, &user, -1);

    // Get additional details
    gchar *virtual_memory = get_virtual_memory(pid);
    gchar *resident_memory = get_resident_memory(pid);
    gchar *shared_memory = get_shared_memory(pid);
    gchar *cpu_time = get_cpu_time(pid);
    gchar *start_time = get_start_time(pid);

    // Ensure the user string is valid UTF-8
    if (!g_utf8_validate(user, -1, NULL)) {
        g_free(user);
        user = g_strdup("Invalid UTF-8");
    }

    // Create a dialog to display the details
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Process Details", NULL, GTK_DIALOG_MODAL, "_Close", GTK_RESPONSE_CLOSE, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);

    // Create a label to show the details
    gchar *details = g_strdup_printf("Name: %s\nUser: %s\nStatus: %s\nPID: %d\nMemory: %.2f MiB\nVirtual Memory: %s\nResident Memory: %s\nShared Memory: %s\nCPU Time: %s\nStart Time: %s", 
                                     name, user, status, pid, memory, virtual_memory, resident_memory, shared_memory, cpu_time, start_time);
    GtkWidget *label = gtk_label_new(details);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    // Run the dialog and wait for a response
    gtk_dialog_run(GTK_DIALOG(dialog));

    // Clean up
    gtk_widget_destroy(dialog);
    g_free(details);
    g_free(name);
    g_free(status);
    g_free(user);
    g_free(virtual_memory);
    g_free(resident_memory);
    g_free(shared_memory);
    g_free(cpu_time);
    g_free(start_time);
}


// Function to list open files of a process
void list_open_files(pid_t pid) {
    gchar *cmdline = g_strdup_printf("lsof -p %d", pid);
    gchar *output = NULL;
    GError *error = NULL;

    // Execute the lsof command and get the output
    if (!g_spawn_command_line_sync(cmdline, &output, NULL, NULL, &error)) {
        g_warning("Failed to run lsof: %s", error->message);
        g_error_free(error);
        g_free(cmdline);
        return;
    }
    g_free(cmdline);

    // Create a dialog to display the open files
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Open Files",
        NULL, // parent window
        GTK_DIALOG_MODAL,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );

    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);

    // Create a scrolled window with a text view
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    // Add the scrolled window to the dialog
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       scrolled_window, TRUE, TRUE, 0);

    // Set the text buffer of the text view to the output of lsof
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, output, -1);

    // Show all widgets in the dialog
    gtk_widget_show_all(dialog);

    // Run the dialog and wait for the user to respond
    gtk_dialog_run(GTK_DIALOG(dialog));

    // Clean up
    gtk_widget_destroy(dialog);
    g_free(output);
}

// Function to list memory maps of a process
void list_memory_maps(pid_t pid) {
    gchar *filepath = g_strdup_printf("/proc/%d/maps", pid);
    gchar *contents = NULL;
    GError *error = NULL;

    // Read the contents of the maps file
    if (!g_file_get_contents(filepath, &contents, NULL, &error)) {
        g_warning("Failed to read memory maps: %s", error->message);
        g_error_free(error);
        g_free(filepath);
        return;
    }
    g_free(filepath);

    // Create a dialog to display the memory maps
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Memory Maps",
        NULL, // parent window
        GTK_DIALOG_MODAL,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );

     gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 400);

    // Create a scrolled window with a text view
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    // Add the scrolled window to the dialog
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
                       scrolled_window, TRUE, TRUE, 0);

    // Set the text buffer of the text view to the contents of the memory maps
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, contents, -1);

    // Show all widgets in the dialog
    gtk_widget_show_all(dialog);

    // Run the dialog and wait for the user to respond
    gtk_dialog_run(GTK_DIALOG(dialog));

    // Clean up
    gtk_widget_destroy(dialog);
    g_free(contents);
}





void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata) {
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        // Get the PID from the model
        gint pid;
        gtk_tree_model_get(model, &iter, COLUMN_PID, &pid, -1);

        show_process_details(model, &iter);

        

        // Create a dialog with buttons for different actions
        GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "Process Actions",
            NULL, // parent window
            GTK_DIALOG_MODAL,
            "_Stop", RESPONSE_STOP,
            "_Continue", RESPONSE_CONTINUE,
            "_Kill", RESPONSE_KILL,
            "_List Memory Maps", RESPONSE_LIST_MEMORY_MAPS,
            "_List Open Files", RESPONSE_LIST_OPEN_FILES,
            "_Close", GTK_RESPONSE_CLOSE,
            NULL
        );

        // Run the dialog and wait for the user to respond
        gint result = gtk_dialog_run(GTK_DIALOG(dialog));

        // Handle the response
        switch (result) {
            case RESPONSE_STOP:
                stop_process(pid);
                break;
            case RESPONSE_CONTINUE:
                continue_process(pid);
                break;
            case RESPONSE_KILL:
                kill_process(pid);
                break;
            case RESPONSE_LIST_MEMORY_MAPS:
                list_memory_maps(pid); // Implement this function
                break;
            case RESPONSE_LIST_OPEN_FILES:
                list_open_files(pid); // Implement this function
                break;
            case GTK_RESPONSE_CLOSE:
                // Close the dialog
                break;
            default:
                // Handle unexpected response
                break;
        }

        // Destroy the dialog once finished
        gtk_widget_destroy(dialog);
    }
}

// Function to populate the GTK list store with the process information
void populate_list_store(GtkListStore *store, GList *process_list) {
    gtk_list_store_clear(store);

    for (GList *iter = process_list; iter != NULL; iter = iter->next) {
        ProcessInfo *info = iter->data;
        gtk_list_store_insert_with_values(store, NULL, -1,
                                          0, info->name,
                                          1, info->status,
                                          2, info->user,
                                          3, info->pid,
                                          4, info->memory,
                                          -1);
    }
}

// Function to filter processes based on search query
static gboolean process_filter_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    const gchar *search_query = data;
    gchar *name;
    gboolean visible = FALSE;

    if (!search_query || !*search_query) {
        return TRUE; // Show all rows if no search query
    }

    gtk_tree_model_get(model, iter, COLUMN_NAME, &name, -1);

    if (name != NULL) {
        visible = g_strstr_len(name, -1, search_query) != NULL;
        g_free(name);
    }

    return visible;
}

// Callback for search entry changes
void search_entry_changed_cb(GtkSearchEntry *search_entry, gpointer user_data) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(search_entry));
    gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(user_data), process_filter_func, (gpointer)text, NULL);
    gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(user_data));
}

// Function to create and display the tree view for process information
void display_process_info(GtkWidget *box, gboolean only_user_processes) {
    GtkListStore *store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_FLOAT, G_TYPE_STRING);
    GtkTreeModelFilter *filter_model = GTK_TREE_MODEL_FILTER(gtk_tree_model_filter_new(GTK_TREE_MODEL(store), NULL));

    // Set up the search entry
    GtkWidget *search_entry = gtk_search_entry_new();
    g_signal_connect(search_entry, "search-changed", G_CALLBACK(search_entry_changed_cb), filter_model);
    gtk_box_pack_start(GTK_BOX(box), search_entry, FALSE, FALSE, 0);

    // Create the tree view with the filter model
    GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(filter_model));
    g_object_unref(store); // Release the list store as the filter model holds a reference to it

    // Add columns to the tree view
    add_tree_view_column(tree_view, "Process Name", COLUMN_NAME);
    add_tree_view_column(tree_view, "Status", COLUMN_STATUS);
    add_tree_view_column(tree_view, "PID", COLUMN_PID);
    add_tree_view_column(tree_view, "Memory (MiB)", COLUMN_MEMORY);
    add_tree_view_column(tree_view, "User", COLUMN_USER);

    // Populate the list store
    get_process_info(store, only_user_processes);

    // Scrolled window for the tree view
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
    gtk_box_pack_start(GTK_BOX(box), scrolled_window, TRUE, TRUE, 0);

    // Row activated signal
    g_signal_connect(tree_view, "row-activated", G_CALLBACK(on_row_activated), NULL);

    // Refresh button
    GtkWidget *refresh_button = gtk_button_new_with_label("Refresh");
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(refresh_process_list), store);
    gtk_box_pack_start(GTK_BOX(box), refresh_button, FALSE, FALSE, 0);

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
