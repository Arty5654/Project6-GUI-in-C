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
          snprintf(info_buffer + strlen(info_buffer), sizeof(info_buffer) - strlen(info_buffer), "%s\n\n", os_name);
        }
      }
      // Extract and append the OS version to the buffer
      else if (strstr(os_release_buffer, "VERSION=") != NULL) {
        char os_version[1024];
        if (sscanf(os_release_buffer, "VERSION=\"%[^\"]\"", os_version) == 1) {
          snprintf(info_buffer + strlen(info_buffer), sizeof(info_buffer) - strlen(info_buffer), "Version: %s\n\n", os_version);
        }
      }
    }
    fclose(os_release_file);
  } else {
    printf("Couldn't read /etc/os-release\n");
  }

  FILE *version_file = fopen("/proc/version", "r");

  if (version_file != NULL) {
    char version_buffer[1024];
    if (fgets(version_buffer, sizeof(version_buffer), version_file) != NULL) {
      // Find the first parenthesis and truncate the string
      char *parenthesis_pos = strchr(version_buffer, '(');
      if (parenthesis_pos != NULL) {
        *parenthesis_pos = '\0'; // Truncate the string at the parenthesis
      }

      // Append the kernel version to the buffer
      snprintf(info_buffer + strlen(info_buffer), sizeof(info_buffer) - strlen(info_buffer), "Kernel: %s", version_buffer);
    }
    fclose(version_file);
  } else {
    printf("Couldn't open /proc/version\n");
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
  FILE *cpuinfo_file = fopen("/proc/cpuinfo", "r");

  if (cpuinfo_file != NULL) {
    char cpuinfo_buffer[1024];
    while (fgets(cpuinfo_buffer, sizeof(cpuinfo_buffer), cpuinfo_file) != NULL) {
      // Check for the line containing the processor information
      if (strstr(cpuinfo_buffer, "model name") != NULL) {
        // Skip leading whitespace and the colon
        char *model_name_start = strchr(cpuinfo_buffer, ':');
        if (model_name_start != NULL) {
          model_name_start += 2; // Move past the colon and the following space

          // Append the processor information to the buffer
          snprintf(info_buffer + strlen(info_buffer), sizeof(info_buffer) - strlen(info_buffer), "Processor: %s", model_name_start);
          break; // Assuming you only want the first occurrence
        }
      }
    }
    fclose(cpuinfo_file);
  } else {
    printf("Couldn't open /proc/cpuinfo\n");
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
  GtkWidget *info_label = gtk_label_new(info_buffer);
  gtk_label_set_selectable(GTK_LABEL(info_label), TRUE);
  gtk_label_set_xalign(GTK_LABEL(info_label), 0.0);  // Align the label text to the left

  // Set the alignment of the info_label
  gtk_widget_set_halign(info_label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(info_label, GTK_ALIGN_CENTER);

  // Remove any previous widgets in the info_box
  GList *children, *iter;
  children = gtk_container_get_children(GTK_CONTAINER(info_box));
  for(iter = children; iter != NULL; iter = g_list_next(iter))
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  g_list_free(children);

  // Add the info_label to the info_box
  gtk_box_pack_start(GTK_BOX(info_box), info_label, TRUE, TRUE, 0);

  // Show all widgets within the info_box
  gtk_widget_show_all(info_box);
} /* display_system_info() */
