#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/* @todo create user space include */
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>
#include <wg_trans.h>
#include <wg_plugin_tools.h>
#include <wg_linked_list.h>
#include <wg_sync_linked_list.h>
#include <wg_wq.h>
#include <img.h>
#include <cam.h>

#include "include/sensor.h"
#include "include/gui_prim.h"
#include "include/gui_progress_dialog.h"
#include "include/vid.h"
#include "include/gui_work.h"

#include "include/collision_detect.h"
#include "include/wg_plugin.h"

#include "include/wg_cam_callibrate.h"
#include "include/gui_display.h"

#include "include/wg_config.h"

#define SCREEN_CORNER_NUM   4
#define COLOR_PANE_R   1.0
#define COLOR_PANE_G   0.0
#define COLOR_PANE_B   0.0

#define COLOR_POINT_R  0.0
#define COLOR_POINT_G  0.0
#define COLOR_POINT_B  1.0

#define POINT_STROKE_SIZE 8
#define LINE_STROKE_SIZE 2
#define BUFFER_SIZE    64

#define PREVIOUS_SETUP_FILENAME "prev.data"
#define PREV_PANE               "pane"
#define PREV_COLOR_TOP          "color_top"
#define PREV_COLOR_BOTTOM       "color_bottom"

#define DEFAULT_HEIGHT  300
#define DEFAULT_WIDTH   500

/** 
* @brief Previous configuration
*/
typedef struct Previous_setup{
    Hsv bottom;           /*!< bottom color range      */
    Hsv top;              /*!< top color range         */
    Cd_pane pane;         /*!< screen                  */
}Previous_setup;

/** 
* @brief Callibration data
*/
typedef struct Callibration_data{
    Camera *camera;                         /*!< camera to calibrate for    */
    wg_boolean is_camera_initialized;       /*!< is initialized             */
    wg_uint corner_count;                   /*!< screen corner counter      */
    Wg_point2d corners[SCREEN_CORNER_NUM];  /*!< screen corners             */
    Previous_setup setup;                   /*!< previous setup             */
    wg_uint load_config:1;                  /*!< load config file           */

    /* for color callibration */
    GtkWidget *load_check_box;
    GtkWidget *sat_scale;
    GtkWidget *val_scale;
}Callibration_data;

/** 
* @brief Setup histogram
*/
typedef struct Setup_hist{
    Wg_rect rect;           /*!< rectangle to get histogram                 */
    Camera *cam;            /*!< camera instance to read histogram from     */
}Setup_hist;

WG_PRIVATE void
clear_pane(Gui_display *display);

WG_PRIVATE wg_status
parse_color(char *value, Hsv *color);

WG_PRIVATE wg_status
parse_pane(char *value, Cd_pane *pane);

WG_PRIVATE wg_status
load_previous_config(const wg_char *filename, Previous_setup *setup);

WG_PRIVATE wg_status
save_config(const wg_char *filename, const Previous_setup *setup);

WG_PRIVATE void
hist_color_cb(Gui_display *display, const Wg_rect *rect, void *user_data);

WG_PRIVATE wg_boolean
callibration_color(Gui_progress_dialog_screen *pds, 
        Gui_progress_action action, void *user_data);

WG_PRIVATE wg_boolean
callibration_finish(Gui_progress_dialog_screen *pds,
        Gui_progress_action action, void *user_data);

WG_PRIVATE wg_boolean
callibration_screen(Gui_progress_dialog_screen *pds,
        Gui_progress_action action, void *user_data);

WG_PRIVATE wg_boolean
callibration_start(Gui_progress_dialog_screen *pds,
        Gui_progress_action action, void *user_data);

WG_PRIVATE void
callibration_exit(wg_uint screen_id, void *user_data);

