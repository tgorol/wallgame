#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>

#include <cairo.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <sys/select.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_linked_list.h>
#include <wg_iterator.h>
#include <wg_lsdir.h>
#include <wg_sync_linked_list.h>
#include <wg_wq.h>
#include <wg_sort.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/cam_cap.h"
#include "include/cam_frame.h"
#include "include/cam_output.h"
#include "include/cam_readwrite.h"
#include "include/cam_format_selector.h"
#include "include/vid.h"

#include "include/img.h"
#include "include/img_draw.h"

#include "include/gui_camera.h"

#include "include/sensor.h"
#include "include/gui_work.h"
#include "include/gui_prim.h"
#include "include/gui_progress_dialog.h"

#include "include/collision_detect.h"
#include "include/wg_plugin.h"
#include "include/wg_cam_callibrate.h"

#include <gdk-pixbuf/gdk-pixbuf.h>





/** @brief device path 
 *
 * @todo find a way to get it from the system
 */
#define DEV_PATH  "/dev/"

/** @brief maximum devixe path size */
#define DEVICE_PATH_MAX  64


wg_status
create_histogram(Wg_image *img, wg_uint width, wg_uint height, Wg_image *hist);

static gboolean
on_expose_acc(GtkWidget *widget,
        cairo_t *cr,
        gpointer data)
{
    Camera *cam = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    int w_width = 0;
    int w_height = 0;

    cam = (Camera*)data;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    if (cam->right_pixbuf != NULL){
        get_selected_resolution(cam->resolution_combo, &width, &height);

        w_width = gtk_widget_get_allocated_width(widget);
        w_height = gtk_widget_get_allocated_height(widget);

        gdk_cairo_set_source_pixbuf(cr,
                cam->right_pixbuf, 
                (double)((w_width - width) >> 1),
                (double)((w_height - height) >> 1));

        cairo_paint(cr);
    }

    return TRUE;
}

static gboolean
on_expose_event(GtkWidget *widget,
        cairo_t *cr,
        gpointer data)
{
    Camera *cam = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    int w_width = 0;
    int w_height = 0;

    cam = (Camera*)data;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    if (cam->left_pixbuf != NULL){
        get_selected_resolution(cam->resolution_combo, &width, &height);

        w_width = gtk_widget_get_allocated_width(widget);
        w_height = gtk_widget_get_allocated_height(widget);

        gdk_cairo_set_source_pixbuf(cr,
                cam->left_pixbuf, 
                (double)((w_width - width) >> 1),
                (double)((w_height - height) >> 1));

        cairo_paint(cr);
    }


    return TRUE;
}

void
xbuf_free(guchar *pixels, gpointer data)
{
    img_cleanup((Wg_image*)data);

    WG_FREE(data);
}


gint 
delete_event( GtkWidget *widget, GdkEvent  *event, gpointer   data )
{
    Camera *camera = NULL;

    camera = (Camera*)data;

    stop_capture(camera);

    if (NULL != camera->right_pixbuf){
        g_object_unref(camera->right_pixbuf);
        camera->right_pixbuf = NULL;
    }

    if (NULL != camera->left_pixbuf){
        g_object_unref(camera->left_pixbuf);
        camera->left_pixbuf = NULL;
    }

    return FALSE;
}

void button_clicked_stop
(GtkWidget *widget, gpointer data){
    Camera    *cam = NULL;

    cam = (Camera*)data;

    stop_capture(cam);

    gtk_widget_set_sensitive(widget, FALSE);
    gtk_widget_set_sensitive(cam->start_capturing, TRUE);
    gtk_widget_set_sensitive(cam->resolution_combo, TRUE);
    gtk_widget_set_sensitive(cam->device_combo, TRUE);
    gtk_window_set_focus(GTK_WINDOW(cam->window), cam->start_capturing);
}






