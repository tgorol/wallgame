#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_linked_list.h>
#include "include/wg_cam.h"
#include "include/wg_cam_cap.h"
#include "include/wg_cam_image.h"

WG_PRIVATE const wg_char* yes_no(wg_boolean value);

wg_cam_status
wg_cam_cap_read(Wg_camera *cam)
{
    int status = -1;
    wg_cam_status cam_status = WG_CAM_SUCCESS;

    CHECK_FOR_NULL_PARAM(cam);

    CHECK_FOR_COND(cam->fd_cam != -1);

    memset(&cam->cap, '\0', sizeof (struct v4l2_capability));

    status = ioctl(cam->fd_cam, VIDIOC_QUERYCAP, &cam->cap);
    if (-1 == status){
        WG_LOG("%s", strerror(errno));
        if (EINVAL == errno){
            cam_status = WG_CAM_INVAL;
        }
    }

    return cam_status;
}

wg_cam_status
wg_cam_cap_print(Wg_camera *cam)
{
    struct v4l2_capability *cap = NULL;

    CHECK_FOR_NULL_PARAM(cam);

    cap = &cam->cap;

    WG_PRINT("driver               : %s\n"
             "dev name             : %s\n"
             "bus                  : %s\n",
           cap->driver, cap->card, cap->bus_info);
    WG_PRINT("version              : %u.%u.%u\n",
            (cap->version >> 16) & 0xFF,
            (cap->version >> 8) & 0xFF,
             cap->version & 0xFF);

    WG_PRINT("video capture        : %s\n", 
            yes_no(wg_cam_cap_video_capture(cam)));
    WG_PRINT("video output         : %s\n", 
            yes_no(wg_cam_cap_video_output(cam)));
    WG_PRINT("video overlay        : %s\n", 
            yes_no(wg_cam_cap_video_overlay(cam)));
    WG_PRINT("vbi capture          : %s\n", 
            yes_no(wg_cam_cap_vbi_capture(cam)));
    WG_PRINT("vbi output           : %s\n", yes_no(wg_cam_cap_vbi_output(cam)));
    WG_PRINT("sliced vbi capture   : %s\n",
            yes_no(wg_cam_cap_sliced_vbi_capture(cam)));
    WG_PRINT("sliced vbi output    : %s\n",
            yes_no(wg_cam_cap_sliced_vbi_output(cam)));
    WG_PRINT("rds capture          : %s\n", 
            yes_no(wg_cam_cap_rds_capture(cam)));
    WG_PRINT("video output overlay : %s\n", 
            yes_no(wg_cam_cap_video_output_overlay(cam)));
    WG_PRINT("tuner                : %s\n", yes_no(wg_cam_cap_tuner(cam)));
    WG_PRINT("audio                : %s\n", yes_no(wg_cam_cap_audio(cam)));
    WG_PRINT("radio                : %s\n", yes_no(wg_cam_cap_radio(cam)));

    WG_PRINT("read/write           : %s\n", yes_no(wg_cam_cap_readwrite(cam)));
    WG_PRINT("async io             : %s\n", yes_no(wg_cam_cap_asyncio(cam)));
    WG_PRINT("streaming            : %s\n", yes_no(wg_cam_cap_streaming(cam)));

    return WG_CAM_SUCCESS;
}

wg_boolean
wg_cam_cap_video_capture(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_video_output(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_video_overlay(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_VIDEO_OVERLAY) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_vbi_capture(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_VBI_CAPTURE) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_vbi_output(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_VBI_OUTPUT) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_sliced_vbi_capture(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_SLICED_VBI_CAPTURE) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_sliced_vbi_output(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_SLICED_VBI_OUTPUT) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_rds_capture(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_RDS_CAPTURE) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_video_output_overlay(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_VIDEO_OUTPUT_OVERLAY) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_tuner(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_TUNER) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_audio(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_AUDIO) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_radio(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_RADIO) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_readwrite(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_READWRITE) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_asyncio(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_ASYNCIO) ? 
        WG_TRUE : WG_FALSE;
}

wg_boolean
wg_cam_cap_streaming(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    return (cam->cap.capabilities & V4L2_CAP_STREAMING) ? 
        WG_TRUE : WG_FALSE;
}

WG_PRIVATE const wg_char*
yes_no(wg_boolean value)
{
    return value == WG_TRUE ? "yes" : "no"; 
}
