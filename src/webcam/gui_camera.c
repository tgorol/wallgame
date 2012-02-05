#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include "include/gui_camera.h"

#include "include/gui_resolution.h"

#define ROW_SPACING    5
#define COL_SPACING    5

#define DEFAULT_DEVICE  "/dev/video0"

#define FPS_INTERVAL   0.5


enum{
    CHANGED_SIGNAL,
    LAST_SIGNAL
};

static gint wsignals[LAST_SIGNAL] = {0};

static void
gui_camera_class_init(Gui_camera_class *wclass)
{
    wsignals[CHANGED_SIGNAL] = g_signal_new("changed",
            G_TYPE_FROM_CLASS(wclass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET(Gui_camera_class, res_selected),
            NULL,
            NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);
}

WG_INLINE guint
get_row_and_inc(Gui_camera *obj)
{
    return obj->row_count++;
}

WG_INLINE guint
get_row(Gui_camera *obj)
{
    return obj->row_count;
}

WG_INLINE void
init_row_count(Gui_camera *obj)
{
    obj->row_count = 0;
}


static void
gui_camera_init(Gui_camera *obj)
{
    GtkWidget *label = 0;

    init_row_count(obj);

    label = gtk_label_new("Device");
    gtk_grid_attach(GTK_GRID(obj),
            label,
            0, get_row(obj),
            1, 1
            );

    obj->device = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(obj->device), DEFAULT_DEVICE);
    gtk_grid_attach(GTK_GRID(obj),
            obj->device,
            1, get_row_and_inc(obj),
            1, 1
            );
    

    label = gtk_label_new("Resolution");
    gtk_grid_attach(GTK_GRID(obj),
            label,
            0, get_row(obj),
            1, 1
            );

    obj->resolution = gui_resolution_new();
    gtk_grid_attach(GTK_GRID(obj),
            obj->resolution,
            1, get_row_and_inc(obj),
            1, 1
            );

    label = gtk_label_new("fps");
    gtk_grid_attach(GTK_GRID(obj),
            label,
            0, get_row(obj),
            1, 1
            );

    obj->fps = gtk_label_new("0");
    gtk_grid_attach(GTK_GRID(obj),
            obj->fps,
            1, get_row_and_inc(obj),
            1, 1
            );


    obj->start_button = gtk_button_new_with_label("Start Capturing");
    gtk_grid_attach(GTK_GRID(obj),
            obj->start_button,
            0, get_row_and_inc(obj),
            2, 1
            );

    obj->stop_button  = gtk_button_new_with_label("Stop Capturing");
    gtk_grid_attach(GTK_GRID(obj),
            obj->stop_button,
            0, get_row_and_inc(obj),
            2, 1
            );

    obj->capture_button  = gtk_button_new_with_label("Capture Frame");
    gtk_grid_attach(GTK_GRID(obj),
            obj->capture_button,
            0, get_row_and_inc(obj),
            2, 1
            );

    obj->color_button  = gtk_button_new_with_label("Select Color");
    gtk_grid_attach(GTK_GRID(obj),
            obj->color_button,
            0, get_row_and_inc(obj),
            2, 1
            );

    gtk_grid_set_row_spacing(GTK_GRID(obj), ROW_SPACING); 
    gtk_grid_set_column_spacing(GTK_GRID(obj), COL_SPACING); 
    
    obj->fps_timer = g_timer_new();
    obj->frame_counter = 0;
    obj->fps_val = 0.0;

    return;
}

GtkWidget *
gui_camera_add_checkbox(Gui_camera *obj, gchar *text)
{
    GtkWidget *button = NULL;

    button = gtk_check_button_new_with_label(text);

    gtk_grid_attach(GTK_GRID(obj),
            button,
            0, get_row_and_inc(obj),
            2, 1
            );

    return button;
}

guint
gui_camera_get_type()
{
    static guint obj_type = 0;
    static GTypeInfo obj_info = {
        sizeof (Gui_camera_class),
        NULL,
        NULL,
        (GClassInitFunc) gui_camera_class_init,
        NULL,
        NULL,
        sizeof (Gui_camera),
        0,
        (GInstanceInitFunc) gui_camera_init
    };

    if (obj_type == 0){
        obj_type = g_type_register_static(GTK_TYPE_GRID, 
            "gui_camera", &obj_info, 0);
    }

    return obj_type;
}


GtkWidget*
gui_camera_new(void)
{
    return GTK_WIDGET(g_object_new(gui_camera_get_type(), NULL));
}

wg_status
gui_camera_get(Gui_camera *obj, wg_uint *width, wg_uint *height)
{

    return WG_SUCCESS;
}

GtkWidget *
gui_camera_get_start_widget(Gui_camera *obj)
{
    return obj->start_button;
}

GtkWidget *
gui_camera_get_capture_widget(Gui_camera *obj)
{
    return obj->capture_button;
}

GtkWidget *
gui_camera_get_stop_widget(Gui_camera *obj)
{
    return obj->stop_button;
}

GtkWidget *
gui_camera_get_color_widget(Gui_camera *obj)
{
    return obj->color_button;
}

GtkWidget *
gui_camera_get_device_widget(Gui_camera *obj)
{
    return obj->device;
}

GtkWidget *
gui_camera_get_resolution_widget(Gui_camera *obj)
{
    return obj->resolution;
}

static void 
update_fps(Gui_camera *obj)
{
    char text[32];

    sprintf(text, "%.1f", (float)obj->fps_val);

    gtk_label_set_text(GTK_LABEL(obj->fps), text);
}

void
gui_camera_fps_start(Gui_camera *obj)
{
    g_timer_start(obj->fps_timer);
}

void
gui_camera_fps_stop(Gui_camera *obj)
{
    g_timer_stop(obj->fps_timer);
}

void
gui_camera_fps_update(Gui_camera *obj, int val)
{
    gulong micro = 0UL;
    double elapsed = 0.0;

    obj->frame_counter += val;

    if ((elapsed = g_timer_elapsed(obj->fps_timer, &micro)) >= FPS_INTERVAL){
        obj->fps_val = obj->frame_counter / elapsed;
        obj->frame_counter = 0;
        g_timer_start(obj->fps_timer);
        update_fps(obj);
    }
}
