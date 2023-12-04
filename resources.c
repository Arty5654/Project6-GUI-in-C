/*
 * resources.c
 * Author: Yash Mehta
 * CS 252 - Task Manager Project
 */

#include <gtk/gtk.h>
#include <cairo.h>
#include <stdio.h>

#define CPU_HISTORY 60
#define MEMORY_HISTORY 60

// Structs to hold memory usage and cpu usage data
typedef struct _MemoryUsage {
    float mem_usage[MEMORY_HISTORY];
    float swap_usage[MEMORY_HISTORY];
    int last;
} MemoryUsage;

typedef struct _CpuUsage {
    float usage[CPU_HISTORY];
    int last;
} CpuUsage;

// global variables
static CpuUsage cpu;
static MemoryUsage mem;
static GtkWidget *g_drawing_area = NULL;
static GtkWidget *g_mem_drawing_area = NULL;

// Forward declaration
static void draw_cpu_graph(GtkWidget *widget, cairo_t *cr);
static void draw_memory_graph(GtkWidget *widget, cairo_t *cr);
static void read_memory_usage();

// Function to read CPU usage from /proc/stat
static float read_cpu_usage() {
    static float last_non_zero_usage = -1.0f;
    FILE *fp;
    char buf[128];
    unsigned long long int user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
    unsigned long long int all_time, idle_all_time, total_diff, idle_diff;
    float usage;

    fp = fopen("/proc/stat", "r");
    if (!fp) {
        perror("Error opening /proc/stat");
        return -1.0f;
    }

    fgets(buf, sizeof(buf), fp);
    sscanf(buf, "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice);

    fclose(fp);

    all_time = user + nice + system + idle + iowait + irq + softirq + steal;
    idle_all_time = idle + iowait;

    static unsigned long long int prev_all_time = 0, prev_idle_all_time = 0;

    if (prev_all_time == 0 || prev_idle_all_time == 0) {
        prev_all_time = all_time;
        prev_idle_all_time = idle_all_time;
        return 0.0f;
    }

    total_diff = all_time - prev_all_time;
    idle_diff = idle_all_time - prev_idle_all_time;

    if (total_diff == 0 || idle_diff > total_diff) {
        return last_non_zero_usage > 0 ? last_non_zero_usage : 0.0f;
    }

    usage = (float)(total_diff - idle_diff) / total_diff;

    // Check if usage is zero and if we have a previous non-zero value to use
    if (usage == 0 && last_non_zero_usage > 0) {
        usage = last_non_zero_usage;
    } else if (usage > 0) {
        last_non_zero_usage = usage;
    }

    prev_all_time = all_time;
    prev_idle_all_time = idle_all_time;

    return usage * 100.0f; // Convert to percentage
}

static gboolean update_resource_usage(gpointer user_data) {
    // Update CPU
    cpu.last = (cpu.last + 1) % CPU_HISTORY;
    cpu.usage[cpu.last] = read_cpu_usage();

    // Update Memory and Swap
    mem.last = (mem.last + 1) % MEMORY_HISTORY;
    read_memory_usage();

    // Queue redraw for both CPU and Memory graphs
    if (g_drawing_area != NULL)
        gtk_widget_queue_draw(g_drawing_area);
    if (g_mem_drawing_area != NULL)
        gtk_widget_queue_draw(g_mem_drawing_area);

    return TRUE;
}

// Function to draw the CPU graph with axes and title
static void draw_cpu_graph(GtkWidget *widget, cairo_t *cr) {
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    int width = allocation.width;
    int height = allocation.height;

    // Calculate the graph's margin
    const int margin = 30;  

    // Clear background
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // Draw the axes
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1);

    // Y-axis
    cairo_move_to(cr, margin, margin);
    cairo_line_to(cr, margin, height - margin);
    cairo_stroke(cr);

    // X-axis
    cairo_move_to(cr, margin, height - margin);
    cairo_line_to(cr, width - margin, height - margin);
    cairo_stroke(cr);

    // Set dashed line style for the horizontal lines
    const double dashed1[] = {4.0};
    int len1  = sizeof(dashed1) / sizeof(dashed1[0]);
    cairo_set_dash(cr, dashed1, len1, 0);

    // Draw horizontal lines at 25%, 50%, and 75%
    cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 0.5); // Light grey, semi-transparent
    cairo_set_line_width(cr, 1);
    for (int i = 1; i < 5; i++) {
        double y = margin + (1.0 - i * 0.25) * (height - 2 * margin);
        cairo_move_to(cr, margin, y);
        cairo_line_to(cr, width - margin, y);
        cairo_stroke(cr);
    }

    // Reset the dashed line to default for the main graph line
    cairo_set_dash(cr, NULL, 0, 0);

    cairo_set_source_rgb(cr, 0, 0, 0); // Black color for text
    cairo_set_font_size(cr, 10);
    const char *percentage_labels[] = {"100%", "75%", "50%", "25%"};
    for (int i = 0; i < 4; i++) {
        double y = margin + i * 0.25 * (height - 2 * margin);
        cairo_move_to(cr, width - margin + 5, y);
        cairo_show_text(cr, percentage_labels[i]);
    }

    // Draw the CPU usage graph
    // blue line for CPU
    cairo_set_source_rgb(cr, 0.3, 0.6, 0.9);
    cairo_set_line_width(cr, 2);

    // Draw the CPU usage graph
    for (int i = 0; i < CPU_HISTORY; i++) {
        int x = margin + i * (width - 2 * margin) / CPU_HISTORY;
        int y = margin + (1.0f - cpu.usage[(cpu.last + i) % CPU_HISTORY] / 100.0f) * (height - 2 * margin);
        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    cairo_stroke(cr);

    const char *title = "Aggregate CPU History";
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14);
    cairo_move_to(cr, width / 2 - margin, margin / 2);
    cairo_show_text(cr, title);

    cairo_set_font_size(cr, 10);
    cairo_move_to(cr, 5, margin / 2);
    cairo_show_text(cr, "100%");
    cairo_move_to(cr, 5, height - margin / 2);
    cairo_show_text(cr, "0%");

    cairo_move_to(cr, margin, height - 10);
    cairo_show_text(cr, "-60s");
    cairo_move_to(cr, width - margin - 30, height - 10);
    cairo_show_text(cr, "Now");
}

