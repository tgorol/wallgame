#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <alloca.h>
#include <sys/mman.h>

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

#include "include/cam.h"
#include "include/cam_frame.h"
#include "include/cam_cap.h"
#include "include/cam_streaming.h"

/*! @defgroup webcam_streaming Webcam Streaming Mode
 * @ingroup webcam 
 */

#define DEFAULT_BUFFER_NUM 5

#define POLL_FLAGS (POLLIN | POLLPRI)

/*! @{ */

WG_PRIVATE cam_status
cam_unmap_buffers(Wg_camera *cam);

WG_PRIVATE cam_status
cam_allocate_buffers(Wg_camera *cam, wg_uint bufnum);

WG_PRIVATE cam_status
cam_put_buffer(Wg_camera *cam, Wg_frame *frame);

WG_PRIVATE cam_status
cam_pop_buffer(Wg_camera *cam, Wg_frame *frame);

WG_PRIVATE cam_status
cam_cleanup_frame(Wg_camera *cam, Wg_frame *frame);

WG_PRIVATE cam_status
open_camera(Wg_camera *cam);

WG_PRIVATE cam_status
close_camera(Wg_camera *cam);

WG_PRIVATE cam_status
cam_start_streaming(Wg_camera *cam);

WG_PRIVATE cam_status
cam_stop_streaming(Wg_camera *cam);

WG_PRIVATE cam_status
cam_start_capturing(Wg_camera *cam);

WG_PRIVATE cam_status
cam_stop_capturing(Wg_camera *cam);

/**
 * @brief Switch camera into a streaming mode.
 *
 * @param cam webcam instance
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_streaming_init(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    cam->cam_ops.read          = cam_pop_buffer;
    cam->cam_ops.cleanup_frame = cam_cleanup_frame;
    cam->cam_ops.empty_frame   = cam_put_buffer;
    cam->cam_ops.close         = close_camera;
    cam->cam_ops.open          = open_camera;
    cam->cam_ops.start         = cam_start_capturing;
    cam->cam_ops.stop          = cam_stop_capturing;

    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
cam_cleanup_frame(Wg_camera *cam, Wg_frame *frame)
{
    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
open_camera(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    if ((cam_cap_video_capture(cam) && cam_cap_streaming(cam))
            == WG_FALSE){
        return CAM_NO_SUPPORT;
    }

    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
cam_start_capturing(Wg_camera *cam)
{
    cam_status status = CAM_FAILURE;

    CHECK_FOR_NULL_PARAM(cam);

    status = cam_allocate_buffers(cam, DEFAULT_BUFFER_NUM);
    if (CAM_SUCCESS != status){
        return CAM_FAILURE;
    }

    status = cam_start_streaming(cam);
    if (CAM_SUCCESS != status){
        cam_unmap_buffers(cam);
        return status;
    }

    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
cam_stop_capturing(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    cam_stop_streaming(cam);

    cam_unmap_buffers(cam);

    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
close_camera(Wg_camera *cam)
{
    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
cam_pop_buffer(Wg_camera *cam, Wg_frame *frame)
{
    struct v4l2_buffer buffer;
    struct v4l2_pix_format *fmt = NULL;
    Wg_cam_buf *cam_buffer = NULL;
    int status = -1;

    buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;

    status = ioctl(cam->fd_cam, VIDIOC_DQBUF, &buffer);
    if (-1 == status){
        /** @todo handle error properly */
        return CAM_FAILURE;
    }

    if (buffer.index >= cam->buf_num){
        return CAM_INVAL;
    }

    cam_buffer = &(cam->buffers[buffer.index]);

    frame->start       = cam_buffer->start;
    frame->size        = buffer.bytesused;
    frame->stream_buf  = buffer;
    frame->state       = WG_FRAME_FULL;

    fmt                = &(cam->fmt[CAM_FMT_CAPTURE].fmt.pix);
    frame->width       = fmt->width;
    frame->height      = fmt->height;
    frame->pixelformat = fmt->pixelformat;

    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
