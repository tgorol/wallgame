#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>
#include <wg_linked_list.h>
#include <wg_sync_linked_list.h>
#include <wg_wq.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/img.h"
#include "include/sensor.h"
#include "include/gui_prim.h"
#include "include/gui_progress_dialog.h"
#include "include/vid.h"
#include "include/gui_work.h"

#include "include/collision_detect.h"
#include "include/wg_plugin.h"

#include "include/wg_cam_callibrate.h"

#define SCREEN_CORNER_NUM   4

typedef struct Callibration_data{
    Camera *camera;
    wg_boolean is_camera_initialized;
    wg_uint corner_count;
    Wg_point2d corners[SCREEN_CORNER_NUM];
}Callibration_data;

typedef struct Setup_hist{
    Wg_rect rect;
    Camera *cam;
}Setup_hist;

WG_PRIVATE wg_boolean
callibration_start(Gui_progress_action action, void *user_data);

WG_PRIVATE wg_boolean
callibration_screen(Gui_progress_action action, void *user_data);

WG_PRIVATE wg_boolean
callibration_color(Gui_progress_action action, void *user_data);

WG_PRIVATE wg_boolean
callibration_finish(Gui_progress_action action, void *user_data);


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

WG_PRIVATE gboolean
screen_corner_release_mouse(GtkWidget *widget, GdkEvent  *event,
        gpointer user_data)
{
    Callibration_data *data = NULL;
    GdkEventButton *event_button = NULL;

    data = (Callibration_data*)user_data;
    event_button = (GdkEventButton*)event;

    data->corner_count %= SCREEN_CORNER_NUM;

    wg_point2d_new(event_button->x, event_button->y, 
            &data->corners[data->corner_count]);

    WG_LOG("Click at x=%3d y=%3d\n", 
            (int)event_button->x, (int)event_button->y); 

    ++data->corner_count;

    return WG_FALSE;
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
callibration_default_cb(Sensor *sensor, Sensor_cb_type type, Wg_image *img, 
        void *user_data)
{
    Camera *cam = NULL;
    Wg_image rgb_img;
    union {
        Update_image *update_img;
    }work;
    GdkPixbuf *pixbuf = NULL;

    cam = (Camera*)user_data;
    switch (type){
    case CB_ENTER:
        wg_plugin_start_fps(cam);
        break;
    case CB_EXIT:
        wg_plugin_stop_fps(cam);
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

        /* update frame */
        work.update_img = gui_work_create(sizeof (Update_image), 
                update_image_cb);

        work.update_img->src_pixbuf  = pixbuf;
        work.update_img->dest_pixbuf = &cam->left_pixbuf;
        work.update_img->area = cam->left_area;

        gui_work_add(work.update_img);
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

WG_PRIVATE wg_boolean
callibration_start(Gui_progress_action action, void *user_data)
{
    Camera *cam = NULL;
    Sensor *sensor = NULL;
    const gchar *device = NULL;
    wg_status status = WG_FAILURE;
    pthread_attr_t attr;
    Callibration_data *data = NULL;

    data = (Callibration_data*)user_data;

    cam = data->camera;

    switch (action){
    case GUI_PROGRESS_ENTER:
        if (data->is_camera_initialized == WG_TRUE){
            break;
        }
        gtk_widget_set_sensitive(cam->start_capturing, FALSE);

        if (go_to_state(cam, WEBCAM_STATE_CALLIBRATE) == WG_TRUE){
            data->is_camera_initialized = WG_TRUE;
            sensor = WG_MALLOC(sizeof (*sensor));
            if (NULL != sensor){
                cam->sensor = sensor;


                device = gtk_combo_box_text_get_active_text(
                        GTK_COMBO_BOX_TEXT(cam->device_combo));

                get_selected_resolution(cam->resolution_combo,
                        &sensor->width, &sensor->height);

                strncpy(sensor->video_dev, device, VIDEO_SIZE_MAX);

                status = sensor_init(cam->sensor);
                if (WG_SUCCESS != status){
                    WG_FREE(cam->sensor);
                    cam->sensor = NULL;
                    return WG_FALSE;
                }

                sensor_noise_reduction_set_state(cam->sensor, 
                        gtk_toggle_button_get_active(
                            GTK_TOGGLE_BUTTON(cam->noise_reduction)
                            ));

                if (WG_SUCCESS == status){
                    sensor_set_default_cb(cam->sensor,
                            (Sensor_def_cb)callibration_default_cb, cam);

                    pthread_attr_init(&attr);
                    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
                    pthread_create(&cam->thread, &attr, capture, cam);
                    pthread_attr_destroy(&attr);
                }else{
                    WG_FREE(cam->sensor);
                    cam->sensor = NULL;
                }
            }
        }else{
            /* @todo Handle thsi through MessageBox */
        }
        break;
    case GUI_PROGRESS_BACK:
        stop_capture(cam);
        break;
    default:
        break;
    }

    return WG_TRUE;
}

WG_PRIVATE wg_boolean
callibration_screen(Gui_progress_action action, void *user_data)
{
    wg_boolean exit_perm = WG_FALSE;
    Camera    *cam = NULL;
    Callibration_data *data = NULL;
    Cd_pane pane;
    wg_status status = WG_FAILURE;

    data = (Callibration_data*)user_data;

    cam = data->camera;
    switch (action){
    case GUI_PROGRESS_ENTER:
        g_signal_connect(cam->left_area, "button-release-event", 
                G_CALLBACK(screen_corner_release_mouse), data);
        data->corner_count = 0;
        break;
    case GUI_PROGRESS_NEXT:
        exit_perm = (data->corner_count == SCREEN_CORNER_NUM);
        break;
    case GUI_PROGRESS_LEAVE:
        g_signal_handlers_disconnect_by_func(cam->left_area,
                G_CALLBACK(screen_corner_release_mouse), data);

        pane.v1 = data->corners[0];
        pane.v2 = data->corners[1];
        pane.v3 = data->corners[2];
        pane.v4 = data->corners[3];
        pane.orientation = CD_PANE_RIGHT;

        status = cd_define_pane(&pane, &cam->cd);
        if (WG_SUCCESS == status){
            exit_perm = WG_TRUE;
        }
        break;
    default:
        break;
    }

    return exit_perm;
}

WG_PRIVATE wg_boolean
callibration_color(Gui_progress_action action, void *user_data)
{
    Camera    *cam = NULL;
    Callibration_data *data = NULL;

    data = (Callibration_data*)user_data;

    cam = data->camera;
    switch (action){
    case GUI_PROGRESS_ENTER:
        cam->dragging = WG_FALSE;

        g_signal_connect(cam->left_area, "button-press-event", 
                G_CALLBACK(pressed_mouse), cam);

        g_signal_connect(cam->left_area, "button-release-event", 
                G_CALLBACK(released_mouse), cam);
        break;
    case GUI_PROGRESS_LEAVE:
        g_signal_handlers_disconnect_by_func(cam->left_area,
                G_CALLBACK(pressed_mouse), cam);

        g_signal_handlers_disconnect_by_func(cam->left_area,
                G_CALLBACK(released_mouse), cam);

        break;
    default:
        break;
    }
    return WG_TRUE;
}

WG_PRIVATE wg_boolean
callibration_finish(Gui_progress_action action, void *user_data)
{
    Camera    *cam = NULL;
    Callibration_data *data = NULL;

    data = (Callibration_data*)user_data;
    cam = data->camera;

    switch (action){
    case GUI_PROGRESS_LEAVE:
        sensor_get_color_range(cam->sensor, &cam->top, &cam->bottom);

        stop_capture(cam);

        gtk_widget_set_sensitive(cam->start_capturing, TRUE);

        WG_FREE(data);
        break;
    default:
        break;
    }

    return WG_TRUE;
}

void
gui_callibration_screen(Camera *cam)
{
    Gui_progress_dialog *pd = NULL;
    Callibration_data *data = NULL;

    CHECK_FOR_NULL_PARAM(cam);

    pd = gui_progress_dialog_new();
    data = WG_MALLOC(sizeof (Callibration_data));

    data->camera = cam;
    data->is_camera_initialized = WG_FALSE;

    gui_progress_dialog_add_screen(pd, 
       gui_progress_dialog_screen_new(callibration_start, data, 
       "Welcome to calibration wizard. This process is very simple and "
       "will take only few seconds.\n\n\n"
       "Click Next to start calibration")
       );

    gui_progress_dialog_add_screen(pd, 
       gui_progress_dialog_screen_new(callibration_screen, data, 
       "Show me where is the screen by clicking in each corner of the screen")
       );

    gui_progress_dialog_add_screen(pd, 
       gui_progress_dialog_screen_new(callibration_color, data, 
       "Select color range on the object you are using")
       );

    gui_progress_dialog_add_screen(pd, 
       gui_progress_dialog_screen_new(callibration_finish, data, 
       "Thank you\n\n\nHave fun")
       );

    gui_progress_dialog_show(pd);
}
