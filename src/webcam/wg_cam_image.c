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

#include "include/wg_cam.h"
#include "include/wg_cam_cap.h"
#include "include/wg_cam_image.h"

WG_PRIVATE const wg_char * cam_image_format_pixel_fmt(__u32 pixel_format);

WG_PRIVATE const wg_char* yes_no(wg_uint value);

wg_cam_status
wg_cam_image_format_get(Wg_camera *cam, WG_CAM_OUT_TYPE type, 
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
                return WG_CAM_BUSY;
                break;
            case EINVAL:
                return WG_CAM_INVAL;
                break;
        }
    }

    return WG_CAM_SUCCESS;
}

wg_cam_status
wg_cam_image_format_set(Wg_camera *cam, struct v4l2_format *format)
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
                return WG_CAM_BUSY;
                break;
            case EINVAL:
                return WG_CAM_INVAL;
                break;
        }
    }

    return WG_CAM_SUCCESS;
}

wg_cam_status
wg_cam_image_fmtdesc_list(Wg_camera *cam, WG_CAM_OUT_TYPE type, List_head *head)
{
    struct v4l2_fmtdesc tmp_desc;
    int status = -1;
    Wg_cam_fmtdesc *fmt = NULL;

    CHECK_FOR_NULL_PARAM(cam);
    CHECK_FOR_NULL_PARAM(head);
    CHECK_FOR_COND(cam->fd_cam != -1);

    tmp_desc.index = 0;
    list_init(head);

    for (;;){
        tmp_desc.type = type;
        status = ioctl(cam->fd_cam, VIDIOC_ENUM_FMT, &tmp_desc);
        if (-1 == status){
            break;
        }
   
        fmt = WG_MALLOC(sizeof (Wg_cam_fmtdesc));
        if (NULL == fmt){
            wg_cam_image_fmtdesc_list_cleanup(head);
            return WG_CAM_FAILURE;
        }

        fmt->desc = tmp_desc;
        list_init(&fmt->list);

        list_add(head, &fmt->list);

        ++tmp_desc.index;
    } 

    return WG_CAM_SUCCESS;
}

wg_cam_status
wg_cam_image_fmtdesc_print(Wg_cam_fmtdesc *fmt)
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
    return WG_CAM_SUCCESS;
    
}


wg_cam_status
wg_cam_image_fmtdesc_list_print(List_head *head)
{
    Iterator itr;
    Wg_cam_fmtdesc *fmt = NULL;

    iterator_list_init(&itr, head, GET_OFFSET(Wg_cam_fmtdesc, list));

    while ((fmt = iterator_list_next(&itr)) != NULL){
        wg_cam_image_fmtdesc_print(fmt);
    }

    return WG_CAM_SUCCESS;

}

wg_cam_status
wg_cam_image_fmtdesc_list_cleanup(List_head *head)
{
    Iterator itr;
    Wg_cam_fmtdesc *fmt = NULL;

    iterator_list_init(&itr, head, GET_OFFSET(Wg_cam_fmtdesc, list));

    while ((fmt = iterator_list_next(&itr)) != NULL){
        list_remove(&fmt->list);

        WG_FREE(fmt);
    }

    return WG_CAM_SUCCESS;

}

wg_cam_status
wg_cam_image_format_print(struct v4l2_format *format)
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
            return WG_CAM_FAILURE;
    }

    return WG_CAM_SUCCESS;
}

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
