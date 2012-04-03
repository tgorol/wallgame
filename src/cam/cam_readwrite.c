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

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_linked_list.h>
#include <wg_iterator.h>
#include <img.h>
#include <cam.h>

#include "include/cam_frame.h"
#include "include/cam_cap.h"
#include "include/cam_readwrite.h"

/*! @defgroup webcam_readwrite read/write mode
 * @ingroup webcam 
 */

/*! @{ */

#define POLL_FLAGS (POLLIN | POLLPRI)

WG_PRIVATE cam_status
read_frame(Wg_camera *cam, Wg_frame *frame);

WG_PRIVATE cam_status
alloc_frame_buffer(Wg_camera *cam, Wg_frame *frame);

WG_PRIVATE cam_status
release_frame_buffer(Wg_frame *frame);

WG_PRIVATE cam_status
get_frame_size(Wg_camera *cam, wg_ssize *frame_size);

WG_PRIVATE cam_status
close_camera(Wg_camera *cam);

WG_PRIVATE cam_status
open_camera(Wg_camera *cam);

WG_PRIVATE cam_status
start_camera(Wg_camera *cam);

WG_PRIVATE cam_status
stop_camera(Wg_camera *cam);

WG_PRIVATE cam_status
cleanup_frame(Wg_camera *cam, Wg_frame *frame);

WG_PRIVATE cam_status
discard_frame(Wg_camera *cam, Wg_frame *frame);

/**
 * @brief Switch camera into read/write mode.
 *
 * @param cam webcam instance
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_readwrite_init(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    cam->cam_ops.read          = read_frame;
    cam->cam_ops.cleanup_frame = cleanup_frame;
    cam->cam_ops.empty_frame   = discard_frame;
    cam->cam_ops.close         = close_camera;
    cam->cam_ops.open          = open_camera;
    cam->cam_ops.start         = start_camera;
    cam->cam_ops.stop          = stop_camera;

    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
discard_frame(Wg_camera *cam, Wg_frame *frame)
{
    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(frame);

    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
cleanup_frame(Wg_camera *cam, Wg_frame *frame)
{
    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(frame);

    return release_frame_buffer(frame);
}

WG_PRIVATE cam_status
stop_camera(Wg_camera *cam)
{
    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
start_camera(Wg_camera *cam)
{
    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
close_camera(Wg_camera *cam)
{
    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
open_camera(Wg_camera *cam)
{
    return CAM_SUCCESS;
}

/**
 * @brief Read a frame from webcam
 *
 * When frame->start is NULL buffer will be allocated.
 * Frame buffer must be released by cam_frame_buffer_release()
 *
 * @param cam       webcam instance
 * @param buffer    memory for buffer buffer 
 * @param buf_size  size of the buffer
 *
 * @return 
 */
WG_PRIVATE cam_status
read_frame(Wg_camera *cam, Wg_frame *frame)
{
    ssize_t len = 0; 
    wg_ssize size = 0;

    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(frame);

    if ((cam_cap_readwrite(cam) && cam_cap_video_capture(cam)) 
            != WG_TRUE){
        return CAM_FAILURE;
    }

    get_frame_size(cam, &size);

    if (frame->start == NULL){
        if (alloc_frame_buffer(cam, frame) != CAM_SUCCESS){
            return CAM_FAILURE;
        }
    }

    CHECK_FOR_COND(frame->size >= size);

    len = read(cam->fd_cam, frame->start, frame->size);
    if (-1 == len){
        switch (errno){
            case EIO:
                return CAM_IO;
                break;
            default:
                WG_FREE(frame->start);
                frame->size = 0;
                return CAM_FAILURE;
        }
    }

    return CAM_SUCCESS;
}

/**
 * @brief Allocate buffer for a frame
 *
 * @param cam    webcam instance
 * @param frame  memory for frame buffer
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
WG_PRIVATE cam_status
alloc_frame_buffer(Wg_camera *cam, Wg_frame *frame)
{
    wg_uchar *buf = NULL;
    wg_ssize size = 0;
    CHECK_FOR_NULL_PARAM(frame);

    get_frame_size(cam, &size);

    buf = WG_CALLOC(size, sizeof (wg_uchar));
    if (NULL == buf){
        return CAM_FAILURE;
    }

    frame->start = buf;
    frame->size  = size;

    return CAM_SUCCESS;
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
cam_status
select_frame(wg_uint num, Wg_camera *cameras[], wg_boolean retval[],
        wg_int timeout_ms)
{
    struct pollfd *fds = NULL;
    wg_int i = 0;
    int status = -1;
    cam_status cam_status = CAM_FAILURE;

    fds = WG_ALLOCA(num * sizeof (struct pollfd));
    CHECK_FOR_NULL(fds);

    for (i = 0; i < num; ++i){
        fds[i].fd     = cameras[i]->fd_cam;
        fds[i].events = POLL_FLAGS;
    }

    status = poll(fds, num, timeout_ms);
    switch (status){
        case -1:
            WG_LOG("%s\n", strerror(errno));
            cam_status = CAM_FAILURE;
            break;
        case  0:
            cam_status = CAM_TIMEOUT;
            break;
        default:
            for (i = 0; i < num; ++i){
                retval[i] = ((fds[i].revents & POLL_FLAGS) != 0) ?
                    WG_TRUE : WG_FALSE;
            }
            cam_status = CAM_SUCCESS;
    }

    return cam_status;
}

/**
 * @brief Release buffer
 *
 * @param frame frame to release
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
WG_PRIVATE cam_status
release_frame_buffer(Wg_frame *frame)
{
    CHECK_FOR_NULL_PARAM(frame);

    WG_FREE(frame->start);

    memset(frame, '\0', sizeof (Wg_frame));

    return CAM_SUCCESS;
}

/**
 * @brief Get number of bytes in frame
 *
 * @param cam   webcam instance
 * @param frame_size memmory for size
 *
 * @return 
 */
WG_PRIVATE cam_status
get_frame_size(Wg_camera *cam, wg_ssize *frame_size)
{
    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(size);

    *frame_size = cam->fmt[CAM_FMT_CAPTURE].fmt.pix.sizeimage;

    return CAM_SUCCESS;
}


/*! @} */
