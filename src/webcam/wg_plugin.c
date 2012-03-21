#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <linux/videodev2.h>

#include <wg_linked_list.h>
#include <wg_sync_linked_list.h>
#include <wg_wq.h>

#include "include/cam.h"
#include "include/img.h"
#include "include/vid.h"

#include "include/gui_prim.h"
#include "include/gui_work.h"
#include "include/sensor.h"
#include "include/collision_detect.h"
#include "include/wg_plugin.h"

/** @brief FPS interwal counter     */
#define FPS_INTERVAL   0.5

/** @brief Default resolution */
#define RESOLUTION_DEFAULT_INDEX    0

typedef struct Resolution{
    wg_char text[16];        /*!< text of the resolution */
    wg_uint  width;          /*!< width in pixels        */
    wg_uint  height;         /*!< height in pixels       */
}Resolution;

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

/** 
* @brief List of supported resolutions
*/
WG_PRIVATE const Resolution res_info[] = {
    {"352x288", 352, 288,} ,
    {"320x240", 320, 240,} ,
    {"176x144", 176, 144,} ,
    {"160x120", 160, 120,}
};

WG_STATIC void
start_fps(Camera *obj)
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