/** 
* @brief Show callibration screen
* 
* @param cam Camera instance
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
gui_callibration_screen(Camera *cam)
{
    Gui_progress_dialog *pd = NULL;
    Callibration_data *data = NULL;
    GtkWidget *widget       = NULL;
    GtkWidget *box          = NULL;
    GtkWidget *box_row      = NULL;

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
                "Click Next to start calibration", NULL)
            );

    widget = gtk_check_button_new_with_label("Load previous configuration");

    gui_progress_dialog_add_screen(pd, 
            gui_progress_dialog_screen_new(callibration_screen, data, 
                "Outline the game area by clicking on each of the four corners"
                , widget)
            );

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    /* create check box for Load previous configurtion             */
    widget = gtk_check_button_new_with_label("Load previous configuration");
    data->load_check_box = widget;
    gtk_box_pack_start(GTK_BOX(box), widget, TRUE, TRUE, 0);

    /* create Saturation scale                                     */
    box_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    widget = gtk_label_new("Sat");
    gtk_box_pack_start(GTK_BOX(box_row), widget, FALSE, FALSE, 0);

    widget = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 
            0.0, 1.0, 0.01);
    data->sat_scale = widget;
    gtk_box_pack_start(GTK_BOX(box_row), widget, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(box), box_row, TRUE, TRUE, 0);

    /* create value scale                                          */
    box_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    widget = gtk_label_new("Val");
    gtk_box_pack_start(GTK_BOX(box_row), widget, FALSE, FALSE, 0);

    widget = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 
            0.0, 1.0, 0.01);
    data->val_scale = widget;
    gtk_box_pack_start(GTK_BOX(box_row), widget, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(box), box_row, TRUE, TRUE, 0);

    gui_progress_dialog_add_screen(pd, 
            gui_progress_dialog_screen_new(callibration_color, data, 
                 "Hold the ball in front of the camera and select it by "
                 "clicking and then dragging the cursor across the ball"  
                 , box)
            );

    gui_progress_dialog_add_screen(pd, 
            gui_progress_dialog_screen_new(callibration_finish, data, 
                "Thank you\n\n\nHave fun", NULL)
            );

    gui_progress_dialog_show(pd, DEFAULT_WIDTH, DEFAULT_HEIGHT);

    return WG_SUCCESS;
}

