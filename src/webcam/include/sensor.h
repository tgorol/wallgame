#ifndef _SENSOR_H
#define _SENSOR_H

#define VIDEO_SIZE_MAX 100

typedef enum Sensor_cb_type {
    CB_IMG         = 0,
    CB_IMG_GS         ,
    CB_IMG_ACC        ,
    CB_IMG_EDGE       ,
    CB_IMG_SMOTH      ,
    CB_SETUP_START    ,
    CB_SETUP_STOP     ,
    CB_SETUP_ERROR    ,
    CB_EXIT           ,
    CB_ENTER          ,
    CB_XY             ,

    CB_NUM
}Sensor_cb_type;

typedef enum Sensor_state{
    SENSOR_STARTED    = 0  ,
    SENSOR_STOPED          
}Sensor_state;

typedef struct Sensor Sensor;

typedef void (*Sensor_def_cb)(const Sensor *, ...);
typedef void (*Sensor_cb)(const Sensor *, Sensor_cb_type, ...);
typedef void (*Sensor_xy_cb)(const Sensor *sensor, Sensor_cb_type type, 
        wg_uint x, wg_uint y, void *user_data);
typedef wg_int (*Sensor_hook_int)(const Sensor *sensor, void *data);

struct Sensor{
    wg_char video_dev[VIDEO_SIZE_MAX + 1];
    wg_uint width;
    wg_uint height;
    Hsv bottom;
    Hsv top;

    /* internal use fields. DO NOT TOUCH */
    Sensor_state state;
    Wg_camera camera;
    Wg_image background;
    wg_boolean noise_reduction;

    Sensor_def_cb cb[CB_NUM];
    void *user_data[CB_NUM];

    pthread_mutex_t lock;
    pthread_cond_t finish;
    wg_boolean complete_request;

    Wg_wq detection_wq;
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

wg_status
sensor_set_color_top(Sensor *sensor, const Hsv *color);

wg_status
sensor_set_color_bottom(Sensor *sensor, const Hsv *color);

#endif
