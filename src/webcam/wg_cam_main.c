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

#include <gdk-pixbuf/gdk-pixbuf.h>

/** @brief FPS interwal counter     */
#define FPS_INTERVAL   0.5

typedef struct Camera{
    /* gui variables */
    GtkWidget *menubar;
    GtkWidget *window;
    GtkWidget *resolution_combo;
    GtkWidget *device_combo;
    GtkWidget *start_capturing;
    GtkWidget *stop_capturing;
    GtkWidget *left_area;
    GtkWidget *right_area;
    GtkWidget *noise_reduction;
    GtkWidget *fps_display;

    /* Sensor variables */
    Sensor    *sensor;
    
    /* fps counter variables */
    GTimer *fps_timer;             /*!< timer used by fps counter */
    gint    frame_counter;         /*!< number of counted frames  */
    gfloat  fps_val;               /*!< fps value                 */

    GdkPixbuf *right_pixbuf;
    GdkPixbuf *left_pixbuf;
    GtkWidget *status_bar;

    pthread_t  thread;
    Wg_video_out vid;
    wg_boolean dragging;
    wg_uint x1;
    wg_uint y1;
    wg_uint x2;
    wg_uint y2;
}Camera;


wg_status
create_histogram(Wg_image *img, wg_uint width, wg_uint height, Wg_image *hist);

WG_PRIVATE void
get_selected_resolution(GtkWidget *combo, wg_uint *width, wg_uint *height);

WG_PRIVATE void
stop_fps(Camera *obj);

WG_STATIC void
start_fps(Camera *obj);

WG_PRIVATE void
update_fps(Camera *obj, int val);

WG_PRIVATE void 
print_fps(Camera *obj);

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

void *
capture(void *data)
{
    Camera *cam = (Camera*)data;

    sensor_start(cam->sensor);

    pthread_exit(NULL);
}


void
stop_capture(Camera *cam)
{
    if (cam->sensor != NULL){
        sensor_stop(cam->sensor);

        pthread_cancel(cam->thread);

        pthread_join(cam->thread, NULL);

        sensor_cleanup(cam->sensor);

        WG_FREE(cam->sensor);
        cam->sensor = NULL;
    }

    return;
}

