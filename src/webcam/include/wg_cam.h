#ifndef _WG_CAM_H
#define _WG_CAM_H


#define DEV_PATH_MAX   64

typedef struct Wg_camera{
    char dev_path[DEV_PATH_MAX + 1];
    int fd_cam;
    struct v4l2_capability cap;
}Wg_camera;

typedef enum wg_cam_status{
    WG_CAM_SUCCESS = WG_SUCCESS ,
    WG_CAM_FAILURE = WG_FAILURE ,
    WG_CAM_BUSY    = 10         ,
    WG_CAM_INVAL                ,
}wg_cam_status;

WG_PUBLIC wg_cam_status wg_cam_init(Wg_camera *cam, wg_char* dev_path);

WG_PUBLIC wg_cam_status wg_cam_open(Wg_camera *cam);

WG_PUBLIC wg_cam_status wg_cam_close(Wg_camera *cam);

#endif
