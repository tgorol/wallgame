#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <alloca.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/videodev2.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include "include/wg_cam.h"
#include "include/wg_cam_cap.h"
#include "include/wg_cam_readwrite.h"

/*! \addtogroup webcam 
 */

#define POLL_FLAGS (POLLIN | POLLPRI)

/*! @{ */

/**
 * @brief Read a frame from webcam
 *
 * When frame->start is NULL buffer will be allocated.
 * Frame buffer must be released by wg_cam_frame_buffer_release()
 *
 * @param cam       webcam instance
 * @param buffer    memory for buffer buffer 
 * @param buf_size  size of the buffer
 *
 * @return 
 */
wg_cam_status
wg_cam_frame_read(Wg_camera *cam, Wg_frame *frame)
{
    ssize_t len = 0; 
    wg_ssize size = 0;

    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(frame);

    if ((wg_cam_cap_readwrite(cam) && wg_cam_cap_video_capture(cam)) 
            != WG_TRUE){
        return WG_CAM_FAILURE;
    }

    wg_cam_frame_get_size(cam, &size);

    if (frame->start == NULL){
        if (wg_cam_frame_buffer_alloc(cam, frame) != WG_CAM_SUCCESS){
            return WG_CAM_FAILURE;
        }
    }

    CHECK_FOR_COND(frame->size >= size);

    len = read(cam->fd_cam, frame->start, frame->size);
    if (-1 == len){
        switch (errno){
            case EIO:
                return WG_CAM_IO;
                break;
            default:
                WG_FREE(frame->start);
                frame->size = 0;
                return WG_CAM_FAILURE;
        }
    }

    return WG_CAM_SUCCESS;
}

/**
 * @brief Allocate buffer for a frame
 *
 * @param cam    webcam instance
 * @param frame  memory for frame buffer
 *
 * @retval WG_CAM_SUCCESS
 * @retval WG_CAM_FAILURE
 */
wg_cam_status
wg_cam_frame_buffer_alloc(Wg_camera *cam, Wg_frame *frame)
{
    wg_uchar *buf = NULL;
    wg_ssize size = 0;
    CHECK_FOR_NULL_PARAM(frame);

    wg_cam_frame_get_size(cam, &size);

    buf = WG_CALLOC(size, sizeof (wg_uchar));
    if (NULL == buf){
        return WG_CAM_FAILURE;
    }

    frame->start = buf;
    frame->size  = size;

    return WG_CAM_SUCCESS;
}

/**
 * @brief Wait till given cameras have data to read
 *
 * This function takes an array of cameras and waits until data is available to
 * read. Cameras which can be read are returned in retval array. WG_TRUE means
 * that the camera can be read.
 *
 * @param num         number of cameras
 * @param cameras[]   cameras
 * @param retval[]    memory to store returned events
 * @param timeout_ms  timeout in ms if < 0 infinity
 *
 * @return 
 */
wg_cam_status
wg_cam_frame_select(wg_uint num, Wg_camera *cameras[], wg_boolean retval[],
        wg_int timeout_ms)
{
    struct pollfd *fds = NULL;
    wg_int i = 0;
    int status = -1;
    wg_cam_status cam_status = WG_CAM_FAILURE;

    fds = alloca(num * sizeof (struct pollfd));
    CHECK_FOR_NULL(fds);

    for (i = 0; i < num; ++i){
        fds[i].fd     = cameras[i]->fd_cam;
        fds[i].events = POLL_FLAGS;
    }

    status = poll(fds, num, timeout_ms);
    switch (status){
        case -1:
            WG_LOG("%s\n", strerror(errno));
            cam_status = WG_CAM_FAILURE;
            break;
        case  0:
            cam_status = WG_CAM_TIMEOUT;
            break;
        default:
            for (i = 0; i < num; ++i){
                retval[i] = ((fds[i].revents & POLL_FLAGS) != 0) ?
                    WG_TRUE : WG_FALSE;
            }
            cam_status = WG_CAM_SUCCESS;
    }

    return cam_status;
}

/**
 * @brief Release buffer
 *
 * @param frame frame to release
 *
 * @retval WG_CAM_SUCCESS
 * @retval WG_CAM_FAILURE
 */
wg_cam_status
wg_cam_frame_buffer_release(Wg_frame *frame)
{
    CHECK_FOR_NULL_PARAM(frame);

    WG_FREE(frame->start);

    memset(frame, '\0', sizeof (Wg_frame));

    return WG_CAM_SUCCESS;
}

/**
 * @brief Get number of bytes in frame
 *
 * @param cam   webcam instance
 * @param frame_size memmory for size
 *
 * @return 
 */
wg_cam_status
wg_cam_frame_get_size(Wg_camera *cam, wg_ssize *frame_size)
{
    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(size);

    *frame_size = cam->fmt[WG_CAM_FMT_CAPTURE].fmt.pix.sizeimage;

    return WG_CAM_SUCCESS;
}

/*! @} */
