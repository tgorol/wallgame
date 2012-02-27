#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <setjmp.h>
#include <pthread.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_linked_list.h>
#include <wg_sync_linked_list.h>
#include <wg_wq.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/cam.h"
#include "include/img.h"
#include "include/img_bgrx.h"
#include "include/img_rgb24.h"
#include "include/img_draw.h"
#include "include/img_gs.h"
#include "include/img_hsv.h"
#include "include/cam_frame.h"

#include "include/sensor.h"

#include "include/ef_engine.h"

/* Do not increase this value above 100 */
#define BG_FRAMES_NUM 25

#define TH_HIGH    240
#define TH_LOW     100

typedef struct Workq_data{
    Wg_image img;
    Sensor *sensor;
}Workq_data;


WG_PRIVATE wg_status
collect_frames(Sensor *sensor, Wg_image *img, wg_uint num);

WG_PRIVATE void
release_frames(Wg_image *img, wg_int num);

WG_PRIVATE wg_status
convert_frames(Wg_image *img, wg_uint num);

WG_PRIVATE wg_status
get_background(Wg_image *img, wg_uint num, Wg_image *bg);

WG_PRIVATE void
get_average_pixel(Wg_image *img, wg_uint num,
        wg_uint y, wg_uint x, gray_pixel *pixel);

WG_PRIVATE wg_status
setup(Sensor *sensor);

WG_PRIVATE void
call_user_callback(const Sensor *const sensor, Sensor_cb_type type, 
        const Wg_image *const image);

wg_status
sensor_init(Sensor *sensor)
{
    cam_status c_status = CAM_FAILURE;
    cam_status status = CAM_SUCCESS;
    wg_uint i = 0;

    CHECK_FOR_NULL_PARAM(sensor);

    c_status = cam_init(&sensor->camera, sensor->video_dev);
    if (CAM_SUCCESS != c_status){
        status = WG_FAILURE;
    }
 
    /* clear callbacks */
    for (i = 0; i < ELEMNUM(sensor->cb); ++i){
        sensor->cb[i] = NULL;
        sensor->user_data[i] = NULL;
    }

    pthread_mutex_init(&sensor->lock, NULL);
    pthread_cond_init(&sensor->finish, NULL);

    sensor->state = SENSOR_STOPED;

    wg_wq_init(&sensor->detection_wq);

    sensor->top.val = 0.0;
    sensor->top.sat = 0.0;
    sensor->top.hue = 0.0;

    sensor->bottom.val = 1.0;
    sensor->bottom.sat = 1.0;
    sensor->bottom.hue = 1.0;

    sensor->noise_reduction = WG_FALSE;

    return status;
}

wg_status
sensor_start_noise_reduction(Sensor *sensor)
{
    CHECK_FOR_NULL_PARAM(sensor);

    sensor_noise_reduction_set_state(sensor, WG_TRUE);

    return WG_SUCCESS;
}

wg_status
sensor_stop_noise_reduction(Sensor *sensor)
{
    CHECK_FOR_NULL_PARAM(sensor);

    sensor_noise_reduction_set_state(sensor, WG_FALSE);

    return WG_SUCCESS;
}

wg_status
sensor_noise_reduction_set_state(Sensor *sensor, wg_boolean state)
{
    CHECK_FOR_NULL_PARAM(sensor);

    pthread_mutex_lock(&sensor->lock);
    sensor->noise_reduction = state;
    pthread_mutex_unlock(&sensor->lock);

    return WG_SUCCESS;
}

wg_boolean
sensor_get_noise_reduction_state(Sensor *sensor)
{
    wg_boolean flag = WG_FALSE;

    pthread_mutex_lock(&sensor->lock);
    flag = sensor->noise_reduction;
    pthread_mutex_unlock(&sensor->lock);

    return flag;
}