static void draw_memory_graph(GtkWidget *widget, cairo_t *cr) {
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    int width = allocation.width;
    int height = allocation.height;

    // Calculate the graph's margin
    const int margin = 30;  

    // Clear background
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // Draw the axes
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1);

    // Y-axis
    cairo_move_to(cr, margin, margin);
    cairo_line_to(cr, margin, height - margin);
    cairo_stroke(cr);

    // X-axis
    cairo_move_to(cr, margin, height - margin);
    cairo_line_to(cr, width - margin, height - margin);
    cairo_stroke(cr);

    // Set dashed line style for the horizontal lines
    const double dashed1[] = {4.0};
    int len1  = sizeof(dashed1) / sizeof(dashed1[0]);
    cairo_set_dash(cr, dashed1, len1, 0);

    cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 0.5);
    cairo_set_line_width(cr, 1);
    for (int i = 1; i < 5; i++) {
        double y = margin + (1.0 - i * 0.25) * (height - 2 * margin);
        cairo_move_to(cr, margin, y);
        cairo_line_to(cr, width - margin, y);
        cairo_stroke(cr);
    }

    // Reset the dashed line to default for the main graph line
    cairo_set_dash(cr, NULL, 0, 0);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_font_size(cr, 10);
    const char *percentage_labels[] = {"100%", "75%", "50%", "25%"};
    for (int i = 0; i < 4; i++) {
        double y = margin + i * 0.25 * (height - 2 * margin);
        cairo_move_to(cr, width - margin + 5, y);
        cairo_show_text(cr, percentage_labels[i]);
    }

    // Draw memory usage graph, GREEN
    cairo_set_source_rgb(cr, 0.2, 0.8, 0.2);
    cairo_set_line_width(cr, 2);

    for (int i = 0; i < MEMORY_HISTORY; i++) {
        int x = margin + i * (width - 2 * margin) / MEMORY_HISTORY;
        int y = margin + (1.0f - mem.mem_usage[(mem.last + i) % MEMORY_HISTORY] / 100.0f) * (height - 2 * margin);
        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    cairo_stroke(cr);

    // Draw swap usage graph, RED
    cairo_set_source_rgb(cr, 0.8, 0.2, 0.2);
    cairo_set_line_width(cr, 2);

    for (int i = 0; i < MEMORY_HISTORY; i++) {
        int x = margin + i * (width - 2 * margin) / MEMORY_HISTORY;
        int y = margin + (1.0f - mem.swap_usage[(mem.last + i) % MEMORY_HISTORY] / 100.0f) * (height - 2 * margin);
        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    cairo_stroke(cr);

    // Draw the title
    const char *title = "Memory & Swap History";
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14);
    cairo_move_to(cr, width / 2 - margin, margin / 2);
    cairo_show_text(cr, title);

    cairo_set_font_size(cr, 10);
    cairo_move_to(cr, 5, margin / 2);
    cairo_show_text(cr, "100%");
    cairo_move_to(cr, 5, height - margin / 2);
    cairo_show_text(cr, "0%");

    cairo_move_to(cr, margin, height - 10);
    cairo_show_text(cr, "-60s");
    cairo_move_to(cr, width - margin - 30, height - 10);
    cairo_show_text(cr, "Now");
}