WG_PRIVATE void
setup_hist(void *data)
{
    Setup_hist *work = NULL;
    Hsv *color = NULL;
    Wg_image hsv_image;
    wg_uint i = 0;
    wg_size num = 0;
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

WG_PRIVATE void
hist_color_cb(Gui_display *display, const Wg_rect *rect, void *user_data)
{
    Camera *cam = NULL;
    Setup_hist *work = NULL;
    wg_int area = 0;

    wg_rect_get_area(rect, &area);

    if (0 != area){
        cam = (Camera*)user_data;

        work = gui_work_create(sizeof (Setup_hist), setup_hist);

        work->cam = cam;
        work->rect = *rect;

        gui_work_add(work);
    }
}

WG_PRIVATE gboolean
sat_slider_changed(GtkRange *range, GtkScrollType scroll, 
        gdouble value, gpointer user_data)
{
    Hsv bottom_color;
    Hsv top_color;
    Camera *cam = (Camera*) user_data;

    sensor_get_color_range(cam->sensor, &top_color, &bottom_color);

    bottom_color.sat = value * top_color.sat;

    sensor_set_color_bottom(cam->sensor, &bottom_color);

    return FALSE;
}

WG_PRIVATE gboolean
val_slider_changed(GtkRange *range, GtkScrollType scroll, 
        gdouble value, gpointer user_data)
{
    Hsv bottom_color;
    Hsv top_color;
    Camera *cam = (Camera*) user_data;

    sensor_get_color_range(cam->sensor, &top_color, &bottom_color);

    bottom_color.val = value * top_color.val;

    sensor_set_color_bottom(cam->sensor, &bottom_color);

    return FALSE;
}

WG_PRIVATE gboolean
screen_corner_release_mouse(GtkWidget *widget, GdkEvent  *event,
        gpointer user_data)
{
    Callibration_data *data = NULL;
    GdkEventButton *event_button = NULL;
    Wg_point2d p1;
    Wg_point2d p2;

    data = (Callibration_data*)user_data;
    event_button = &event->button;

    data->corner_count %= SCREEN_CORNER_NUM;

    if (data->corner_count == 0){
        gui_display_clean_lines(&data->camera->left_display);
    }

    wg_point2d_new(event_button->x, event_button->y, 
            &data->corners[data->corner_count]);

    WG_LOG("Click at x=%3d y=%3d\n", 
            (int)event_button->x, (int)event_button->y); 

    ++data->corner_count;

    wg_point2d_new(event_button->x, event_button->y, &p1);
    wg_point2d_new(event_button->x, event_button->y, &p2);
    gui_display_draw_line(&data->camera->left_display, &p1, &p2,
            COLOR_POINT_R, COLOR_POINT_G, COLOR_POINT_B, POINT_STROKE_SIZE);

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
callibration_start(Gui_progress_dialog_screen *pds, 
        Gui_progress_action action, void *user_data)
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
        gtk_widget_set_sensitive(cam->resolution_combo, FALSE);
        gtk_widget_set_sensitive(cam->callibrate, FALSE);
        gtk_widget_set_sensitive(cam->device_combo, FALSE);

        if (go_to_state(cam, WEBCAM_STATE_CALLIBRATE) == WG_TRUE){
            data->is_camera_initialized = WG_TRUE;
            sensor = WG_MALLOC(sizeof (*sensor));
            if (NULL != sensor){
                gui_display_clean_lines(&cam->left_display);

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

                    status = load_previous_config(PREVIOUS_SETUP_FILENAME, 
                            &data->setup);
                    if (WG_FAILURE == status){
                        WG_LOG("No previous configuration file\n");
                        data->load_config = 0;
                    }
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
    case GUI_PROGRESS_NEXT:
        break;
    case GUI_PROGRESS_LEAVE:
        break;
    default:
        break;
    }

    return WG_TRUE;
}

WG_PRIVATE void
clear_pane(Gui_display *display)
{
    gui_display_clean_lines(display);

    return;
}

WG_PRIVATE void
paint_pane(Gui_display *display, const Cd_pane *pane_dimention)
{
    Wg_point2d p1;
    Wg_point2d p2;

    gui_display_clean_lines(display);

    wg_point2d_new(pane_dimention->v1.x , pane_dimention->v1.y, &p1); 
    wg_point2d_new(pane_dimention->v2.x , pane_dimention->v2.y, &p2); 
    gui_display_draw_line(display, &p1, &p2,
            COLOR_PANE_R, COLOR_PANE_G, COLOR_PANE_B, LINE_STROKE_SIZE);

    wg_point2d_new(pane_dimention->v2.x , pane_dimention->v2.y, &p1); 
    wg_point2d_new(pane_dimention->v3.x , pane_dimention->v3.y, &p2); 
    gui_display_draw_line(display, &p1, &p2,
            COLOR_PANE_R, COLOR_PANE_G, COLOR_PANE_B, LINE_STROKE_SIZE);

    wg_point2d_new(pane_dimention->v3.x , pane_dimention->v3.y, &p1); 
    wg_point2d_new(pane_dimention->v4.x , pane_dimention->v4.y, &p2); 
    gui_display_draw_line(display, &p1, &p2,
            COLOR_PANE_R, COLOR_PANE_G, COLOR_PANE_B, LINE_STROKE_SIZE);

    wg_point2d_new(pane_dimention->v4.x , pane_dimention->v4.y, &p1); 
    wg_point2d_new(pane_dimention->v1.x , pane_dimention->v1.y, &p2); 
    gui_display_draw_line(display, &p1, &p2,
            COLOR_PANE_R, COLOR_PANE_G, COLOR_PANE_B, LINE_STROKE_SIZE);

    return;
}

WG_PRIVATE void
pane_dimention_to_array(const Cd_pane *pane_dimention, 
        Wg_point2d array[PANE_VERTICLES_NUM])
{
    array[V0] = pane_dimention->v1;
    array[V1] = pane_dimention->v2;
    array[V2] = pane_dimention->v3;
    array[V3] = pane_dimention->v4;

    return;
}

WG_PRIVATE void
pane_dimention_from_array(Cd_pane *pane_dimention, 
        Wg_point2d array[PANE_VERTICLES_NUM])
{
    pane_dimention->v1 = array[V0];
    pane_dimention->v2 = array[V1];
    pane_dimention->v3 = array[V2];
    pane_dimention->v4 = array[V3];

    return;
}

void
load_previous_screen(GtkToggleButton *togglebutton, gpointer user_data)
{
    Callibration_data *data = NULL;
    Cd_pane pane_dimention;

    data = (Callibration_data*)user_data;
    if (gtk_toggle_button_get_active(togglebutton) == TRUE){
        pane_dimention = data->setup.pane;
        pane_dimention_to_array(&pane_dimention, data->corners);
        paint_pane(&data->camera->left_display, &pane_dimention);
        data->corner_count = 4;
    }else{
        data->corner_count = 0;
        clear_pane(&data->camera->left_display);
    }

    return;
}

WG_PRIVATE wg_boolean
callibration_screen(Gui_progress_dialog_screen *pds, 
        Gui_progress_action action, void *user_data)
{
    wg_boolean exit_perm = WG_TRUE;
    Camera    *cam = NULL;
    Callibration_data *data = NULL;
    Cd_pane pane_dimention;
    wg_status status = WG_FAILURE;
    GtkWidget *widget = NULL;
    GtkWidget *check_box = NULL;

    data = (Callibration_data*)user_data;

    check_box = gui_progress_dialog_screen_get_widget(pds);

    cam = data->camera;
    switch (action){
        case GUI_PROGRESS_ENTER:
            widget = cam->left_display.widget;

            g_signal_connect(widget, "button-release-event", 
                    G_CALLBACK(screen_corner_release_mouse), data);
            data->corner_count = 0;

            g_signal_connect(GTK_TOGGLE_BUTTON(check_box), "toggled", 
                    G_CALLBACK(load_previous_screen), data);

            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_box), TRUE);

            break;
        case GUI_PROGRESS_LEAVE:
            widget = cam->left_display.widget;

            g_signal_handlers_disconnect_by_func(widget,
                    G_CALLBACK(screen_corner_release_mouse), data);

            g_signal_handlers_disconnect_by_func(check_box,
                    G_CALLBACK(load_previous_screen), data);
            break;
        case GUI_PROGRESS_NEXT:
            exit_perm = (data->corner_count == SCREEN_CORNER_NUM);
            if (exit_perm == WG_FALSE){
                break;
            }
            widget = cam->left_display.widget;

            g_signal_handlers_disconnect_by_func(widget,
                    G_CALLBACK(screen_corner_release_mouse), data);

            pane_dimention_from_array(&pane_dimention, data->corners);
            pane_dimention.orientation = CD_PANE_RIGHT;

            status = cd_init(&pane_dimention, &cam->cd);
            if (WG_SUCCESS == status){
                exit_perm = WG_TRUE;
            }

            cd_get_pane(&cam->cd, &pane_dimention);
            paint_pane(&cam->left_display, &pane_dimention);

            data->setup.pane = pane_dimention;
            break;
        default:
            break;
    }

    return exit_perm;
}

