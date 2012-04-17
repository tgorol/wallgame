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
#include <img.h>
#include <cam.h>

#include "include/sensor.h"

#include "include/ef_engine.h"

/*! \defgroup sensor Object Detector
*  \ingroup plugin_webcam
*/

/*! @{ */

/*! \brief Number of buffers
 *
 *  Do not increase this value above 100 
 */
#define BG_FRAMES_NUM 25

WG_PRIVATE void
call_user_callback(const Sensor *const sensor, Sensor_cb_type type, 
        const Wg_image *const image);

WG_PRIVATE void
call_user_xy_callback(const Sensor *const sensor, wg_uint x, wg_uint y);

/** 
* @brief Initialize sensor
* 
* @param sensor sensor instance
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
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

/** 
* @brief Release resource allocated by sensor
* 
* @param sensor sensor instance
*/
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

/** 
* @brief Enable noise reduction
* 
* @param sensor sensor instance
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
sensor_start_noise_reduction(Sensor *sensor)
{
    CHECK_FOR_NULL_PARAM(sensor);

    sensor_noise_reduction_set_state(sensor, WG_TRUE);

    return WG_SUCCESS;
}

/** 
* @brief Stop noise reduction
* 
* @param sensor sensor instance
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
sensor_stop_noise_reduction(Sensor *sensor)
{
    CHECK_FOR_NULL_PARAM(sensor);

    sensor_noise_reduction_set_state(sensor, WG_FALSE);

    return WG_SUCCESS;
}

/** 
* @brief Enable/Disable noise reduction
*
* If state == WG_TRUE enable noise reduction
* 
* @param sensor sensor instance
* @param state  noise reduction state.
* 
* @return 
*/
wg_status
sensor_noise_reduction_set_state(Sensor *sensor, wg_boolean state)
{
    CHECK_FOR_NULL_PARAM(sensor);

    pthread_mutex_lock(&sensor->lock);
    sensor->noise_reduction = state;
    pthread_mutex_unlock(&sensor->lock);

    return WG_SUCCESS;
}

/** 
* @brief Get noise reduction state
* 
* @param sensor sensor instance
* 
* @retval WG_TRUE enabled
* @retval WG_TRUE disabled
*/
wg_boolean
sensor_get_noise_reduction_state(Sensor *sensor)
{
    wg_boolean flag = WG_FALSE;

    pthread_mutex_lock(&sensor->lock);
    flag = sensor->noise_reduction;
    pthread_mutex_unlock(&sensor->lock);

    return flag;
}

/** 
* @brief Get color range for object detection
* 
* @param sensor  sensor instance
* @param top     top color
* @param bottom  bottom color
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
sensor_get_color_range(const Sensor *sensor, Hsv *top, Hsv *bottom)
{
    CHECK_FOR_NULL_PARAM(sensor);
    CHECK_FOR_NULL_PARAM(top);
    CHECK_FOR_NULL_PARAM(bottom);

    *top    = sensor->top;
    *bottom = sensor->bottom;

    return WG_SUCCESS;
}

/** 
* @brief Set sensor top color
* 
* @param sensor sensor instance
* @param color  top color
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
sensor_set_color_top(Sensor *sensor, const Hsv *color)
{
    CHECK_FOR_NULL_PARAM(sensor);
    CHECK_FOR_NULL_PARAM(color);

    pthread_mutex_lock(&sensor->lock);

    sensor->top = *color;

    pthread_mutex_unlock(&sensor->lock);

    return WG_SUCCESS;
}

/** 
* @brief Set sensor bottom color
* 
* @param sensor sensor instance
* @param color  bottom color
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
sensor_set_color_bottom(Sensor *sensor, const Hsv *color)
{
    CHECK_FOR_NULL_PARAM(sensor);
    CHECK_FOR_NULL_PARAM(color);

    pthread_mutex_lock(&sensor->lock);

    sensor->bottom = *color;

    pthread_mutex_unlock(&sensor->lock);

    return WG_SUCCESS;
}

/** 
* @brief Set color range
* 
* @param sensor  sensor instance
* @param top     top color
* @param bottom  bottom color
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
sensor_set_color_range(Sensor *sensor, const Hsv *top, const Hsv *bottom)
{
    CHECK_FOR_NULL_PARAM(sensor);
    CHECK_FOR_NULL_PARAM(top);
    CHECK_FOR_NULL_PARAM(bottom);

    sensor_set_color_top(sensor, top);
    sensor_set_color_bottom(sensor, bottom);

    return WG_SUCCESS;
}

/** 
* @brief Reset color range
* 
* @param sensor sensor instance
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
sensor_reset_color_range(Sensor *sensor)
{
    static Hsv top = { 0.0, 0.0, 0.0 };
    static Hsv bottom = { 1.0, 1.0, 1.0 };

    return sensor_set_color_range(sensor, &top, &bottom);
}

/** 
* @brief Add color for object detection
* 
* @param sensor sensor instance
* @param color  color value
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
sensor_add_color(Sensor *sensor, const Hsv *color)
{
    wg_double h = 0.0;
    wg_double s = 0.0;
    wg_double v = 0.0;

    CHECK_FOR_NULL_PARAM(sensor);
    CHECK_FOR_NULL_PARAM(color);

    h = 0.0;
    s = 0.0;
    v = 0.0;

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


/** 
* @brief Start sensor
*
* This function must be called from a seperate thread context.
* 
* @param sensor sensor instance
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
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
           
            call_user_callback(sensor, CB_IMG, &image);

            call_user_xy_callback(sensor, x, y);

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

/** 
* @brief Stop sensor
* 
* @param sensor sensor instance
* 
* @retval WG_SUCCESS
* @retval WG_FALSE
*/
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

/** 
* @brief Set default callback
* 
* @param sensor    sensor instance
* @param cb        callback function
* @param user_data user defined data
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
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

/** 
* @brief Set specific callback
* 
* @param sensor    sensor instance
* @param type      callback type
* @param cb        callback function
* @param user_data user defined data
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
sensor_set_cb(Sensor *sensor, Sensor_cb_type type, 
        Sensor_def_cb cb, void *user_data)
{
    CHECK_FOR_NULL_PARAM(sensor);

    sensor->cb[type] = cb;
    sensor->user_data[type] = user_data;

    return WG_SUCCESS;
}

/** 
* @brief Get specific callback
* 
* @param sensor sensor instance
* @param type   callback type
* @param cb     callback function
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
sensor_get_cb(Sensor *sensor, Sensor_cb_type type, Sensor_def_cb *cb)
{
    CHECK_FOR_NULL_PARAM(sensor);
    CHECK_FOR_NULL_PARAM(cb);

    *cb = sensor->cb[type];

    return WG_SUCCESS;
}


WG_PRIVATE void
call_user_xy_callback(const Sensor *const sensor, wg_uint x, wg_uint y)
{
    register Sensor_xy_cb user_callback = NULL;
    void *user_data = NULL;

    user_data     = sensor->user_data[CB_XY];
    user_callback = (Sensor_xy_cb)sensor->cb[CB_XY];
    if (NULL != user_callback){
        user_callback(sensor, CB_XY, x, y, user_data);
    }

    return;
}


WG_PRIVATE void
call_user_callback(const Sensor *const sensor, Sensor_cb_type type, 
        const Wg_image *const image)
{
    register Sensor_cb user_callback = NULL;

    user_callback = (Sensor_cb)sensor->cb[type];
    if (NULL != user_callback){
        user_callback(sensor, type, image, sensor->user_data[type]);
    }

    return;
}

/*! @} */
