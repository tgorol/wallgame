#ifndef _SENSOR_H
#define _SENSOR_H

#define VIDEO_SIZE_MAX 100

/** 
* @brief Sensor callback id
*/
typedef enum Sensor_cb_type {
    CB_IMG         = 0,        /*!< image read                             */
    CB_IMG_GS         ,        /*!< iamge in grayscale                     */
    CB_IMG_ACC        ,        /*!< accumulator                            */
    CB_IMG_EDGE       ,        /*!< image edge                             */
    CB_IMG_SMOTH      ,        /*!< image smooth                           */
    CB_SETUP_START    ,        /*!< sensor started                         */
    CB_SETUP_STOP     ,        /*!< sensor stoped                          */
    CB_SETUP_ERROR    ,        /*!< sensor error                           */
    CB_EXIT           ,        /*!< sensor exited                          */
    CB_ENTER          ,        /*!< sensor entered                         */
    CB_XY             ,        /*!< sendor hit                             */

    CB_NUM                     /*!< number of callback ids                 */
}Sensor_cb_type;

/** 
* @brief Sensor state
*
* @todo check if needed
*/
typedef enum Sensor_state{
    SENSOR_STARTED    = 0  ,   /*!< sensor started                         */
    SENSOR_STOPED              /*!< sensor stoped                          */
}Sensor_state;

typedef struct Sensor Sensor;

typedef void (*Sensor_def_cb)(const Sensor *, ...);
typedef void (*Sensor_cb)(const Sensor *, Sensor_cb_type, ...);
typedef void (*Sensor_xy_cb)(const Sensor *sensor, Sensor_cb_type type, 
        wg_uint x, wg_uint y, void *user_data);
typedef wg_int (*Sensor_hook_int)(const Sensor *sensor, void *data);

/** 
* @brief Sensor instance structure
*/
struct Sensor{
    wg_char video_dev[VIDEO_SIZE_MAX + 1]; /*!< device binded to sensor     */
    wg_uint width;                         /*!< width of image              */
    wg_uint height;                        /*!< height of image             */
    Hsv bottom;                            /*!< color range bottom          */
    Hsv top;                               /*!< color range top             */

    /* internal use fields. DO NOT TOUCH */
    Sensor_state state;                    /*!< sensor state                */
    Wg_camera camera;                      /*!< camera instance             */
    wg_boolean noise_reduction;            /*!< noise reduction enabled     */

    Sensor_def_cb cb[CB_NUM];              /*!< sensor callback             */
    void *user_data[CB_NUM];               /*!< user data                   */

    pthread_mutex_t lock;                  /*!< lock                        */
    pthread_cond_t finish;                 /*!< finish condition            */
    wg_boolean complete_request;           /*!< sensor finished             */

    Wg_wq detection_wq;                    /*!< workq detection             */
};

WG_PUBLIC wg_status
sensor_init(Sensor *sensor);

WG_PUBLIC void
sensor_cleanup(Sensor *sensor);

WG_PUBLIC wg_status
sensor_start(Sensor *sensor);

WG_PUBLIC wg_status
sensor_stop(Sensor *sensor);

WG_PUBLIC wg_status
sensor_set_default_cb(Sensor *sensor, Sensor_def_cb cb, void *user_data);

WG_PUBLIC wg_status
sensor_set_cb(Sensor *sensor, Sensor_cb_type type, 
        Sensor_def_cb cb, void *user_data);

WG_PUBLIC wg_status
sensor_get_cb(Sensor *sensor, Sensor_cb_type type, Sensor_def_cb *cb);

WG_PUBLIC wg_status
sensor_add_color(Sensor *sensor, const Hsv *color);

WG_PUBLIC wg_status
sensor_start_noise_reduction(Sensor *sensor);

WG_PUBLIC wg_status
sensor_stop_noise_reduction(Sensor *sensor);

WG_PUBLIC wg_boolean
sensor_get_noise_reduction_state(Sensor *sensor);

WG_PUBLIC wg_boolean
sensor_get_noise_reduction_set_state(Sensor *sensor);

WG_PUBLIC wg_status
sensor_noise_reduction_set_state(Sensor *sensor, wg_boolean state);

WG_PUBLIC wg_status
sensor_get_color_range(const Sensor *sensor, Hsv *top, Hsv *bottom);

WG_PUBLIC wg_status
sensor_set_color_top(Sensor *sensor, const Hsv *color);

WG_PUBLIC wg_status
sensor_set_color_bottom(Sensor *sensor, const Hsv *color);

WG_PUBLIC wg_status
sensor_set_color_range(Sensor *sensor, const Hsv *top, const Hsv *bottom);

WG_PUBLIC wg_status
sensor_reset_color_range(Sensor *sensor);

#endif
