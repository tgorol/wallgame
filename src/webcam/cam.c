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
#include <poll.h>

#include <linux/videodev2.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_linked_list.h>

#include "include/cam.h"
#include "include/cam_frame.h"
#include "include/cam_cap.h"
#include "include/cam_output.h"
#include "include/cam_readwrite.h"
#include "include/cam_streaming.h"
#include "include/cam_format_selector.h"

/*! \defgroup webcam Webcam
 */

/*! @{ */

/*! \brief Return WG_TRUE if selected mode is STREAMING. Otherwise WH_FALSE 
 */
#define MODE_STREAM(mode)                                                      \
    ((mode == CAM_MODE_STREAMING) || (mode == CAM_MODE_INVALID))

/*! \brief Return WG_TRUE if selected mode is READWRITE mode. Otherwisw
 *  WG_FALSE
 */
#define MODE_READWRITE(mode)                                                   \
    ((mode == CAM_MODE_READWRITE) || (mode == CAM_MODE_INVALID))


WG_PRIVATE cam_status
select_mode(Wg_camera *cam, CAM_MODE mode);

WG_PRIVATE CAM_MODE
get_fallback_mode(CAM_MODE mode);

/**
 * @brief Initialize a webcam
 *
 * @param cam      memory to store webcam instance
 * @param dev_path webcam device name
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_init(Wg_camera *cam, wg_char* dev_path)
{
    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(dev_path);

    memset(cam, '\0', sizeof (Wg_camera));

    cam->mode = CAM_MODE_INVALID;

    strncpy(cam->dev_path, dev_path, DEV_PATH_MAX);

    cam->fd_cam = -1;

    return CAM_SUCCESS;
}

/**
 * @brief Open a webcam
 *
 * If mode == 0 a driver selects the most efficient mode.
 *
 * @param cam webcam instance
 * @param mode mode to open camera in.
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_open(Wg_camera *cam, CAM_MODE mode)
{
    cam_status status = CAM_FAILURE;
    struct stat st; 

    CHECK_FOR_NULL_PARAM(cam);


    if (-1 == stat (cam->dev_path, &st)) {
        WG_LOG("Cannot identify '%s': %d, %s\n",
                cam->dev_path, errno, strerror (errno));
        return CAM_FAILURE;
    }

    if (!S_ISCHR (st.st_mode)) {
        WG_LOG("%s is no device\n", cam->dev_path);
        return CAM_FAILURE;
    }
    do {
        cam->fd_cam = open(cam->dev_path, O_RDWR, 0);
        if (-1 == cam->fd_cam){
            WG_ERROR("%s", strerror(errno));
            return CAM_FAILURE;
        }

        status = cam_cap_get(cam);
        if (CAM_FAILURE == status){
            close(cam->fd_cam);
            cam->fd_cam = -1;
            return CAM_FAILURE;
        }

        status = cam_output_format_get(cam, CAM_OUT_VIDEO_CAPTURE, 
                &cam->fmt[CAM_FMT_CAPTURE]);
        if (CAM_FAILURE == status){
            close(cam->fd_cam);
            cam->fd_cam = -1;
            return CAM_FAILURE;
        }

        switch (mode){
            case CAM_MODE_INVALID:
            case CAM_MODE_READWRITE:
                select_mode(cam, mode);
                break;
            default:
                break;
        }
        status = cam->cam_ops.open(cam); 
        if (CAM_SUCCESS != status){
            close(cam->fd_cam);
            cam->fd_cam = -1;
            mode = get_fallback_mode(mode);
        }
    } while(status != CAM_SUCCESS);

    status = cam_select_decompressor(cam);
    if (CAM_SUCCESS != status){
        close(cam->fd_cam);
        cam->fd_cam = -1;
    }

    return status;
}



/**
 * @brief Close a webcam
 *
 * @param cam  webcam instance
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_close(Wg_camera *cam)
{
    CHECK_FOR_NULL_PARAM(cam);

    CHECK_FOR_COND(cam->fd_cam != -1);

    close(cam->fd_cam);

    return CAM_SUCCESS;
}

/**
 * @brief Start capturing data.
 *
 * @param cam webcam instance
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_start(Wg_camera *cam)
{
    cam_status status = CAM_FAILURE;

    CHECK_FOR_NULL_PARAM(cam);

    if (NULL != cam->cam_ops.start){
        status = cam->cam_ops.start(cam);
    }

    return status;
}

/**
 * @brief Stop capturing data
 *
 * @param cam webcam instance
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_stop(Wg_camera *cam)
{
    cam_status status = CAM_FAILURE;

    CHECK_FOR_NULL_PARAM(cam);

    if (NULL != cam->cam_ops.stop){
        status = cam->cam_ops.stop(cam);
    }

    return status;
}

/**
 * @brief Read a frame from camera
 *
 * @param cam   webcam instance
 * @param frame memory to store readed frame
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_read(Wg_camera *cam, Wg_frame *frame)
{
    struct pollfd fds;
    cam_status status = CAM_FAILURE;
    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(frame);

    switch (frame->state){
        case WG_FRAME_EMPTY:
        case WG_FRAME_INVALID:
            if (NULL != cam->cam_ops.read){
                fds.fd     = cam->fd_cam;
                fds.events = POLLIN | POLLPRI;

                poll(&fds, 1, -1);
                status = cam->cam_ops.read(cam, frame);
                if (CAM_SUCCESS == status){
                    frame->state = WG_FRAME_FULL;
                }
            }
        default:
            break;
    }

    return status;
}

/**
 * @brief Mark buffer as EMPTY.
 *
 * After this call the frame can be reused. This function
 * doesnt release any resources assosiated with a frame. Should be called
 * as soon as possible after frame is not needed.
 *
 * @param cam    camera instance
 * @param frame  frame
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_discard_frame(Wg_camera *cam, Wg_frame *frame)
{
    cam_status status = CAM_FAILURE;

    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(frame);

    switch (frame->state){
        case WG_FRAME_FULL:
            if (NULL != cam->cam_ops.empty_frame){
                status = cam->cam_ops.empty_frame(cam, frame);
                if (CAM_SUCCESS == status){
                    frame->state = WG_FRAME_EMPTY;
                }
            }
        case WG_FRAME_EMPTY:
            status = CAM_SUCCESS;
            break;
        default:
            break;
    }

    return status;
}

/**
 * @brief Release resources associated with a frame
 *
 * @param cam    camera instance frame was allocated by
 * @param frame  frame to free
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_free_frame(Wg_camera *cam, Wg_frame *frame)
{
    cam_status status = CAM_FAILURE;
    CHECK_FOR_NULL_PARAM(frame);
    CHECK_FOR_NULL_PARAM(cam);

    switch (frame->state){
        case WG_FRAME_EMPTY:
        case WG_FRAME_FULL:
            cam_discard_frame(cam, frame);
            break;
        default:
            return CAM_FAILURE;
            break;
    }

    if (NULL != cam->cam_ops.cleanup_frame){
        status = cam->cam_ops.cleanup_frame(cam, frame);
        if (CAM_SUCCESS ==status){
            memset(frame,'\0', sizeof (Wg_frame));
            frame->state = WG_FRAME_INVALID;
        }
    }

    return status;
}

/**
 * @brief Return a decompressor to RGB24.
 *
 * Returned finction can be used to decompress a frame readed from the camera
 * into RGB24. This value depends on the output format selected during
 * configuration. This function should be called aters cam_start()
 *
 * @param cam  webcam instance
 * @param dcomp memory to store a decompressor function
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_decompressor(Wg_camera *cam, Wg_cam_decompressor *dcomp)
{
    cam_status status = CAM_FAILURE;

    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(dcomp);

    if (NULL != cam->dcomp.run){
        *dcomp = cam->dcomp;
        status = CAM_SUCCESS;
    }

    return status;
}


WG_PRIVATE CAM_MODE
get_fallback_mode(CAM_MODE mode)
{
    static CAM_MODE fallback_mode[] = {
        [CAM_MODE_INVALID]   = CAM_MODE_READWRITE,
        [CAM_MODE_STREAMING] = CAM_MODE_READWRITE,
        [CAM_MODE_READWRITE] = CAM_MODE_UNKNOWN
    };
    CAM_MODE new_mode = CAM_MODE_UNKNOWN;

    if (CAM_MODE_UNKNOWN != mode){
        new_mode =fallback_mode[mode];
    }

    return new_mode;
}

WG_PRIVATE cam_status
select_mode(Wg_camera *cam, CAM_MODE mode)
{
    cam_status status = CAM_FAILURE;
    CHECK_FOR_NULL_PARAM(cam);

    if (cam_cap_video_capture(cam) == WG_TRUE){
        if ((cam_cap_streaming(cam) == WG_TRUE) && MODE_STREAM(mode)){
            WG_LOG("Webcam switch into STREAMING mode\n");
            cam_streaming_init(cam);
        }else if ((cam_cap_readwrite(cam) == WG_TRUE)
                && MODE_READWRITE(mode)){
            WG_WARN("Webcam switch into READ/WRITE mode\n");
            cam_readwrite_init(cam);
        }else{
            status = CAM_NO_SUPPORT;
        }
    }else{
        status = CAM_NO_SUPPORT;
    }

    return status;
}

/*! @} */