static void read_memory_usage() {
    FILE *fp;
    char buf[256];
    unsigned long memTotal, memFree, swapTotal, swapFree;

    fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("Error opening /proc/meminfo");
        return;
    }

    while (fgets(buf, sizeof(buf), fp)) {
        sscanf(buf, "MemTotal: %lu kB", &memTotal);
        sscanf(buf, "MemFree: %lu kB", &memFree);
        sscanf(buf, "SwapTotal: %lu kB", &swapTotal);
        sscanf(buf, "SwapFree: %lu kB", &swapFree);
    }

    fclose(fp);

    // Calculate usage as a percentage
    mem.mem_usage[mem.last] = 100.0f * (1.0f - ((float)memFree / memTotal));
    mem.swap_usage[mem.last] = 100.0f * (1.0f - ((float)swapFree / swapTotal));
}

// Function to be called when the "Resources" tab is selected
void display_resource_usage(GtkWidget *box) {
    memset(&cpu, 0, sizeof(cpu));
    memset(&mem, 0, sizeof(mem));

    // Create a drawing area for the CPU graph
    g_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(g_drawing_area, 200, 100);
    g_signal_connect(G_OBJECT(g_drawing_area), "draw", G_CALLBACK(draw_cpu_graph), NULL);
    gtk_box_pack_start(GTK_BOX(box), g_drawing_area, TRUE, TRUE, 0);

    // Create a drawing area for the Memory and Swap graph
    g_mem_drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(g_mem_drawing_area, 200, 100);
    g_signal_connect(G_OBJECT(g_mem_drawing_area), "draw", G_CALLBACK(draw_memory_graph), NULL);
    gtk_box_pack_start(GTK_BOX(box), g_mem_drawing_area, TRUE, TRUE, 0);

    g_timeout_add_seconds(1, (GSourceFunc)update_resource_usage, g_drawing_area);
    g_timeout_add_seconds(1, (GSourceFunc)update_resource_usage, g_mem_drawing_area);

    gtk_widget_show_all(box);
}

