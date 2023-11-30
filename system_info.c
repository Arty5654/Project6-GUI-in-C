// system_info.c
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>

/*
 * Function to get all system information
 */

void display_system_info(GtkWidget *info_box) {
  // Buffer to accumulate information
  char info_buffer[4096];
  info_buffer[0] = '\0'; // Ensure the buffer is initially empty

  // Read /etc/os-release file for OS name and version
  FILE *os_release_file = fopen("/etc/os-release", "r");
  if (os_release_file != NULL) {
    char os_release_buffer[1024];
    while (fgets(os_release_buffer, sizeof(os_release_buffer), os_release_file) != NULL) {
      // Extract and append the OS name to the buffer
      if (strstr(os_release_buffer, "PRETTY_NAME=") != NULL) {
        char os_name[1024];
        if (sscanf(os_release_buffer, "PRETTY_NAME=\"%[^ ]", os_name) == 1) {
          snprintf(info_buffer + strlen(info_buffer), sizeof(info_buffer) - strlen(info_buffer), "%s\n", os_name);
        }
      }
      // Extract and append the OS version to the buffer
      else if (strstr(os_release_buffer, "VERSION=") != NULL) {
        char os_version[1024];
        if (sscanf(os_release_buffer, "VERSION=\"%[^\"]\"", os_version) == 1) {
          snprintf(info_buffer + strlen(info_buffer), sizeof(info_buffer) - strlen(info_buffer), "Version: %s\n", os_version);
        }
      }
    }
    fclose(os_release_file);
  } else {
    printf("Couldn't read /etc/os-release\n");
  }

  // Run "uname -srm" to get kernel version
  FILE *uname_pipe = popen("uname -srm", "r");
  if (uname_pipe != NULL) {
    char kernel_version_buffer[1024];
    if (fgets(kernel_version_buffer, sizeof(kernel_version_buffer), uname_pipe) != NULL) {
      // Extract and append the kernel version to the buffer
      snprintf(info_buffer + strlen(info_buffer), sizeof(info_buffer) - strlen(info_buffer), "Kernel: %s", kernel_version_buffer);
    }
    pclose(uname_pipe);
  } else {
    printf("Couldn't run uname command\n");
  }

  // Read /proc/meminfo file for memory information
  FILE *meminfo_file = fopen("/proc/meminfo", "r");
  if (meminfo_file != NULL) {
    char meminfo_buffer[1024];
    while (fgets(meminfo_buffer, sizeof(meminfo_buffer), meminfo_file) != NULL) {
      // Extract and append the total memory to the buffer
      if (strstr(meminfo_buffer, "MemTotal:") != NULL) {
        unsigned long total_memory_kb;
        if (sscanf(meminfo_buffer, "MemTotal: %lu kB", &total_memory_kb) == 1) {
          double total_memory_gb = (double)total_memory_kb / (1024 * 1024);
          // Append the total memory information to the buffer in GiB
          snprintf(info_buffer + strlen(info_buffer), sizeof(info_buffer) - strlen(info_buffer), "\nMemory: %.2f GiB\n", total_memory_gb);
        }
      }
    }
    fclose(meminfo_file);
  } else {
    printf("Couldn't read /proc/meminfo\n");
  }

  // Read processor information from lscpu command
  FILE *lscpu_file = popen("lscpu", "r");
  if (lscpu_file != NULL) {
    char lscpu_buffer[1024];
    while (fgets(lscpu_buffer, sizeof(lscpu_buffer), lscpu_file) != NULL) {
      // Check for the line containing the processor information
      if (strstr(lscpu_buffer, "Model name:") != NULL) {
        // Skip leading whitespace
        char *model_name_start = strchr(lscpu_buffer, ':');
        if (model_name_start != NULL) {
          model_name_start++; // Move past the colon
          while (*model_name_start == ' ' || *model_name_start == '\t') {
            model_name_start++; // Skip leading whitespace
          }

          // Append the processor information to the buffer
          snprintf(info_buffer + strlen(info_buffer), sizeof(info_buffer) - strlen(info_buffer), "Processor: %s\n", model_name_start);
        }
        break; // Assuming you only want the first occurrence
      }
    }
    pclose(lscpu_file);
  } else {
      printf("Couldn't run lscpu command\n");
  }

  // Get disk space information using statvfs
  struct statvfs disk_info;
  if (statvfs("/", &disk_info) == 0) {
    unsigned long total_disk_space = disk_info.f_blocks * disk_info.f_frsize;
    // Convert total disk space to MB for example
    double total_disk_space_gb = (double)total_disk_space / (1024 * 1024 * 1024);
    // Append the total disk space information to the buffer in GiB
    snprintf(info_buffer + strlen(info_buffer), sizeof(info_buffer) - strlen(info_buffer), "Total Disk Space: %.2f GiB", total_disk_space_gb);
  } else {
    printf("Couldn't get disk space information\n");
  }

  // Create a GtkLabel for the system info
  GtkWidget *info_label = gtk_label_new(info_buffer); // No conflict now, as we've changed the parameter name
  gtk_label_set_selectable(GTK_LABEL(info_label), TRUE);
  gtk_label_set_xalign(GTK_LABEL(info_label), 0.0);  // Align the label text to the left

  // Remove any previous widgets in the info_box
  GList *children, *iter;
  children = gtk_container_get_children(GTK_CONTAINER(info_box));
  for(iter = children; iter != NULL; iter = g_list_next(iter))
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  g_list_free(children);

  // Add the new label to the info_box
  gtk_box_pack_start(GTK_BOX(info_box), info_label, TRUE, TRUE, 0);

  // Show all widgets within the info_box
  gtk_widget_show_all(info_box);
} /* display_system_info() */