void button_clicked_start
(GtkWidget *widget, gpointer data){
    Sensor *sensor = NULL;
    Camera *cam = NULL;
    const gchar *device = NULL;
    wg_status status = WG_FAILURE;
    pthread_attr_t attr;

    cam = (Camera*)data;

    sensor = WG_MALLOC(sizeof (*sensor));
    if (NULL != sensor){
        cam->sensor = sensor;

        cam->dragging = WG_FALSE;

        device = gtk_combo_box_text_get_active_text(
                GTK_COMBO_BOX_TEXT(cam->device_combo));

        get_selected_resolution(cam->resolution_combo,
                &sensor->width, &sensor->height);
        
        strncpy(sensor->video_dev, device, VIDEO_SIZE_MAX);

        status = sensor_init(cam->sensor);
        if (WG_SUCCESS != status){
            WG_FREE(cam->sensor);
            cam->sensor = NULL;
            return;
        }

        sensor_noise_reduction_set_state(cam->sensor, 
                gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(cam->noise_reduction)
                ));

        if (WG_SUCCESS == status){
            sensor_set_default_cb(cam->sensor, (Sensor_def_cb)default_cb, cam);

            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
            pthread_create(&cam->thread, &attr, capture, cam);
            pthread_attr_destroy(&attr);

            gtk_widget_set_sensitive(widget, FALSE);
            gtk_widget_set_sensitive(cam->stop_capturing, TRUE);
            gtk_widget_set_sensitive(cam->resolution_combo, FALSE);
            gtk_widget_set_sensitive(cam->device_combo, FALSE);
            gtk_window_set_focus(GTK_WINDOW(cam->window), cam->stop_capturing);
        }else{
            WG_FREE(cam->sensor);
            cam->sensor = NULL;
        }
    }

    return;
}

typedef struct Add_color{
    Hsv color;
    Camera *camera;
}Add_color;

WG_PRIVATE void
add_color(void *data)
{
    Add_color *c = (Add_color*)data;

    if (NULL != c->camera->sensor){
        sensor_add_color(c->camera->sensor, &c->color);
    }

    return;
}

WG_PRIVATE void
new_color(GtkColorSelection *colorselection, gpointer user_data) 
{
    Camera *cam = NULL;
    GdkColor rgb_color;
    Add_color *c = NULL;

    cam = (Camera*)user_data;

    gtk_color_selection_get_current_color(colorselection, &rgb_color);

    c = gui_work_create(sizeof (Add_color), add_color);

    gtk_rgb_to_hsv(
            WG_DOUBLE(rgb_color.red) / 65535.0, 
            WG_DOUBLE(rgb_color.green) / 65535.0,
            WG_DOUBLE(rgb_color.blue) / 65535.0, 
            &c->color.hue ,
            &c->color.sat ,
            &c->color.val
            );

    c->camera = cam;

    gui_work_add(c);

    return;
}

WG_PRIVATE void
button_clicked_color(GtkWidget *widget, gpointer data){
    GtkWidget *color_sel = NULL;
    GtkWidget *cs = NULL;
    Camera *cam  = NULL;

    cam  = (Camera*)data;

    color_sel = gtk_color_selection_dialog_new("Pick up a color");

    cs = gtk_color_selection_dialog_get_color_selection(
            GTK_COLOR_SELECTION_DIALOG(color_sel));

    g_signal_connect(GTK_COLOR_SELECTION(cs), 
            "color-changed", G_CALLBACK(new_color), cam);

    gtk_dialog_run(GTK_DIALOG(color_sel));
}


static void 
button_clicked_callibrate(GtkWidget *widget, gpointer data)
{
    Camera *cam             = NULL;

    cam = (Camera*)data;

    gui_callibration_screen(cam);

    return;
}

static void 
button_clicked_capture(GtkWidget *widget, gpointer data)
{
    Camera *cam = NULL;
    GError *gerr = NULL;
    GtkWidget *error_msg = NULL;

    cam = (Camera*)data;

    if (cam->left_pixbuf != NULL){
        gdk_pixbuf_save(cam->left_pixbuf, "frame.png", "png", &gerr, NULL);
    }else{
        error_msg = gtk_message_dialog_new(GTK_WINDOW(cam->window),
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                "No captured frames"
                );

        gtk_dialog_run(GTK_DIALOG(error_msg));
        gtk_widget_destroy(error_msg);
    }
}

