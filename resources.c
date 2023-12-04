// resources.c
#include <gtk/gtk.h>
#include <cairo.h>
#include <stdio.h>

#define CPU_HISTORY 60 // Number of points in the graph

typedef struct _CpuUsage {
    float usage[CPU_HISTORY]; // Array to store usage history
    int last; // Index of the last recorded usage
} CpuUsage;

static CpuUsage cpu;

// Forward declaration
static void draw_cpu_graph(GtkWidget *widget, cairo_t *cr);

// Function to read CPU usage from /proc/stat
static float read_cpu_usage() {
    FILE *fp;
    char buf[128];
    unsigned long long int user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
    unsigned long long int all_time, idle_all_time;
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

    if (prev_all_time == 0) { // First call, no previous data
        prev_all_time = all_time;
        prev_idle_all_time = idle_all_time;
        return 0.0f;
    }

    // Calculate the difference in total and idle time from last check
    usage = (float)(all_time - prev_all_time - (idle_all_time - prev_idle_all_time)) / (all_time - prev_all_time);

    // Save current times for next calculation
    prev_all_time = all_time;
    prev_idle_all_time = idle_all_time;

    return usage * 100.0f; // Convert to percentage
}

// Function to update CPU usage and redraw graph
static gboolean update_cpu_usage(GtkWidget *widget) {
    cpu.last = (cpu.last + 1) % CPU_HISTORY;
    cpu.usage[cpu.last] = read_cpu_usage();
    gtk_widget_queue_draw(widget); // Queue a redraw of the widget
    return TRUE; // Continue timeout
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
    // Set color for CPU usage line
    cairo_set_source_rgb(cr, 0.3, 0.6, 0.9); // This sets a blue color
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

    // Draw the title
    const char *title = "Aggregate CPU History";
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14);
    cairo_move_to(cr, width / 2 - margin, margin / 2);
    cairo_show_text(cr, title);

    // Draw Y-axis labels
    cairo_set_font_size(cr, 10);
    cairo_move_to(cr, 5, margin / 2);
    cairo_show_text(cr, "100%");
    cairo_move_to(cr, 5, height - margin / 2);
    cairo_show_text(cr, "0%");

    // Draw X-axis labels (assuming each point represents 1 second)
    cairo_move_to(cr, margin, height - 10);
    cairo_show_text(cr, "-60s");
    cairo_move_to(cr, width - margin - 30, height - 10);
    cairo_show_text(cr, "Now");
}

// Function to be called when the "Resources" tab is selected
void display_resource_usage(GtkWidget *box) {
    memset(&cpu, 0, sizeof(cpu));

    // Create a drawing area for the CPU graph
    GtkWidget *drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, 200, 100); // Set the size you want
    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(draw_cpu_graph), NULL);
    gtk_box_pack_start(GTK_BOX(box), drawing_area, TRUE, TRUE, 0);

    // Start a timer to update the CPU usage every second
    g_timeout_add_seconds(1, (GSourceFunc)update_cpu_usage, drawing_area);

    gtk_widget_show_all(box);
}