cam_put_buffer(Wg_camera *cam, Wg_frame *frame)
{
    int status = -1;
    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(frame);

    status = ioctl(cam->fd_cam, VIDIOC_QBUF, &frame->stream_buf);
    if (-1 == status){
        return CAM_FAILURE;
    }

    frame->state = WG_FRAME_EMPTY;
    frame->start = NULL;
    frame->size  = 0;

    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
cam_start_streaming(Wg_camera *cam)
{
    enum v4l2_buf_type type =  V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_buffer buffer;
    int status = -1;
    wg_int index = 0;

    for (index = 0; index < cam->buf_num; ++index){
        memset(&buffer, '\0', sizeof (struct v4l2_buffer));

        buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index  = index;

        status = ioctl(cam->fd_cam, VIDIOC_QBUF, &buffer);
        if (-1 == status){
            return CAM_FAILURE;
        }
    }

    status = ioctl(cam->fd_cam, VIDIOC_STREAMON, &type);
    if (-1 == status){
        return CAM_INVAL;
    }

    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
cam_stop_streaming(Wg_camera *cam)
{
    enum v4l2_buf_type type =  V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int status = -1;

    status = ioctl(cam->fd_cam, VIDIOC_STREAMOFF, &type);
    if (-1 == status){
        return CAM_INVAL;
    }

    return CAM_SUCCESS;
}


WG_PRIVATE cam_status
cam_allocate_buffers(Wg_camera *cam, wg_uint bufnum)
{
    struct v4l2_requestbuffers requested_buffers;
    int status = -1;
    wg_int index = 0;
    Wg_cam_buf *buffers = NULL;
    struct v4l2_buffer buffer;

    CHECK_FOR_NULL_PARAM(cam);

    memset(&requested_buffers, '\0', sizeof (struct v4l2_requestbuffers));

    requested_buffers.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requested_buffers.memory = V4L2_MEMORY_MMAP;
    requested_buffers.count  = bufnum;

    status = ioctl(cam->fd_cam, VIDIOC_REQBUFS, &requested_buffers);
    if (-1 == status){
        if (errno == EINVAL){
            return CAM_NO_SUPPORT;
        }else{
            return CAM_FAILURE;
        }
    }

    if (requested_buffers.count < 2){
        return CAM_FAILURE;
    }

    buffers = WG_CALLOC(requested_buffers.count, sizeof (Wg_cam_buf));
    if (NULL == buffers){
        return CAM_FAILURE;
    }

    cam->buffers = buffers;
    cam->buf_num = 0;

    for (index = 0; index < requested_buffers.count; ++index){
        memset(&buffer, '\0', sizeof (struct v4l2_buffer));
        buffer.type   = requested_buffers.type;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index  = index;

        status = ioctl(cam->fd_cam, VIDIOC_QUERYBUF, &buffer);
        if (-1 == status){
            WG_FREE(cam->buffers);
            return CAM_FAILURE;
        }

        cam->buffers[index].length = buffer.length;

        cam->buffers[index].start = mmap(NULL, buffer.length, 
                PROT_READ | PROT_WRITE, MAP_SHARED,
                cam->fd_cam, buffer.m.offset);

        if (MAP_FAILED == buffers[index].start){
            cam_unmap_buffers(cam);
            WG_FREE(cam->buffers);
            return CAM_FAILURE;
        }

        ++cam->buf_num;
    }

    return CAM_SUCCESS;
}


WG_PRIVATE cam_status
cam_unmap_buffers(Wg_camera *cam)
{
    wg_int index = 0;
    Wg_cam_buf *buffer = NULL;

    CHECK_FOR_NULL_PARAM(cam);

    for (index = 0; index < cam->buf_num; ++index){
        buffer = &(cam->buffers[index]);
        if (NULL != buffer){
            munmap(buffer->start, buffer->length);
            memset(buffer, '\0', sizeof (Wg_cam_buf));
        }
    }

    WG_FREE(cam->buffers);

    return CAM_SUCCESS;
}

/*! @} */
