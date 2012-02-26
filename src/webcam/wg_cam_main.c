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
#include <wg_lsdir.h>
#include <wg_sync_linked_list.h>
#include <wg_wq.h>

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

#include <gdk-pixbuf/gdk-pixbuf.h>

#define NORM_RANGE_MIN 0
#define NORM_RANGE_MAX 255

#define TH_HIGH_DEF  240
#define TH_LOW_DEF   100


typedef struct Camera{
    GtkWidget *window;
    GdkPixbuf *pixbuf;
    GdkPixbuf *hist_pixbuf;
    GdkPixbuf *acc_pixbuf;
    GtkWidget *hist_area;
    GtkWidget *acc_area;
    GtkWidget *area;
    GtkWidget *gui_camera;
    GtkWidget *show_gray;
    GtkWidget *smooth;
    GtkWidget *threshold_low;
    GtkWidget *threshold_high;
    GtkWidget *status_bar;
    pthread_t  thread;
    Sensor    *sensor;
    gint fps;
    gint frame_count;
    Wg_video_out vid;
    wg_uint th_low;
    wg_uint th_high;
    wg_boolean dragging;
    wg_uint x1;
    wg_uint y1;
    wg_uint x2;
    wg_uint y2;
}Camera;


wg_status
create_histogram(Wg_image *img, wg_uint width, wg_uint height, Wg_image *hist);