gint 
delete_event( GtkWidget *widget, GdkEvent  *event, gpointer   data )
{
    Camera *cam = NULL;

    cam = (Camera*)data;

    stop_capture(cam);

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

typedef struct Update_image{
    GdkPixbuf *src_pixbuf;
    GdkPixbuf **dest_pixbuf;
    GtkWidget *area;
}Update_image;

typedef struct Update_fps{
    wg_uint frame_inc;
    Camera *camera;
}Update_fps;

typedef struct Encode_frame{
    Wg_image img;    
    Wg_video_out *out;
}Encode_frame;

WG_PRIVATE void
update_fps_cb(void *data)
{
    Update_fps *fps = NULL;

    fps = (Update_fps*)data;

    update_fps(fps->camera, fps->frame_inc);

    return;
}

WG_PRIVATE void
update_image_cb(void *data)
{
    Update_image *img = NULL;

    img = (Update_image*)data;

    gdk_threads_enter();

    if (*img->dest_pixbuf != NULL){
        g_object_unref(*img->dest_pixbuf);
        *img->dest_pixbuf = NULL;
    }

    *img->dest_pixbuf = img->src_pixbuf;

    gtk_widget_queue_draw(img->area);

    gdk_threads_leave();

    return;
}

void
encode_frame(void *data)
{
    Encode_frame *ef = (Encode_frame*)data;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec   = 1000 * 40;
    select(0, NULL, NULL, NULL, &tv);

//    video_encode_frame(ef->out, &ef->img);

    img_cleanup(&ef->img);

    return;
}

WG_PRIVATE void
default_cb(Sensor *sensor, Sensor_cb_type type, Wg_image *img, void *user_data)
{
    Camera *cam = NULL;
    Wg_image rgb_img;
    union {
        Update_image *update_img;
        Update_fps   *update_fps;
        Encode_frame   *encode_frame;
    }work;
    GdkPixbuf *pixbuf = NULL;

    cam = (Camera*)user_data;
    switch (type){
    case CB_ENTER:
        video_open_output_stream("text.mpg", &cam->vid, sensor->width, 
                sensor->height);
        start_fps(cam);
        break;
    case CB_EXIT:
        stop_fps(cam);
        video_close_output_stream(&cam->vid);
        break;
    case CB_SETUP_START:
        break;
    case CB_SETUP_STOP:
        break;
    case CB_IMG_ACC:
        /* update fps */
        work.update_fps = gui_work_create(sizeof (Update_fps), update_fps_cb);

        work.update_fps->camera = cam;
        work.update_fps->frame_inc = 1;

        gui_work_add(work.update_fps);
        break;
    case CB_IMG:
        img_convert_to_pixbuf(img, &pixbuf, NULL);

        /* update frame */
        work.update_img = gui_work_create(sizeof (Update_image), 
                update_image_cb);

        work.update_img->src_pixbuf  = pixbuf;
        work.update_img->dest_pixbuf = &cam->left_pixbuf;
        work.update_img->area = cam->left_area;

        gui_work_add(work.update_img);

#if 0
        work.encode_frame = gui_work_create(sizeof (Encode_frame), 
                encode_frame);

        img_copy(img, &work.encode_frame->img);

        work.encode_frame->out = &cam->vid;

        gui_work_add(work.encode_frame);
#endif

        break;
    case CB_IMG_EDGE:
        /* update frame */
        img_gs_2_rgb(img, &rgb_img);
        img_convert_to_pixbuf(&rgb_img, &pixbuf, NULL);

        img_cleanup(&rgb_img);

        work.update_img = gui_work_create(sizeof (Update_image), 
                update_image_cb);

        work.update_img->src_pixbuf  = pixbuf;
        work.update_img->dest_pixbuf = &cam->right_pixbuf;
        work.update_img->area = cam->right_area;

        gui_work_add(work.update_img);
        break;
    default:
        cam = NULL;
    }
    return;
}

WG_PRIVATE gboolean
pressed_mouse(GtkWidget *widget, GdkEvent  *event, gpointer user_data) 
{
    GdkEventButton *event_button = NULL;
    Camera *cam = NULL;

    cam = (Camera*)user_data;
    event_button = (GdkEventButton*)event;
    cam->dragging = WG_TRUE;
    cam->x1 = event_button->x;
    cam->y1 = event_button->y;

    return FALSE;
}

typedef struct Setup_hist{
    Wg_rect rect;
    Camera *cam;
}Setup_hist;

WG_PRIVATE void
setup_hist(void *data)
{
    Setup_hist *work = NULL;
    Wg_image image;
    Wg_image rect_image;
    Wg_image hsv_image;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint t_width = 0;
    wg_uint t_height = 0;
    wg_uchar *buffer = NULL;
    wg_uint i = 0;
    Hsv *color = NULL;
    wg_uint size = 0;
    wg_uint num = 0;

    work = (Setup_hist*)data;

    gdk_threads_enter();

    width  = gdk_pixbuf_get_width(work->cam->left_pixbuf);
    height = gdk_pixbuf_get_height(work->cam->left_pixbuf);
    buffer = gdk_pixbuf_get_pixels(work->cam->left_pixbuf); 

    img_rgb_from_buffer(buffer, width, height, &image);

    gdk_threads_leave();

    t_width = gtk_widget_get_allocated_width(work->cam->left_area);
    t_height = gtk_widget_get_allocated_height(work->cam->left_area);

    img_fill(work->rect.width, work->rect.height,
            RGB24_COMPONENT_NUM, IMG_RGB, &rect_image);

    wg_rect_move(&work->rect, 
            -((t_width - width) >> 1), -((t_height - height) >> 1));

    img_get_subimage(&image, work->rect.x, work->rect.y, &rect_image);

    img_rgb_2_hsv_gtk(&rect_image, &hsv_image);

    img_get_data(&hsv_image, (wg_uchar**)&color, &num, &size);

    for (i = 0; i < num; ++i, ++color){
        sensor_add_color(work->cam->sensor, color);
    }

    img_cleanup(&image);
    img_cleanup(&hsv_image);
    img_cleanup(&rect_image);

    return;
}

WG_PRIVATE gboolean
released_mouse(GtkWidget *widget, GdkEvent  *event, gpointer user_data) 
{
    GdkEventButton *event_button = NULL;
    Camera *cam = NULL;
    Setup_hist *work = NULL;

    cam = (Camera*)user_data;
    event_button = (GdkEventButton*)event;
    cam->x2 = event_button->x;
    cam->y2 = event_button->y;
    cam->dragging = WG_FALSE;

    work = gui_work_create(sizeof (Setup_hist), setup_hist);

    wg_rect_new_from_points(cam->x1, cam->y1, cam->x2, cam->y2, &work->rect);
    work->cam = cam;

    gui_work_add(work);

    return FALSE;
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

        g_signal_connect(cam->left_area, "button-press-event", 
                G_CALLBACK(pressed_mouse), cam);

        g_signal_connect(cam->left_area, "button-release-event", 
                G_CALLBACK(released_mouse), cam);

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

static void button_clicked_capture
(GtkWidget *widget, gpointer data){
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

/** @brief Default resolution */
#define RESOLUTION_DEFAULT_INDEX    0

/** @brief device path 
 *
 * @todo find a way to get it from the system
 */
#define DEV_PATH  "/dev/"

/** @brief maximum devixe path size */
#define DEVICE_PATH_MAX  64

typedef struct Resolution{
    wg_char text[16];        /*!< text of the resolution */
    wg_uint  width;          /*!< width in pixels        */
    wg_uint  height;         /*!< height in pixels       */
}Resolution;

/** 
* @brief List of supported resolutions
*/
static const Resolution res_info[] = {
    {"352x288", 352, 288,} ,
    {"320x240", 320, 240,} ,
    {"176x144", 176, 144,} ,
    {"160x120", 160, 120,}
};


WG_PRIVATE void
fill_resolution_combo(GtkComboBoxText *combo)
{
    wg_int i = 0;

    for (i = 0; i < ELEMNUM(res_info); ++i){
        gtk_combo_box_text_append_text(combo, res_info[i].text);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), RESOLUTION_DEFAULT_INDEX);

    gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(combo), FALSE); 
    
    return;
}

WG_PRIVATE void
get_selected_resolution(GtkWidget *combo, wg_uint *width, wg_uint *height)
{
    guint index = 0;

    index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

    *width  = res_info[index].width;
    *height = res_info[index].height;

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

WG_STATIC void
start_fps(Camera *obj)
{
    g_timer_start(obj->fps_timer);
}

/** 
* @brief Update fps counter
* 
* @param obj wg_camera widget
* @param val number of frames to update
*/
WG_PRIVATE void
update_fps(Camera *obj, int val)
{
    gulong micro = 0UL;
    double elapsed = 0.0;

    obj->frame_counter += val;

    if ((elapsed = g_timer_elapsed(obj->fps_timer, &micro)) >= FPS_INTERVAL){
        obj->fps_val = obj->frame_counter / elapsed;
        obj->frame_counter = 0;
        g_timer_start(obj->fps_timer);
        print_fps(obj);
    }
}

/** 
* @brief Stop fps counter
* 
* @param obj wg_camera widget
*/
WG_PRIVATE void
stop_fps(Camera *obj)
{
    g_timer_stop(obj->fps_timer);
}

WG_PRIVATE void 
print_fps(Camera *obj)
{
    char text[32];

    sprintf(text, "%.1f", (float)obj->fps_val);

    gtk_label_set_text(GTK_LABEL(obj->fps_display), text);
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

    gtk_widget_set_sensitive(widget, TRUE);

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

#if 0
    MEMLEAK_START;

    list_init(&video);

    wg_lsdir("/dev/", "video", &video);

    wg_lsdir_cleanup(&video);

    if (!g_thread_supported()){
        g_thread_init(NULL);
        gdk_threads_init();
        gdk_threads_enter();
        WG_LOG("g_thread supported\n");
    }else{
        WG_LOG("g_thread not supported\n");
        return EXIT_FAILURE;
    }

    gtk_init(&argc, &argv);

    camera = WG_CALLOC(1, sizeof (Camera));

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    area   = gtk_drawing_area_new();
    gtk_cam = gui_camera_new();

    gui_camera_get_active_resolution(GUI_CAMERA(gtk_cam),
            &width, &height);

    camera->acc_area   = gtk_drawing_area_new();
    camera->hist_pixbuf = NULL;
    camera->area = area;
    camera->gui_camera  = gtk_cam;
    camera->window = window;
    camera->fps = 0;
    camera->frame_count = 0;
    camera->status_bar = gtk_statusbar_new();
    camera->menubar = gtk_menu_bar_new();

    gtk_menu_shell_append(GTK_MENU_SHELL(camera->menubar),
            gtk_menu_item_new_with_label("File"));

    camera->noise_reduction = 
        gui_camera_add_checkbox(GUI_CAMERA(gtk_cam), "Noise Reduction");

    g_signal_connect(GTK_TOGGLE_BUTTON(camera->noise_reduction), "toggled",
            G_CALLBACK(noise_reduction), camera);

    gtk_widget_add_events(camera->area, 
            GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK ); 

    guint ctx = gtk_statusbar_get_context_id(GTK_STATUSBAR(camera->status_bar),
            "Message");

    gtk_statusbar_push(GTK_STATUSBAR(camera->status_bar), ctx, 
            "Welcome to Wall Game Plugin");

    gtk_widget_set_app_paintable(camera->acc_area, TRUE);
    gtk_widget_set_double_buffered(camera->acc_area, TRUE);

    gtk_widget_set_app_paintable(camera->area, TRUE);
    gtk_widget_set_double_buffered(camera->area, TRUE);

    button_start = gui_camera_get_start_widget(GUI_CAMERA(gtk_cam));
    button_stop = gui_camera_get_stop_widget(GUI_CAMERA(gtk_cam));
    capture_button  = gui_camera_get_capture_widget(GUI_CAMERA(gtk_cam));
    color_button = gui_camera_get_color_widget(GUI_CAMERA(gtk_cam));

    gtk_widget_set_sensitive(button_start, TRUE);
    gtk_widget_set_sensitive(button_stop, FALSE);

    gtk_window_set_focus(GTK_WINDOW(window), button_start);

    gtk_widget_set_size_request(area, width, height);
    gtk_widget_set_size_request(camera->acc_area, width, height);

    g_signal_connect(gtk_widget_get_toplevel(area), "draw",
            G_CALLBACK(on_expose_event), camera);
    g_signal_connect(gtk_widget_get_toplevel(camera->acc_area), "draw",
            G_CALLBACK(on_expose_acc), camera);

    g_signal_connect(gtk_widget_get_toplevel(window), "destroy",
            G_CALLBACK(gtk_main_quit), camera);

    g_signal_connect(button_start, "clicked",
            G_CALLBACK(button_clicked_start), camera);
    g_signal_connect(button_stop, "clicked",
            G_CALLBACK(button_clicked_stop), camera);
    g_signal_connect(capture_button, "clicked",
            G_CALLBACK(button_clicked_capture), camera);
    g_signal_connect(color_button, "clicked",
            G_CALLBACK(button_clicked_color), camera);

    g_signal_connect (gtk_widget_get_toplevel (window), "delete_event",
            G_CALLBACK(delete_event), camera);

    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    display_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    top_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    gtk_box_pack_start(GTK_BOX(display_box), area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(display_box), camera->acc_area, TRUE, TRUE, 0);

    //    gtk_box_pack_start(GTK_BOX(display_box), camera->status_bar, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), display_box, FALSE, FALSE, 2);

    gtk_box_pack_start(GTK_BOX(hbox), gtk_cam, TRUE, TRUE, 2);

    gtk_box_pack_start(GTK_BOX(top_box), camera->menubar, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(top_box), hbox, FALSE, FALSE, 2);

    gtk_container_add (GTK_CONTAINER(window), top_box);

    gtk_widget_show_all(window);

    gui_work_thread_init();

    gtk_main();

    stop_capture(camera);

    gui_work_thread_cleanup();

    if (NULL != camera->pixbuf){
        g_object_unref(camera->pixbuf);
    }

    if (NULL != camera->pixbuf){
        g_object_unref(camera->acc_pixbuf);
    }

    if (NULL != camera->pixbuf){
        g_object_unref(camera->hist_pixbuf);
    }

    if (NULL != camera->sensor){
        sensor_cleanup(camera->sensor);
        WG_FREE(camera->sensor);
    }

    WG_FREE(camera);

    gdk_threads_leave();

    MEMLEAK_STOP;

    return 0;
#endif
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