wg_status
sensor_add_color(Sensor *sensor, const Hsv *color)
{
    wg_double h = 0.0;
    wg_double s = 0.0;
    wg_double v = 0.0;

    CHECK_FOR_NULL_PARAM(sensor);
    CHECK_FOR_NULL_PARAM(color);

    h = color->hue * 0.02;
    s = color->sat * 0.02;
    v = color->val * 0.02;

    pthread_mutex_lock(&sensor->lock);

    sensor->top.val =
        color->val > sensor->top.val ? color->val + v : sensor->top.val;

    sensor->top.sat =
        color->sat > sensor->top.sat ? color->sat + s : sensor->top.sat;

    sensor->top.hue =
        color->hue > sensor->top.hue ? color->hue + h: sensor->top.hue;

    sensor->bottom.val =
        color->val < sensor->bottom.val ? color->val - v: sensor->bottom.val;

    sensor->bottom.sat =
        color->sat < sensor->bottom.sat ? color->sat - s: sensor->bottom.sat;

    sensor->bottom.hue =
        color->hue < sensor->bottom.hue ? color->hue - h : sensor->bottom.hue;

    pthread_mutex_unlock(&sensor->lock);

    return WG_SUCCESS;
}

void
sensor_cleanup(Sensor *sensor)
{
    /* stop plugin just in case */
    sensor_stop(sensor);

    pthread_cond_destroy(&sensor->finish);
    pthread_mutex_destroy(&sensor->lock);

    wg_wq_cleanup(&sensor->detection_wq);

    return;
}

wg_status
sensor_start(Sensor *sensor)
{
    Wg_frame frame;
    Wg_image image;
    Wg_image acc;
    Wg_image edge_image;
    Wg_image hsv_image;
    Wg_image filtered_image;
    Wg_image tmp_image;
    Wg_image bgrx_image;
    Img_draw ctx;
    rgb24_pixel color;
    Hsv top;
    Hsv bottom;
    Wg_cam_decompressor decomp;
    union{
        cam_status cam;
        wg_status  wg;
    }status;
    wg_uint x = 0;
    wg_uint y = 0;
    wg_uint v = 0;

    ef_init();

    color[0] = 0;
    color[1] = 0;
    color[2] = 255;

    status.cam = cam_open(&sensor->camera, 0, ENABLE_DECOMPRESSOR);
    if (CAM_SUCCESS != status.cam){
        return WG_FAILURE;
    }

    status.cam = cam_set_resolution(&sensor->camera, 
            sensor->width, sensor->height);
    if (CAM_SUCCESS != status.cam){
        return WG_FAILURE;
    }

    status.cam = cam_start(&sensor->camera);
    if (CAM_SUCCESS != status.cam){
        return WG_FAILURE;
    }

    call_user_callback(sensor, CB_SETUP_START, NULL);

#if 0
    status.wg = setup(sensor);
    if (WG_SUCCESS != status.wg){
        cam_close(&sensor->camera);
        call_user_callback(sensor, CB_SETUP_ERROR, NULL);
        return WG_FAILURE;
    }
#endif

    call_user_callback(sensor, CB_SETUP_STOP, NULL);

    cam_frame_init(&frame);
    cam_decompressor(&sensor->camera, &decomp);

    pthread_mutex_lock(&sensor->lock);
    sensor->state = SENSOR_STARTED;
    pthread_mutex_unlock(&sensor->lock);

    call_user_callback(sensor, CB_ENTER, NULL);

    for (;;){
        pthread_mutex_lock(&sensor->lock);
        if (WG_TRUE == ((volatile Sensor *)sensor)->complete_request){
            sensor->complete_request = WG_FALSE;
            sensor->state = SENSOR_STOPED;
            cam_stop(&sensor->camera);
            cam_close(&sensor->camera);
            pthread_cond_signal(&sensor->finish);
            pthread_mutex_unlock(&sensor->lock);
            return WG_SUCCESS;
        }
        pthread_mutex_unlock(&sensor->lock);

        if (cam_read(&sensor->camera, &frame) == CAM_SUCCESS){
            status.cam = invoke_decompressor(&decomp, 
                    frame.start, frame.size, 
                    frame.width, frame.height, &image);

            cam_discard_frame(&sensor->camera, &frame);

            img_rgb_2_bgrx(&image, &bgrx_image);
            img_cleanup(&image);

            if (sensor_get_noise_reduction_state(sensor) == WG_TRUE){
                img_bgrx_median_filter(&bgrx_image, &tmp_image);
                img_cleanup(&bgrx_image);

                bgrx_image = tmp_image;
            }

            img_bgrx_2_rgb(&bgrx_image, &image);
            img_cleanup(&bgrx_image);

            img_rgb_2_hsv_gtk(&image, &hsv_image);

            pthread_mutex_lock(&sensor->lock);
            top    = sensor->top;
            bottom = sensor->bottom;
            pthread_mutex_unlock(&sensor->lock);

            ef_filter(&hsv_image, &filtered_image, &top, &bottom);

            ef_threshold(&filtered_image, 1);

            ef_detect_edge(&filtered_image, &edge_image);

            call_user_callback(sensor, CB_IMG_EDGE, &edge_image);

#ifndef G_GENTER
            ef_detect_circle(&edge_image, &acc);

            ef_acc_get_max(&acc, &y, &x, &v);

            call_user_callback(sensor, CB_IMG_ACC, &acc);

            img_cleanup(&acc);
#else
            ef_center(&edge_image, &y, &x);
            call_user_callback(sensor, CB_IMG_ACC, NULL);
#endif  /* G_CENTER */
           
            img_draw_get_context(image.type, &ctx);
            img_draw_cross(&ctx, &image, y, x, &color);
            img_draw_cleanup_context(&ctx);

            call_user_callback(sensor, CB_IMG, &image);

            img_cleanup(&image);
            img_cleanup(&hsv_image);
            img_cleanup(&filtered_image);
            img_cleanup(&edge_image);
        }else{
            pthread_mutex_lock(&sensor->lock);
            sensor->complete_request = WG_FALSE;
            sensor->state = SENSOR_STOPED;
            cam_stop(&sensor->camera);
            cam_close(&sensor->camera);
            pthread_cond_signal(&sensor->finish);
            pthread_mutex_unlock(&sensor->lock);
            return WG_SUCCESS;
        }
    }

    call_user_callback(sensor, CB_EXIT, NULL);

    pthread_mutex_lock(&sensor->lock);
    sensor->complete_request = WG_FALSE;
    sensor->state = SENSOR_STOPED;
    cam_stop(&sensor->camera);
    cam_close(&sensor->camera);
    pthread_cond_signal(&sensor->finish);
    pthread_mutex_unlock(&sensor->lock);
    return WG_SUCCESS;
}