void
load_previous_color(GtkToggleButton *togglebutton, gpointer user_data)
{
    Callibration_data *data = NULL;
    Camera *cam             = NULL;

    data = (Callibration_data*)user_data;
    cam = data->camera;

    if (gtk_toggle_button_get_active(togglebutton) == TRUE){
        sensor_set_color_range(cam->sensor,
                &data->setup.top,  &data->setup.bottom);
    }else{
        sensor_reset_color_range(cam->sensor);
    }

    return;
}

WG_PRIVATE wg_boolean
callibration_color(Gui_progress_dialog_screen *pds, 
        Gui_progress_action action, void *user_data)
{
    Camera    *cam = NULL;
    Callibration_data *data = NULL;

    data = (Callibration_data*)user_data;

    cam = data->camera;
    switch (action){
        case GUI_PROGRESS_ENTER:
            cam->dragging = WG_FALSE;

            gui_display_set_drag_callback(&cam->left_display,
            hist_color_cb, cam);
            gui_display_enable_dragging(&cam->left_display);

            g_signal_connect(GTK_RANGE(data->sat_scale), "change-value",
                    G_CALLBACK(sat_slider_changed), cam);

            g_signal_connect(GTK_RANGE(data->val_scale), "change-value",
                    G_CALLBACK(val_slider_changed), cam);

            g_signal_connect(GTK_TOGGLE_BUTTON(data->load_check_box),
                    "toggled", G_CALLBACK(load_previous_color), data);

            break;
        case GUI_PROGRESS_LEAVE:
            g_signal_handlers_disconnect_by_func(data->sat_scale,
                    G_CALLBACK(sat_slider_changed), cam);

            g_signal_handlers_disconnect_by_func(data->val_scale,
                    G_CALLBACK(val_slider_changed), cam);

            g_signal_handlers_disconnect_by_func(data->load_check_box,
                    G_CALLBACK(load_previous_color), cam);

            gui_display_disable_dragging(&cam->left_display);

            break;
        case GUI_PROGRESS_NEXT:
            sensor_get_color_range(cam->sensor,
                    &data->setup.top, &data->setup.bottom);

            break;
        default:
            break;
    }
    return WG_TRUE;
}

