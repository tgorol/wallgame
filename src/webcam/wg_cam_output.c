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
#include <wg_iterator.h>

#include "include/cam.h"
#include "include/cam_cap.h"
#include "include/cam_output.h"

/*! @defgroup webcam_format Webcam Capture Output Format
 *  @ingroup webcam 
 */

/*! @{ */

WG_PRIVATE const wg_char * cam_image_format_pixel_fmt(__u32 pixel_format);

WG_PRIVATE const wg_char* yes_no(wg_uint value);

/**
 * @brief Get webcam format
 *
 * @param cam     webcam instance
 * @param type    format type
 * @param format  memmory to store format data
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 * @retval CAM_BUSY
 * @retval CAM_INVAL
 */
cam_status
cam_output_format_get(Wg_camera *cam, CAM_OUT_TYPE type, 
        struct v4l2_format *format)
{
    int status = -1;

    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(format);

    CHECK_FOR_COND(cam->fd_cam != -1);

    memset(format, '\0', sizeof (struct v4l2_format));

    format->type = type;

    status = ioctl(cam->fd_cam, VIDIOC_G_FMT, format);
    if (-1 == status){
        WG_LOG("%s", strerror(errno));
        switch (errno){
            case EBUSY:
                return CAM_BUSY;
                break;
            case EINVAL:
                return CAM_INVAL;
                break;
        }
    }

    return CAM_SUCCESS;
}

/**
 * @brief Set webcam format
 *
 * @param cam     webcam instance
 * @param format  format to set
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 * @retval CAM_BUSY
 * @retval CAM_INVAL
 */
cam_status
cam_output_format_set(Wg_camera *cam, struct v4l2_format *format)
{
    int status = -1;

    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(format);

    CHECK_FOR_COND(cam->fd_cam != -1);

    status = ioctl(cam->fd_cam, VIDIOC_S_FMT, format);
    if (-1 == status){
        WG_LOG("%s", strerror(errno));
        switch (errno){
            case EBUSY:
                return CAM_BUSY;
                break;
            case EINVAL:
                return CAM_INVAL;
                break;
        }
    }

    return CAM_SUCCESS;
}

/**
 * @brief Get list of supported formats
 *
 * Type of the element in the list os Wg_cam_format_description
 *
 * @param cam    webcam instance
 * @param type   format type
 * @param head   head of the list to store data
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_output_format_description_list(Wg_camera *cam, CAM_OUT_TYPE type, List_head *head)
{
    struct v4l2_fmtdesc tmp_desc;
    int status = -1;
    Wg_cam_format_description *fmt = NULL;

    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(head);
    CHECK_FOR_COND(cam->fd_cam != -1);

    tmp_desc.index = 0;
    list_init(head);

    memset(&tmp_desc, '\0', sizeof (struct v4l2_fmtdesc));

    for (;;){
        tmp_desc.type = type;
        status = ioctl(cam->fd_cam, VIDIOC_ENUM_FMT, &tmp_desc);
        if (-1 == status){
            break;
        }
   
        fmt = WG_MALLOC(sizeof (Wg_cam_format_description));
        if (NULL == fmt){
            cam_output_format_description_list_cleanup(head);
            return CAM_FAILURE;
        }

        fmt->desc = tmp_desc;
        list_init(&fmt->list);

        list_add(head, &fmt->list);

        ++tmp_desc.index;
    } 

    return CAM_SUCCESS;
}

/**
 * @brief Print webcam format description
 *
 * @param fmt  format to pint
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_output_format_description_print(Wg_cam_format_description *fmt)
{
    CHECK_FOR_NULL_PARAM(desc);


    WG_PRINT("index        : %u\n"
             "description  : %s\n"
             "pixel format : %s\n"
             "compressed   : %s\n",
             fmt->desc.index,
             fmt->desc.description, 
             cam_image_format_pixel_fmt(fmt->desc.pixelformat),
             yes_no(fmt->desc.flags & V4L2_FMT_FLAG_COMPRESSED)
            );
    return CAM_SUCCESS;
}


/**
 * @brief Print format list
 *
 * @param head head of the list to print
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_output_format_description_list_print(List_head *head)
{
    Iterator itr;
    Wg_cam_format_description *fmt = NULL;

    iterator_list_init(&itr, head, GET_OFFSET(Wg_cam_format_description, list));

    while ((fmt = iterator_list_next(&itr)) != NULL){
        cam_output_format_description_print(fmt);
    }

    return CAM_SUCCESS;
}

/**
 * @brief Cleanup format list
 *
 * @param head list of formats
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_output_format_description_list_cleanup(List_head *head)
{
    Iterator itr;
    Wg_cam_format_description *fmt = NULL;

    iterator_list_init(&itr, head, GET_OFFSET(Wg_cam_format_description, list));

    while ((fmt = iterator_list_next(&itr)) != NULL){
        list_remove(&fmt->list);

        WG_FREE(fmt);
    }

    return CAM_SUCCESS;

}

/**
 * @brief Print format information 
 *
 * @param format format to print
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_output_format_print(struct v4l2_format *format)
{
    switch (format->type){
        case V4L2_BUF_TYPE_VIDEO_CAPTURE:
            WG_PRINT("width          : %u\n", format->fmt.pix.width);
            WG_PRINT("height         : %u\n", format->fmt.pix.height);
            WG_PRINT("bytes per line : %u\n", format->fmt.pix.bytesperline);
            WG_PRINT("size image     : %u\n", format->fmt.pix.sizeimage);
            WG_PRINT("pixel format   : %s(%0x)\n",
                    cam_image_format_pixel_fmt(format->fmt.pix.pixelformat),
                    format->fmt.pix.pixelformat
                    );
            break;
        default:
            WG_LOG("Unsupported output format\n");
            return CAM_FAILURE;
    }

    return CAM_SUCCESS;
}

/**
 * @brief Convert pixel format to string
 *
 * @param pixel_format pixel format
 *
 * @return pixel format  ASCIZ representation
 */
WG_PRIVATE const wg_char *
cam_image_format_pixel_fmt(__u32 pixel_format)
{
    const wg_char *string = (wg_char*)&pixel_format;
    /** @todo fix: make it reentrant (get rid of static) */
    static wg_char ret_string[sizeof (__u32) + 1] = {'\0'}; 
    wg_int i = 0;

    for (i = 0; i < sizeof (__u32); ++i){
        ret_string[i] = string[i];
    }

    ret_string[i] = '\0';

    return ret_string;
}

WG_PRIVATE const wg_char*
yes_no(wg_uint value)
{
    return value ? "yes" : "no"; 
}

/*! @} */