static gboolean
on_expose_hist(GtkWidget *widget,
        cairo_t *cr,
        gpointer data)
{
    Camera *cam = NULL;

    cam = (Camera*)data;

    if (cam->hist_pixbuf != NULL){

        gdk_cairo_set_source_pixbuf(cr, cam->hist_pixbuf, 0, 0);

        cairo_paint(cr);
    }

    return TRUE;
}

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

    if (cam->acc_pixbuf != NULL){
        gui_camera_get_active_resolution(GUI_CAMERA(cam->gui_camera),
			&width, &height);

        w_width = gtk_widget_get_allocated_width(widget);
        w_height = gtk_widget_get_allocated_height(widget);

        gdk_cairo_set_source_pixbuf(cr,
                cam->acc_pixbuf, 
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

    if (cam->pixbuf != NULL){
        gui_camera_get_active_resolution(GUI_CAMERA(cam->gui_camera),
			&width, &height);

        w_width = gtk_widget_get_allocated_width(widget);
        w_height = gtk_widget_get_allocated_height(widget);

        gdk_cairo_set_source_pixbuf(cr,
                cam->pixbuf, 
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
#if 0
    Camera *cam = NULL;
    Wg_frame *frame = NULL;
    Wg_image *image_sub = NULL;
    Wg_image *tmp_img = NULL;
    Wg_image *acc_img = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint w = 0;
    wg_uint h = 0;
    wg_uint votes = 0;
    Wg_image  acc_img_gs;
    Wg_image  gs_img;
    Wg_image  rgb_img;
    Wg_image  hist_img;
    cam_status status;
    Img_draw draw;
    Wg_cam_decompressor decompressor;
    rgb24_pixel rgb_color = {0, 0, 255};

    ef_init();

    cam = (Camera*)data;

    frame = WG_CALLOC(1, sizeof (Wg_frame));

    //    cam_select_user_decompressor(camera, v4l2_fourcc('M', 'J', 'P', 'G'));

    cam_decompressor(cam->camera, &decompressor);

    gui_camera_get_active_resolution(GUI_CAMERA(cam->gui_camera),
            &width, &height);
    cam_set_resolution(cam->camera, width, height);

    cam_get_resolution(cam->camera, &width, &height);

    cam_start(cam->camera);

    gui_camera_fps_start(GUI_CAMERA(cam->gui_camera));

    video_open_output_stream("text.mpg", &cam->vid, width, height);

    for(;;){
        gdk_threads_enter();
        if ((cam->camera != NULL) && 
                (cam_read(cam->camera, frame) == CAM_SUCCESS)){
            gdk_threads_leave();

            image_sub = WG_MALLOC(sizeof (Wg_image));
            tmp_img = WG_MALLOC(sizeof (Wg_image));
            acc_img = WG_MALLOC(sizeof (Wg_image));

            status = invoke_decompressor(&decompressor, 
                    frame->start, frame->size, 
                    frame->width, frame->height, &rgb_img);
            if(CAM_SUCCESS == status){

                cam_discard_frame(cam->camera, frame);

                img_rgb_2_grayscale(&rgb_img, &gs_img);

                width = gtk_widget_get_allocated_width(cam->hist_area);
                height = gtk_widget_get_allocated_height(
                        cam->hist_area);

                create_histogram(&gs_img, width, height, &hist_img);

                img_grayscale_2_rgb(&hist_img, tmp_img);

                img_cleanup(&hist_img);

                if (gtk_toggle_button_get_active(
                            GTK_TOGGLE_BUTTON(cam->smooth))){

                    img_grayscale_normalize(&gs_img, NORM_RANGE_MAX,
                            NORM_RANGE_MIN);

                    ef_smooth(&gs_img, image_sub);

                    img_cleanup(&gs_img);

                    gs_img = *image_sub;
                }

                ef_detect_edge(&gs_img, image_sub);

                img_cleanup(&gs_img);

                gs_img = *image_sub;

                ef_hyst_thr(&gs_img,
                        (gray_pixel)gtk_range_get_value(
                            GTK_RANGE(cam->threshold_high)),
                        (gray_pixel)gtk_range_get_value(
                            GTK_RANGE(cam->threshold_low))
                        );

                ef_threshold(&gs_img, 255);

                img_grayscale_normalize(&gs_img, 255, 0);

                ef_detect_circle(&gs_img, image_sub);

                ef_acc_2_gs(image_sub, &acc_img_gs);

                img_grayscale_2_rgb(&acc_img_gs, acc_img);

                img_cleanup(&acc_img_gs);

                ef_acc_get_max(image_sub, &h, &w, &votes);
                if (votes < 100){
                    h = 0;
                    w = 0;
                }

                img_cleanup(image_sub);

                if (gtk_toggle_button_get_active(
                            GTK_TOGGLE_BUTTON(cam->show_gray))){
                    img_grayscale_2_rgb(&gs_img, image_sub);
                    img_cleanup(&rgb_img);
                    img_cleanup(&gs_img);
                }else{
                    *image_sub = rgb_img;
                    img_cleanup(&gs_img);
                }

                img_draw_get_context(image_sub->type, &draw);

                img_draw_cross(&draw, image_sub, h, w, &rgb_color);

                img_draw_cleanup_context(&draw);

                gdk_threads_enter();

                //            video_encode_frame(&cam->vid, image_sub);

                if (cam->acc_pixbuf != NULL){
                    g_object_unref(cam->acc_pixbuf);
                    cam->acc_pixbuf = NULL;
                }

                img_convert_to_pixbuf(acc_img, &cam->acc_pixbuf, NULL);

                gtk_widget_queue_draw(cam->acc_area);

                if (cam->pixbuf != NULL){
                    g_object_unref(cam->pixbuf);
                    cam->pixbuf = NULL;
                }

                img_convert_to_pixbuf(image_sub, &cam->pixbuf, NULL);

                gtk_widget_queue_draw(cam->area);

                if (cam->hist_pixbuf != NULL){
                    g_object_unref(cam->hist_pixbuf);
                    cam->hist_pixbuf = NULL;
                }

                img_convert_to_pixbuf(tmp_img, &cam->hist_pixbuf, NULL);

                gtk_widget_queue_draw(cam->hist_area);

                gui_camera_fps_update(GUI_CAMERA(cam->gui_camera), 1);

                gdk_threads_leave();
            }else{
                gdk_threads_leave();
            }
        }else{
            gdk_threads_leave();
        }
    }

    video_close_output_stream(&cam->vid);

    gui_camera_fps_stop(GUI_CAMERA(cam->gui_camera));

    //    cam_stop(cam->camera);

    //    cam_close(cam->camera);

    cam_free_frame(cam->camera, frame);

    WG_FREE(frame);
#endif
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
    GtkWidget *start_button = NULL;
    GtkWidget *resolution = NULL;
    GtkWidget *dev_path = NULL;

    cam = (Camera*)data;

    stop_capture(cam);

    start_button = gui_camera_get_start_widget(GUI_CAMERA(cam->gui_camera));
    resolution = gui_camera_get_resolution_widget(GUI_CAMERA(cam->gui_camera));
    dev_path = gui_camera_get_device_widget(GUI_CAMERA(cam->gui_camera));

    gtk_widget_set_sensitive(widget, FALSE);
    gtk_widget_set_sensitive(start_button, TRUE);
    gtk_widget_set_sensitive(resolution, TRUE);
    gtk_widget_set_sensitive(dev_path, TRUE);
    gtk_window_set_focus(GTK_WINDOW(cam->window), start_button);
}

typedef struct Update_image{
    GdkPixbuf *src_pixbuf;
    GdkPixbuf **dest_pixbuf;
    GtkWidget *area;
}Update_image;

typedef struct Update_fps{
    wg_uint frame_inc;
    Gui_camera *widget;
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

    gui_camera_fps_update(fps->widget, fps->frame_inc);

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
        break;
    case CB_EXIT:
        video_close_output_stream(&cam->vid);
        break;
    case CB_SETUP_START:
        break;
    case CB_SETUP_STOP:
        break;
    case CB_IMG_ACC:
        /* update fps */
        work.update_fps = gui_work_create(sizeof (Update_fps), update_fps_cb);

        work.update_fps->widget = GUI_CAMERA(cam->gui_camera);
        work.update_fps->frame_inc = 1;

        gui_work_add(work.update_fps);

        break;
    case CB_IMG:
        img_convert_to_pixbuf(img, &pixbuf, NULL);

        /* update frame */
        work.update_img = gui_work_create(sizeof (Update_image), 
                update_image_cb);

        work.update_img->src_pixbuf  = pixbuf;
        work.update_img->dest_pixbuf = &cam->pixbuf;
        work.update_img->area = cam->area;

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
        work.update_img->dest_pixbuf = &cam->acc_pixbuf;
        work.update_img->area = cam->acc_area;

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
    wg_uint x1;
    wg_uint y1;
    wg_uint x2;
    wg_uint y2;
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
    wg_uchar *buffer = NULL;
    wg_uint *hist_hue = NULL;
    wg_size hist_hue_size = 0;
    wg_uint i = 0;
    wg_uint max_hue = 0;
    Hsv color;

    work = (Setup_hist*)data;

    gdk_threads_enter();

    width  = gdk_pixbuf_get_width(work->cam->pixbuf);
    height = gdk_pixbuf_get_height(work->cam->pixbuf);
    buffer = gdk_pixbuf_get_pixels(work->cam->pixbuf); 

    img_rgb_from_buffer(buffer, width, height, &image);

    gdk_threads_leave();

    img_fill(abs(work->x2 - work->x1), abs(work->y2 - work->y1),
            RGB24_COMPONENT_NUM, IMG_RGB, &rect_image);

    img_get_subimage(&image, work->x1, work->y1, &rect_image);

    img_rgb_2_hsv_gtk(&rect_image, &hsv_image);

    img_hsv_hist(&hsv_image, &hist_hue, NULL, NULL,
                        &hist_hue_size, NULL, NULL);


    for (i = 0; i < hist_hue_size; ++i){
        if (max_hue < hist_hue[i]){
            color.hue = i;
            max_hue = hist_hue[i];
        }
    }

    color.hue /= WG_FLOAT(hist_hue_size);
    color.val = 0.7;
    color.sat = 0.7;
    sensor_add_color(work->cam->sensor, &color);

    WG_FREE(hist_hue);
    img_cleanup(&image);
    img_cleanup(&hsv_image);
    img_cleanup(&rect_image);
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

    work->x1 = cam->x1;
    work->y1 = cam->y1;
    work->x2 = cam->x2;
    work->y2 = cam->y2;
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
    GtkWidget *dev_path = NULL;
    GtkWidget *stop_button = NULL;
    GtkWidget *resolution = NULL;
    pthread_attr_t attr;

    cam = (Camera*)data;

    sensor = WG_MALLOC(sizeof (*sensor));
    if (NULL != sensor){
        cam->sensor = sensor;

        cam->dragging = WG_FALSE;

        g_signal_connect(cam->area, "button-press-event", 
                G_CALLBACK(pressed_mouse), cam);

        g_signal_connect(cam->area, "button-release-event", 
                G_CALLBACK(released_mouse), cam);

        dev_path = gui_camera_get_device_widget(GUI_CAMERA(cam->gui_camera));
        stop_button = gui_camera_get_stop_widget(GUI_CAMERA(cam->gui_camera));
        resolution = gui_camera_get_resolution_widget(
                GUI_CAMERA(cam->gui_camera));

        device = gtk_combo_box_text_get_active_text(
                GTK_COMBO_BOX_TEXT(dev_path));

        gui_camera_get_active_resolution(GUI_CAMERA(cam->gui_camera),
                &sensor->width, &sensor->height);
        
        strncpy(sensor->video_dev, device, VIDEO_SIZE_MAX);

        status = sensor_init(cam->sensor);
        sensor_set_threshold(cam->sensor, cam->th_high, cam->th_low);
        if (WG_SUCCESS == status){
            sensor_set_default_cb(cam->sensor, (Sensor_def_cb)default_cb, cam);

            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
            pthread_create(&cam->thread, &attr, capture, cam);
            pthread_attr_destroy(&attr);

            gtk_widget_set_sensitive(widget, FALSE);
            gtk_widget_set_sensitive(stop_button, TRUE);
            gtk_widget_set_sensitive(resolution, FALSE);
            gtk_widget_set_sensitive(dev_path, FALSE);
            gtk_window_set_focus(GTK_WINDOW(cam->window), stop_button);
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

    if (cam->pixbuf != NULL){
        gdk_pixbuf_save(cam->pixbuf, "frame.png", "png", &gerr, NULL);
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

    static void
select_resolution(GtkWidget *obj, gpointer data)
{
#if 0
    wg_uint width = 0;
    wg_uint height = 0;

    gui_camera_get_active_resolution(GTK_CAMERA(obj), &width, &height);
    WG_LOG("New resolution w:%u h:%u\n", width, height);
#endif

}

WG_PRIVATE gboolean
on_change_value_th_low(GtkRange     *range,
        GtkScrollType scroll,
        gdouble       value,
        gpointer      user_data)
{
    Camera *cam = (Camera*)user_data;

    cam->th_low = (wg_uint)value;
    if (cam->sensor != NULL){
        sensor_set_threshold(cam->sensor, cam->th_high, cam->th_low);
    }
    return FALSE;
}

WG_PRIVATE gboolean
on_change_value_th_high(GtkRange     *range,
        GtkScrollType scroll,
        gdouble       value,
        gpointer      user_data)
{
    Camera *cam = (Camera*)user_data;

    cam->th_high = (wg_uint)value;
    if (cam->sensor != NULL){
        sensor_set_threshold(cam->sensor, cam->th_high, cam->th_low);
    }
    return FALSE;
}


int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *button_start = NULL;
    GtkWidget *button_stop = NULL;
    GtkWidget *capture_button = NULL;
    GtkWidget *resolution = NULL;
    GtkWidget *hbox = NULL;
    GtkWidget *area = NULL;
    GtkWidget *color_button = NULL;
    GtkWidget *thres_low = NULL;
    GtkWidget *thres_high = NULL;
    GtkWidget *display_box = NULL;
    Camera    *camera = NULL;
    GtkWidget *gtk_cam = NULL;
    List_head  video;
    wg_uint width = 0;
    wg_uint height = 0;

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
    thres_low =  gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 255.0, 1.0);
    thres_high =  gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 255.0, 1.0);

    camera = WG_CALLOC(1, sizeof (Camera));

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    area   = gtk_drawing_area_new();
    gtk_cam = gui_camera_new();

    gui_camera_get_active_resolution(GUI_CAMERA(gtk_cam),
            &width, &height);

    camera->hist_area  = gtk_drawing_area_new();
    camera->acc_area   = gtk_drawing_area_new();
    camera->hist_pixbuf = NULL;
    camera->area = area;
    camera->gui_camera  = gtk_cam;
    camera->window = window;
    camera->fps = 0;
    camera->frame_count = 0;
    camera->threshold_low = thres_low;
    camera->threshold_high = thres_high;
    camera->status_bar = gtk_statusbar_new();

    gtk_widget_add_events(camera->area, 
            GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK ); 

    guint ctx = gtk_statusbar_get_context_id(GTK_STATUSBAR(camera->status_bar),
            "Message");

    gtk_statusbar_push(GTK_STATUSBAR(camera->status_bar), ctx, 
            "Welcome to Wall Game Plugin");

    gtk_widget_set_app_paintable(camera->hist_area, TRUE);
    gtk_widget_set_double_buffered(camera->hist_area, TRUE);

    gtk_widget_set_app_paintable(camera->acc_area, TRUE);
    gtk_widget_set_double_buffered(camera->acc_area, TRUE);

    gtk_widget_set_app_paintable(camera->area, TRUE);
    gtk_widget_set_double_buffered(camera->area, TRUE);

    button_start = gui_camera_get_start_widget(GUI_CAMERA(gtk_cam));
    button_stop = gui_camera_get_stop_widget(GUI_CAMERA(gtk_cam));
    resolution  = gui_camera_get_resolution_widget(GUI_CAMERA(gtk_cam));
    capture_button  = gui_camera_get_capture_widget(GUI_CAMERA(gtk_cam));
    color_button = gui_camera_get_color_widget(GUI_CAMERA(gtk_cam));

    camera->smooth = gui_camera_add_checkbox(GUI_CAMERA(gtk_cam), 
            "Smoothing");

    camera->show_gray = gui_camera_add_checkbox(GUI_CAMERA(gtk_cam), 
            "Show gray scale");

    gui_camera_add(GUI_CAMERA(gtk_cam), gtk_label_new("High threshold"));
    gui_camera_add(GUI_CAMERA(gtk_cam), thres_high);
    gui_camera_add(GUI_CAMERA(gtk_cam), gtk_label_new("Low threshold"));
    gui_camera_add(GUI_CAMERA(gtk_cam), thres_low);

    g_signal_connect(GTK_RANGE(thres_high), "change-value", 
            G_CALLBACK(on_change_value_th_high), camera);

    g_signal_connect(GTK_RANGE(thres_low), "change-value", 
            G_CALLBACK(on_change_value_th_low), camera);

    gtk_range_set_value(GTK_RANGE(thres_low),  TH_LOW_DEF); 
    gtk_range_set_value(GTK_RANGE(thres_high), TH_HIGH_DEF); 

    camera->th_high = TH_HIGH_DEF;
    camera->th_low  = TH_LOW_DEF;

    gtk_widget_set_sensitive(button_start, TRUE);
    gtk_widget_set_sensitive(button_stop, FALSE);

    gtk_window_set_focus(GTK_WINDOW(window), button_start);

    gtk_widget_set_size_request(area, width, height);
    gtk_widget_set_size_request(camera->hist_area, width, 75);
    gtk_widget_set_size_request(camera->acc_area, width, height);

    g_signal_connect(gtk_widget_get_toplevel(area), "draw",
            G_CALLBACK(on_expose_event), camera);
    g_signal_connect(gtk_widget_get_toplevel(camera->hist_area), "draw",
            G_CALLBACK(on_expose_hist), camera);
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

    g_signal_connect(gtk_widget_get_toplevel(resolution), "changed",
            G_CALLBACK(select_resolution), camera);

    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    display_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

    gtk_box_pack_start(GTK_BOX(display_box), camera->hist_area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(display_box), area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(display_box), camera->acc_area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(display_box), camera->status_bar, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), display_box, FALSE, TRUE, 5);

    gtk_box_pack_start(GTK_BOX(hbox), gtk_cam, FALSE, TRUE, 5);

    gtk_container_add (GTK_CONTAINER(window), hbox);

    gtk_widget_show_all(window);

    gui_work_thread_init();

    gtk_main();

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