WG_PRIVATE wg_boolean
callibration_finish(Gui_progress_dialog_screen *pds, Gui_progress_action
        action, void *user_data)
{
    Camera    *cam = NULL;
    Callibration_data *data = NULL;

    data = (Callibration_data*)user_data;
    cam = data->camera;

    switch (action){
        case GUI_PROGRESS_NEXT:
            sensor_get_color_range(cam->sensor, &cam->top, &cam->bottom);

            gtk_widget_set_sensitive(cam->start_capturing, TRUE);

            save_config(PREVIOUS_SETUP_FILENAME, &data->setup);
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

        gtk_widget_set_sensitive(cam->resolution_combo, TRUE);
        gtk_widget_set_sensitive(cam->callibrate, TRUE);
        gtk_widget_set_sensitive(cam->device_combo, TRUE);

        stop_capture(cam);
        WG_FREE(user_data);
    }

    return;
}


WG_PRIVATE wg_status
load_previous_config(const wg_char *filename, Previous_setup *setup)
{
    wg_status status = WG_FAILURE;
    Wg_config config;
    wg_char value[64];

    CHECK_FOR_NULL_PARAM(filename);
    CHECK_FOR_NULL_PARAM(setup);

    status = wg_config_init(filename, &config);
    if (WG_FAILURE == status){
        WG_LOG("No previous configuration file\n");
        return WG_FAILURE;
    }

    status = wg_config_get_value(&config, PREV_COLOR_TOP, value, 
            sizeof (value));
    if (WG_FAILURE == status){
        WG_LOG("Key %s does not exists\n", PREV_COLOR_TOP);
        wg_config_cleanup(&config);
        return WG_FAILURE;
    }

    status = parse_color(value, &setup->top);
    if (WG_FAILURE == status){
        WG_LOG("Key %s parsing error\n", PREV_COLOR_TOP);
        wg_config_cleanup(&config);
        return WG_FAILURE;
    }

    status = wg_config_get_value(&config, PREV_COLOR_BOTTOM, value, 
            sizeof (value));
    if (WG_FAILURE == status){
        WG_LOG("Key %s does not exists\n", PREV_COLOR_BOTTOM);
        wg_config_cleanup(&config);
        return WG_FAILURE;
    }

    status = parse_color(value, &setup->bottom);
    if (WG_FAILURE == status){
        WG_LOG("Key %s parsing error\n", PREV_COLOR_BOTTOM);
        wg_config_cleanup(&config);
        return WG_FAILURE;
    }

    status = wg_config_get_value(&config, PREV_PANE, value, 
            sizeof (value));
    if (WG_FAILURE == status){
        WG_LOG("Key %s does not exists\n", PREV_PANE);
        wg_config_cleanup(&config);
        return WG_FAILURE;
    }

    status = parse_pane(value, &setup->pane);
    if (WG_FAILURE == status){
        WG_LOG("Key %s parsing error\n", PREV_PANE);
        wg_config_cleanup(&config);
        return WG_FAILURE;
    }

    wg_config_cleanup(&config);

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
get_point(char *value, Wg_point2d *point)
{
    wg_uint x = 0;
    wg_uint y = 0;
    char *tok = NULL;

    CHECK_FOR_NULL_PARAM(point);

    tok = strtok(value, " ");
    if (NULL == tok){
        return WG_FAILURE;
    }
    x = atoi(tok); 

    tok = strtok(NULL, " ");
    if (NULL == tok){
        return WG_FAILURE;
    }
    y = atoi(tok); 

    wg_point2d_new(x, y, point);

    return WG_SUCCESS;
}

wg_status
get_orientation(char *value, Cd_orientation *orientation)
{
    char *tok = NULL;

    CHECK_FOR_NULL_PARAM(orientation);

    tok = strtok(value, " ");
    if (NULL == tok){
        return WG_FAILURE;
    }

    *orientation =
        (strcmp(tok, "LEFT") == 0) ? CD_PANE_LEFT : CD_PANE_RIGHT;

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
parse_pane(char *value, Cd_pane *pane)
{
    CHECK_FOR_NULL_PARAM(value);
    CHECK_FOR_NULL_PARAM(pane);

    if (get_point(value, &pane->v1) == WG_FAILURE){
        return WG_FAILURE;
    }
    if (get_point(NULL, &pane->v2) == WG_FAILURE){
        return WG_FAILURE;
    }
    if (get_point(NULL, &pane->v3) == WG_FAILURE){
        return WG_FAILURE;
    }
    if (get_point(NULL, &pane->v4) == WG_FAILURE){
        return WG_FAILURE;
    }

    if (get_orientation(NULL, &pane->orientation) == WG_FAILURE){
        return WG_FAILURE;
    }

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
get_color_component(char *value, wg_double *component)
{
    char *tok = NULL;

    CHECK_FOR_NULL_PARAM(component);

    tok = strtok(value, " ");
    if (NULL == tok){
        return WG_FAILURE;
    }

    *component = atof(tok); 

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
parse_color(char *value, Hsv *color)
{
    CHECK_FOR_NULL_PARAM(value);
    CHECK_FOR_NULL_PARAM(color);

    if (get_color_component(value, &color->hue) == WG_FAILURE){
        return WG_FAILURE;
    }

    if (get_color_component(NULL, &color->sat) == WG_FAILURE){
        return WG_FAILURE;
    }

    if (get_color_component(NULL, &color->val) == WG_FAILURE){
        return WG_FAILURE;
    }

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
create_point(const Wg_point2d* point, char *value, size_t num)
{
    CHECK_FOR_NULL_PARAM(point);
    CHECK_FOR_NULL_PARAM(value);

    snprintf(value, num, "%u %u", point->x, point->y);

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
create_pane(const Cd_pane *pane, char *value, size_t num)
{
    char val[4][BUFFER_SIZE];

    create_point(&pane->v1, val[0], BUFFER_SIZE);
    create_point(&pane->v2, val[1], BUFFER_SIZE);
    create_point(&pane->v3, val[2], BUFFER_SIZE);
    create_point(&pane->v4, val[3], BUFFER_SIZE);

    snprintf(value, num, "%s %s %s %s %s", val[0], val[1], val[2], val[3],
        pane->orientation == CD_PANE_RIGHT ? "RIGHT" : "LEFT");

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
create_color(const Hsv *color, char *value, size_t num)
{
    CHECK_FOR_NULL_PARAM(color);
    CHECK_FOR_NULL_PARAM(value);

    snprintf(value, num, "%lf %lf %lf", color->hue, color->sat, color->val);

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
save_config(const wg_char *filename, const Previous_setup *setup)
{
    wg_status status = WG_FAILURE;
    Wg_config config;
    wg_char value[BUFFER_SIZE];

    CHECK_FOR_NULL_PARAM(filename);
    CHECK_FOR_NULL_PARAM(setup);

    status = wg_config_init(filename, &config);
    if (WG_FAILURE == status){
        return WG_FAILURE;
    }
   
    create_color(&setup->top, value, sizeof (value));
    wg_config_add_value(&config, PREV_COLOR_TOP, value);

    create_color(&setup->bottom, value, sizeof (value));
    wg_config_add_value(&config, PREV_COLOR_BOTTOM, value);

    create_pane(&setup->pane, value, sizeof (value));
    wg_config_add_value(&config, PREV_PANE, value);

    wg_config_cleanup(&config);

    return WG_SUCCESS;
}
