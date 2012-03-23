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
#include "include/gui_display.h"

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
    Hsv *color = NULL;
    Wg_image hsv_image;
    wg_uint i = 0;
    wg_uint num = 0;
    wg_size size = 0;

    work = (Setup_hist*)data;


    gui_display_copy(&work->cam->left_display, &work->rect, &hsv_image);

    img_get_data(&hsv_image, (wg_uchar**)&color, &num, &size);

    for (i = 0; i < num; ++i, ++color){
        sensor_add_color(work->cam->sensor, color);
    }

    img_cleanup(&hsv_image);

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

void
callibration_default_cb(Sensor *sensor, Sensor_cb_type type, Wg_image *img, 
        void *user_data)
{
    Camera *cam = NULL;
    Wg_image rgb_img;
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
    GtkWidget *widget = NULL;

    data = (Callibration_data*)user_data;

    cam = data->camera;
    switch (action){
    case GUI_PROGRESS_ENTER:
        widget = cam->left_display.widget;

        g_signal_connect(widget, "button-release-event", 
                G_CALLBACK(screen_corner_release_mouse), data);
        data->corner_count = 0;
        break;
    case GUI_PROGRESS_NEXT:
        exit_perm = (data->corner_count == SCREEN_CORNER_NUM);
        break;
    case GUI_PROGRESS_LEAVE:
        widget = cam->left_display.widget;

        g_signal_handlers_disconnect_by_func(widget,
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
    GtkWidget *widget = NULL;

    data = (Callibration_data*)user_data;

    cam = data->camera;
    switch (action){
    case GUI_PROGRESS_ENTER:
        cam->dragging = WG_FALSE;

        widget = cam->left_display.widget;

        g_signal_connect(widget, "button-press-event", 
                G_CALLBACK(pressed_mouse), cam);

        g_signal_connect(widget, "button-release-event", 
                G_CALLBACK(released_mouse), cam);
        break;
    case GUI_PROGRESS_LEAVE:
        widget = cam->left_display.widget;

        g_signal_handlers_disconnect_by_func(widget,
                G_CALLBACK(pressed_mouse), cam);

        g_signal_handlers_disconnect_by_func(widget,
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

        gtk_widget_set_sensitive(cam->start_capturing, TRUE);
        break;
    default:
        break;
    }

    return WG_TRUE;
}

WG_PRIVATE void
callibration_exit(wg_uint screen_id, void *user_data)
{
    Camera    *cam = NULL;
    Callibration_data *data = NULL;

    if (screen_id == 0){
        data = (Callibration_data*)user_data;
        cam = data->camera;

        stop_capture(cam);
        WG_FREE(user_data);
    }

    return;
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

    gui_progress_dialog_set_exit_action(pd, callibration_exit);

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