WG_PRIVATE void
noise_reduction(GtkToggleButton *togglebutton,  gpointer user_data)
{
    Camera *cam = (Camera*)user_data;

    if (cam->sensor != NULL){
        sensor_noise_reduction_set_state(cam->sensor, 
                gtk_toggle_button_get_active(togglebutton));
    }

    return;
}

WG_PRIVATE void
enable_threads(void)
{
    if (!g_thread_supported()){
        g_thread_init(NULL);
        gdk_threads_init();
        gdk_threads_enter();
        WG_LOG("g_thread supported\n");
    }else{
        WG_LOG("g_thread not supported\n");
        exit(EXIT_FAILURE);
    }

    return;
}




WG_PRIVATE int
compare_strings(const void *s1, const void *s2)
{
    wg_dirent *d1 = NULL;
    wg_dirent *d2 = NULL;

    d1 = *(wg_dirent**)s1;
    d2 = *(wg_dirent**)s2;

    return strcmp(d1->d_name, d2->d_name);
}

WG_PRIVATE void
fill_video_combo(GtkComboBoxText *combo)
{
    List_head head;
    wg_dirent *dir_entry;
    wg_dirent **dir_entry_array;
    wg_char full_path[DEVICE_PATH_MAX];
    wg_int i = 0;
    wg_uint num_path = 0;

    list_init(&head);

    /* find all video devices in /dev directory */
    wg_lsdir(DEV_PATH, "video", &head);

    /* sort video device path names */
    num_path = list_size(&head);
    dir_entry_array = WG_ALLOCA(num_path * sizeof (wg_dirent*));
    memset(dir_entry_array, '\0', num_path * sizeof (wg_dirent*));

    list_to_array(&head, wg_dirent, list, (void**)dir_entry_array);

    qsort(dir_entry_array, num_path, sizeof (wg_dirent*), compare_strings);

    /* add found video devices to a combobox */
    for (i = 0; i < num_path; ++i){
        dir_entry = dir_entry_array[i];
        strcpy(full_path, DEV_PATH);
        strcat(full_path, dir_entry->d_name);
        gtk_combo_box_text_append_text(combo, full_path);
    }

    wg_lsdir_cleanup(&head);

    if (num_path == 0){
        gtk_combo_box_text_append_text(combo, "No device");
    }

    /* activate first element in combbox */
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);

    /* disable focus on click */
    gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(combo), FALSE); 

    return;
}


