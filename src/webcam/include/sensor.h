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
typedef void (*Sensor_xy_cb)(const Sensor *sensor, wg_uint x, wg_uint y);

struct Sensor{
    wg_char video_dev[VIDEO_SIZE_MAX + 1];
    wg_uint width;
    wg_uint height;

    /* internal use fields. DO NOT TOUCH */
    Sensor_state state;
    Wg_camera camera;
    Wg_image background;
    Sensor_def_cb cb[CB_NUM];
    void *user_data[CB_NUM];
    pthread_mutex_t lock;
    pthread_cond_t finish;
    wg_boolean complete_request;
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

#endif
