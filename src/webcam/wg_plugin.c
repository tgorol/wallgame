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

/** @brief FPS interwal counter     */
#define FPS_INTERVAL   0.5

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

typedef struct Update_fps{
    wg_uint frame_inc;
    Camera *camera;
}Update_fps;

typedef struct Encode_frame{
    Wg_image img;    
    Wg_video_out *out;
}Encode_frame;

/** 
* @brief List of supported resolutions
*/
WG_PRIVATE const Resolution res_info[] = {
    {"352x288", 352, 288,} ,
    {"320x240", 320, 240,} ,
    {"176x144", 176, 144,} ,
    {"160x120", 160, 120,}
};

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

WG_PRIVATE void 
xy_cb(const Sensor *sensor, Sensor_cb_type type, wg_uint x, wg_uint y, 
        void *user_data)
{
    Wg_point2d hit_point;
    Camera *cam = NULL;

    cam = (Camera*)user_data;

    wg_point2d_new(x, y, &hit_point);
    cd_add_position(&cam->cd, &hit_point);

    return;
}

WG_PRIVATE void 
hit_cb(wg_float x, wg_float y)
{
    static wg_uint count = 0;
    WG_LOG("Hit at x=%3.2f y=%3.2f %s\n", x, y, (count & 0x1) ? "--" : " ");

    ++count;

    return;
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

            sensor_set_cb(cam->sensor, CB_XY, (Sensor_def_cb)xy_cb, cam);

            sensor_add_color(cam->sensor, &cam->top);
            sensor_add_color(cam->sensor, &cam->bottom);

            cd_set_hit_callback(&cam->cd, hit_cb);

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

void
wg_plugin_start_fps(Camera *obj)
{
    g_timer_start(obj->fps_timer);
}

WG_PRIVATE void 
print_fps(Camera *obj)
{
    char text[32];

    sprintf(text, "%.1f", (float)obj->fps_val);

    gtk_label_set_text(GTK_LABEL(obj->fps_display), text);
}

/** 
* @brief Stop fps counter
* 
* @param obj wg_camera widget
*/
void
wg_plugin_stop_fps(Camera *obj)
{
    g_timer_stop(obj->fps_timer);
}

WG_PRIVATE void
update_fps_cb(void *data)
{
    Update_fps *fps = NULL;
    gulong micro = 0UL;
    double elapsed = 0.0;
    Camera *obj = NULL;

    fps = (Update_fps*)data;

    obj = fps->camera;

    obj->frame_counter += fps->frame_inc;

    if ((elapsed = g_timer_elapsed(obj->fps_timer, &micro)) >= FPS_INTERVAL){
        obj->fps_val = obj->frame_counter / elapsed;
        obj->frame_counter = 0;
        g_timer_start(obj->fps_timer);
        print_fps(obj);
    }

    return;
}

void
wg_plugin_update_fps(Camera *cam, wg_int val)
{
    Update_fps *update_fps = NULL;

    update_fps = gui_work_create(sizeof (Update_fps), update_fps_cb);

    update_fps->camera = cam;
    update_fps->frame_inc = val;

    gui_work_add(update_fps);

    return;
}


WG_PRIVATE void
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

wg_boolean
go_to_state(Camera *camera, WEBCAM_STATE new_state)
{
    switch (camera->state){
    case WEBCAM_STATE_UNINITIALIZED:
        switch (new_state){
        case WEBCAM_STATE_CALLIBRATE:
            return WG_TRUE;
        default:
            return WG_FALSE;
        }
        break;
    case WEBCAM_STATE_CALLIBRATE:
        switch (new_state){
        case WEBCAM_STATE_CALLIBRATE:
        case WEBCAM_STATE_START:
        case WEBCAM_STATE_GET_COLOR:
            return WG_TRUE;
        default:
            return WG_FALSE;
        }
        break;
    case WEBCAM_STATE_START:
        switch (new_state){
        case WEBCAM_STATE_STOP:
            return WG_TRUE;
        default:
            return WG_FALSE;
        }
        break;
    case WEBCAM_STATE_STOP:
        switch (new_state){
        case WEBCAM_STATE_START:
        case WEBCAM_STATE_CALLIBRATE:
            return WG_TRUE;
        default:
            return WG_FALSE;
        }
        break;
    case WEBCAM_STATE_GET_COLOR:
        switch (new_state){
        case WEBCAM_STATE_GET_PANE:
        case WEBCAM_STATE_CALLIBRATE:
            return WG_TRUE;
        default:
            return WG_FALSE;
        }
        break;
    case WEBCAM_STATE_GET_PANE:
        switch (new_state){
        case WEBCAM_STATE_CALLIBRATE:
            return WG_TRUE;
        default:
            return WG_FALSE;
        }
    }

    return WG_FALSE;
}

void
get_selected_resolution(GtkWidget *combo, wg_uint *width, wg_uint *height)
{
    guint index = 0;

    index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

    *width  = res_info[index].width;
    *height = res_info[index].height;

    return;
}

void
default_cb(Sensor *sensor, Sensor_cb_type type, Wg_image *img, void *user_data)
{
    Camera *cam = NULL;
    Wg_image rgb_img;
    GdkPixbuf *pixbuf = NULL;

    cam = (Camera*)user_data;
    switch (type){
    case CB_ENTER:
        video_open_output_stream("text.mpg", &cam->vid, sensor->width, 
                sensor->height);
        wg_plugin_start_fps(cam);
        break;
    case CB_EXIT:
        wg_plugin_stop_fps(cam);
        video_close_output_stream(&cam->vid);
        break;
    case CB_SETUP_START:
        break;
    case CB_SETUP_STOP:
        break;
    case CB_IMG_ACC:
        wg_plugin_update_fps(cam, 1);
        break;
    case CB_IMG:
        img_convert_to_pixbuf(img, &pixbuf, NULL);

        gui_display_set_pixbuf(&cam->left_display, 0, 0, pixbuf);

        g_object_unref(pixbuf);
        break;
    case CB_IMG_EDGE:
        /* update frame */
        img_gs_2_rgb(img, &rgb_img);
        img_convert_to_pixbuf(&rgb_img, &pixbuf, NULL);

        img_cleanup(&rgb_img);

        gui_display_set_pixbuf(&cam->right_display, 0, 0, pixbuf);

        g_object_unref(pixbuf);
        break;
    default:
        cam = NULL;
    }
    return;
}

void *
capture(void *data)
{
    Camera *cam = (Camera*)data;

    sensor_start(cam->sensor);

    pthread_exit(NULL);

    return NULL;
}

void
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

wg_status
wg_plugin_init(int argc, char *argv[], Camera *camera)
{
    GtkWidget *window = NULL;
    GtkBuilder      *builder = NULL;
    GtkWidget *widget = NULL;
    wg_uint width = 0;
    wg_uint height = 0;

    CHECK_FOR_NULL_PARAM(camera);

    memset(camera, '\0', sizeof (Camera));

    enable_threads();

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

    gtk_widget_set_size_request(widget, width, height);

    gui_display_init(widget, &camera->left_display);

    /* setup drawable right area */
    widget = GTK_WIDGET(
            gtk_builder_get_object (builder, "draw_right"));

    gtk_widget_set_size_request(widget, width, height);

    gui_display_init(widget, &camera->right_display);

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

    return WG_SUCCESS;
}

void
wg_plugin_start(Camera *camera)
{
    gtk_widget_show (camera->window);                
    gtk_main ();

    return;
}

void
wg_plugin_cleanup(Camera *camera)
{
    gui_work_thread_cleanup();

    stop_capture(camera);

    gui_display_cleanup(&camera->left_display);
    gui_display_cleanup(&camera->right_display);

    gdk_threads_leave();

    return;
}