wg_status
sensor_stop(Sensor *sensor)
{
    pthread_mutex_lock(&sensor->lock);

    if (SENSOR_STOPED == sensor->state){
        pthread_mutex_unlock(&sensor->lock);
        return WG_FAILURE;
    }

    sensor->complete_request = WG_TRUE;

    while(WG_TRUE == ((volatile Sensor *)sensor)->complete_request){
        pthread_cond_wait(&sensor->finish, &sensor->lock);
    }

    pthread_mutex_unlock(&sensor->lock);

#if 0
    img_cleanup(&sensor->background);
#endif

    return WG_SUCCESS;
}

wg_status
sensor_set_default_cb(Sensor *sensor, Sensor_def_cb cb, void *user_data)
{
    wg_int i = 0;

    CHECK_FOR_NULL_PARAM(sensor);

    for (i = 0; i < ELEMNUM(sensor->cb); ++i){
        sensor->cb[i] = cb;
        sensor->user_data[i] = user_data;
    }

    return WG_SUCCESS;
}

wg_status
sensor_set_cb(Sensor *sensor, Sensor_cb_type type, 
        Sensor_def_cb cb, void *user_data)
{
    CHECK_FOR_NULL_PARAM(sensor);

    sensor->cb[type] = cb;
    sensor->user_data[type] = user_data;

    return WG_SUCCESS;
}

wg_status
sensor_get_cb(Sensor *sensor, Sensor_cb_type type, Sensor_def_cb *cb)
{
    CHECK_FOR_NULL_PARAM(sensor);
    CHECK_FOR_NULL_PARAM(cb);

    *cb = sensor->cb[type];

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
setup(Sensor *sensor)
{
    Wg_image bg[BG_FRAMES_NUM];
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL_PARAM(sensor);

    status = collect_frames(sensor, bg, ELEMNUM(bg));
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }

    status = convert_frames(bg, ELEMNUM(bg));
    if (WG_SUCCESS != status){
        release_frames(bg, ELEMNUM(bg));
        return WG_FAILURE;
    }

    status = get_background(bg, ELEMNUM(bg), &sensor->background);
    if (WG_SUCCESS != status){
        release_frames(bg, ELEMNUM(bg));
        return WG_FAILURE;
    }

    release_frames(bg, ELEMNUM(bg));

    return WG_SUCCESS;
}

