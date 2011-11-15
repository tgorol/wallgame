#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/videodev2.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include "include/wg_cam.h"
#include "include/wg_cam_cap.h"


wg_cam_status
wg_cam_init(Wg_camera *cam, wg_char* dev_path)
{
    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(dev_path);

    memset(cam, '\0', sizeof (Wg_camera));

    strncpy(cam->dev_path, dev_path, DEV_PATH_MAX);

    cam->fd_cam = -1;

    return WG_CAM_SUCCESS;
}

wg_cam_status
wg_cam_open(Wg_camera *cam)
{
    wg_cam_status status = WG_CAM_FAILURE;
    CHECK_FOR_NULL_PARAM(cam);

    cam->fd_cam = open(cam->dev_path, O_RDWR | O_NONBLOCK, 0);
    if (-1 == cam->fd_cam){
        WG_ERROR("%s", strerror(errno));
        return WG_CAM_FAILURE;
    }

    status = wg_cam_cap_read(cam);
    if (WG_CAM_FAILURE == status){
        close(cam->fd_cam);
        cam->fd_cam = -1;
        return WG_CAM_FAILURE;
    }

    return WG_CAM_SUCCESS;
}

wg_cam_status
wg_cam_close(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    CHECK_FOR_COND(cam->fd_cam != -1);

    close(cam->fd_cam);

    return WG_CAM_SUCCESS;
}