int
main(int argc, char *argv[])
{
    Camera *camera = NULL;
    GtkWidget *window = NULL;
    GtkBuilder      *builder = NULL;
    GtkWidget *widget = NULL;
    wg_uint width = 0;
    wg_uint height = 0;

    enable_threads();

    MEMLEAK_START;

    camera = WG_CALLOC(1, sizeof (Camera));

    camera->state = WEBCAM_STATE_UNINITIALIZED;

    gtk_init (&argc, &argv);

    builder = gtk_builder_new ();
    gtk_builder_add_from_file (
            builder, "layout.xml", NULL);
    window = GTK_WIDGET (gtk_builder_get_object (builder, "window"));
    gtk_builder_connect_signals (builder, NULL);

    g_signal_connect (window, "delete_event",
            G_CALLBACK(delete_event), camera);

    g_signal_connect(window, "destroy",
            G_CALLBACK(gtk_main_quit), camera);

    camera->window = window;

    /* fill supported resolutions */
    widget = GTK_WIDGET(
            gtk_builder_get_object (builder, "resolution_select"));

    fill_resolution_combo(GTK_COMBO_BOX_TEXT(widget));

    get_selected_resolution(widget, &width, &height);

    camera->resolution_combo = widget;
 
    /* fill available video devices */
    widget = GTK_WIDGET(
            gtk_builder_get_object (builder, "device_select"));

    fill_video_combo(GTK_COMBO_BOX_TEXT(widget));

    gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));

    camera->device_combo = widget;

    /* setup drawable left area */
    widget = GTK_WIDGET(
            gtk_builder_get_object (builder, "draw_left"));

    gtk_widget_set_app_paintable(widget, TRUE);
    gtk_widget_set_double_buffered(widget, TRUE);

    gtk_widget_set_size_request(widget, width, height);

    g_signal_connect(widget, "draw",
            G_CALLBACK(on_expose_event), camera);

    camera->left_area = widget;

    /* setup drawable right area */
    widget = GTK_WIDGET(
            gtk_builder_get_object (builder, "draw_right"));

    gtk_widget_set_app_paintable(widget, TRUE);
    gtk_widget_set_double_buffered(widget, TRUE);

    gtk_widget_set_size_request(widget, width, height);

    g_signal_connect(widget, "draw",
            G_CALLBACK(on_expose_acc), camera);

    camera->right_area = widget;

    /* setup noise reduction check box */
    widget = GTK_WIDGET(
            gtk_builder_get_object (builder, "noise_reduction_check"));

    g_signal_connect(GTK_TOGGLE_BUTTON(widget), "toggled",
            G_CALLBACK(noise_reduction), camera);

    camera->noise_reduction = widget;

    /* setup start_button */
    widget = GTK_WIDGET(
            gtk_builder_get_object (builder, "start_capturing"));

    gtk_widget_set_sensitive(widget, FALSE);

    g_signal_connect(widget, "clicked",
            G_CALLBACK(button_clicked_start), camera);

    gtk_window_set_focus(GTK_WINDOW(window), widget);

    camera->start_capturing = widget;

    /* setup stop button */
    widget = GTK_WIDGET(
            gtk_builder_get_object (builder, "stop_capturing"));

    gtk_widget_set_sensitive(widget, FALSE);

    g_signal_connect(widget, "clicked",
            G_CALLBACK(button_clicked_stop), camera);

    camera->stop_capturing = widget;

    /* setup callibrate button */
    widget = GTK_WIDGET(
            gtk_builder_get_object (builder, "callibrate"));

    gtk_widget_set_sensitive(widget, TRUE);

    g_signal_connect(widget, "clicked",
            G_CALLBACK(button_clicked_callibrate), camera);

    camera->callibrate = widget;

    /* setup fps couter display */
    widget = GTK_WIDGET(
            gtk_builder_get_object (builder, "fps_display"));

    camera->fps_display = widget;

    /* initialize fps counter */
    camera->fps_timer     = g_timer_new();
    camera->frame_counter = 0;
    camera->fps_val       = 0.0;

    g_object_unref (G_OBJECT (builder));

    gui_work_thread_init();

    gtk_widget_show (window);                
    gtk_main ();

    gui_work_thread_cleanup();

    stop_capture(camera);

    WG_FREE(camera);

    gdk_threads_leave();

    MEMLEAK_STOP;

    return 0;
}

wg_status
create_histogram(Wg_image *img, wg_uint width, wg_uint height, Wg_image *hist)
{
    wg_status status = CAM_FAILURE;
    wg_int i = 0;
    wg_uint max_val = 0;
    wg_uint histogram[GS_PIXEL_MAX + 1];
    wg_float my = WG_FLOAT(0.0);
    wg_float mx = WG_FLOAT(0.0);
    wg_float x = WG_FLOAT(0);
    Img_draw ctx;

    img_gs_histogram(img, histogram, ELEMNUM(histogram));

    status = img_fill(width, height, GS_COMPONENT_NUM, IMG_GS, hist);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }

    for (i = 0; i < ELEMNUM(histogram); ++i){
        max_val = WG_MAX(max_val, histogram[i]);
    }

    if (max_val == 0){
        return WG_SUCCESS;
    }

    mx = WG_FLOAT(width) / ELEMNUM(histogram);
    my = WG_FLOAT(height) / max_val;

    img_draw_get_context(hist->type, &ctx);

    for (i = 0, x = 0; i < ELEMNUM(histogram); ++i, x += 1.0){
        img_draw_line(&ctx, hist, 
                height, mx * x, height - histogram[i] * my, mx * x, 128);   
    }

    img_draw_cleanup_context(&ctx);

    return WG_SUCCESS;
}