WG_PRIVATE void
call_user_xy_callback(const Sensor *const sensor, wg_uint x, wg_uint y)
{
    register Sensor_xy_cb user_callback = NULL;

    CHECK_FOR_NULL_PARAM(sensor);

    user_callback = (Sensor_xy_cb)sensor->cb[CB_XY];
    if (NULL != user_callback){
        user_callback(sensor, x, y);
    }

    return;
}

WG_PRIVATE void
release_frames(Wg_image *img, wg_int num)
{
    wg_int i = 0;

    for (i = 0; i < num; ++i){
        img_cleanup(&img[i]);
        memset(&img[i], '\0', sizeof (Wg_image));
    }

    return;
}

WG_PRIVATE void
call_user_callback(const Sensor *const sensor, Sensor_cb_type type, 
        const Wg_image *const image)
{
    register Sensor_cb user_callback = NULL;

    CHECK_FOR_NULL_PARAM(sensor);

    user_callback = (Sensor_cb)sensor->cb[type];
    if (NULL != user_callback){
        user_callback(sensor, type, image, sensor->user_data[type]);
    }

    return;
}

WG_PRIVATE void
get_average_pixel(Wg_image *img, wg_uint num,
        wg_uint y, wg_uint x, gray_pixel *pixel)
{
    wg_uint i = 0;
    wg_uint sum = 0;
    gray_pixel *img_pix = 0;

    for (i = 0 ; i < num; ++i){
        img_get_pixel(&img[i], y, x, (wg_uchar**) &img_pix);
        sum += *img_pix;
    }

    *pixel = sum / num;

    return;
}


WG_PRIVATE wg_status
get_background(Wg_image *img, wg_uint num, Wg_image *bg)
{
    Wg_image bg_img;
    cam_status status = CAM_FAILURE;
    wg_uint x = 0;
    wg_uint y = 0;
    gray_pixel *bg_pix = 0;

    status = img_fill(img[0].width, img[0].height, img[0].components_per_pixel,
            img[0].type, &bg_img);
    if (CAM_SUCCESS != status){
        return WG_FAILURE;
    }

    for (x = 0; x < bg_img.width; ++x){
        for (y = 0; y < bg_img.height; ++y){
            img_get_pixel(&bg_img, y, x, (wg_uchar **)&bg_pix);
            get_average_pixel(img, num, y, x, bg_pix);
        }
    }

    *bg = bg_img;

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
convert_frames(Wg_image *img, wg_uint num)
{
    Wg_image *local_img = NULL;
    wg_uint i = 0;
    cam_status status = WG_FAILURE;
    wg_size size = 0;

    size = num * sizeof (Wg_image);
    local_img = WG_ALLOCA(size);
    memset(local_img, '\0', size);

    /* convert images to grayscale pixel format*/
    for (i = 0; i < num ; ++i){
        status = img_rgb_2_grayscale(&img[i], &local_img[i]);
        if (CAM_SUCCESS != status){
            release_frames(local_img, i);
            return WG_FAILURE;
        }
    }

    /* copy converted images */
    for (i = 0; i < num; ++i){
        img_cleanup(&img[i]);
        img[i] = local_img[i];
    }

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
collect_frames(Sensor *sensor, Wg_image *img, wg_uint num)
{
    cam_status status = CAM_FAILURE;
    Wg_cam_decompressor decompressor;
    Wg_frame frame;
    wg_uint i = 0;

    CHECK_FOR_NULL_PARAM(sensor);
    CHECK_FOR_NULL_PARAM(img);

    memset(img, '\0', num * sizeof (Wg_image));

    cam_frame_init(&frame);

    cam_decompressor(&sensor->camera, &decompressor);

    for (i = 0; i < num; ++i){
        status = cam_read(&sensor->camera, &frame);
        if (CAM_SUCCESS != status){
            release_frames(img, i);
            return WG_FAILURE;
        }
        status = invoke_decompressor(&decompressor, 
                frame.start, frame.size, 
                frame.width, frame.height, &img[i]);
        if (CAM_SUCCESS != status){
            cam_discard_frame(&sensor->camera, &frame);
            release_frames(img, i);
            return WG_FAILURE;
        }

        cam_discard_frame(&sensor->camera, &frame);
    }

    return WG_SUCCESS;
}

