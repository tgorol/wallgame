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
#include <wg_linked_list.h>

#include "include/wg_cam.h"
#include "include/wg_cam_cap.h"
#include "include/wg_cam_image.h"

/*! \defgroup webcam Webcam
 */

/*! @{ */


/**
 * @brief Initialize a webcam
 *
 * @param cam      memory to store webcam instance
 * @param dev_path webcam device name
 *
 * @retval WG_CAM_SUCCESS
 * @retval WG_CAM_FAILURE
 */
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

/**
 * @brief Open a webcam
 *
 * @param cam webcam instance
 *
 * @retval WG_CAM_SUCCESS
 * @retval WG_CAM_FAILURE
 */
wg_cam_status
wg_cam_open(Wg_camera *cam)
{
    wg_cam_status status = WG_CAM_FAILURE;
    struct stat st; 

    CHECK_FOR_NULL_PARAM(cam);


    if (-1 == stat (cam->dev_path, &st)) {
        WG_LOG("Cannot identify '%s': %d, %s\n",
                cam->dev_path, errno, strerror (errno));
        return WG_CAM_FAILURE;
    }

    if (!S_ISCHR (st.st_mode)) {
        WG_LOG("%s is no device\n", cam->dev_path);
        return WG_CAM_FAILURE;
    }

    cam->fd_cam = open(cam->dev_path, O_RDWR, 0);
    if (-1 == cam->fd_cam){
        WG_ERROR("%s", strerror(errno));
        return WG_CAM_FAILURE;
    }

    status = wg_cam_cap_get(cam);
    if (WG_CAM_FAILURE == status){
        close(cam->fd_cam);
        cam->fd_cam = -1;
        return WG_CAM_FAILURE;
    }

    status = wg_cam_image_format_get(cam, WG_CAM_OUT_VIDEO_CAPTURE, 
            &cam->fmt[WG_CAM_FMT_CAPTURE]);
    if (WG_CAM_FAILURE == status){
        close(cam->fd_cam);
        cam->fd_cam = -1;
        return WG_CAM_FAILURE;
    }

    return WG_CAM_SUCCESS;
}

/**
 * @brief Close a webcam
 *
 * @param cam  webcam instance
 *
 * @retval WG_CAM_SUCCESS
 * @retval WG_CAM_FAILURE
 */
wg_cam_status
wg_cam_close(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    CHECK_FOR_COND(cam->fd_cam != -1);

    close(cam->fd_cam);

    return WG_CAM_SUCCESS;
}

/*! @} */
