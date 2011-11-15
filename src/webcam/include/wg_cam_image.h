#ifndef _WG_CAM_IMAGE_H
#define _WG_CAM_IMAGE_H


typedef enum WG_CAM_OUT_TYPE {
    WG_CAM_OUT_VIDEO_CAPTURE        = V4L2_BUF_TYPE_VIDEO_CAPTURE        ,
    WG_CAM_OUT_VIDEO_OUTPUT         = V4L2_BUF_TYPE_VIDEO_OUTPUT         ,
    WG_CAM_OUT_VIDEO_OVERLAY        = V4L2_BUF_TYPE_VIDEO_OVERLAY        ,
    WG_CAM_OUT_VBI_CAPTURE          = V4L2_BUF_TYPE_VBI_CAPTURE          ,
    WG_CAM_OUT_VBI_OUTPUT           = V4L2_BUF_TYPE_VBI_OUTPUT           ,
    WG_CAM_OUT_SLICED_VBI_CAPTURE   = V4L2_BUF_TYPE_SLICED_VBI_CAPTURE   ,
    WG_CAM_OUT_SLICED_VBI_OUTPUT    = V4L2_BUF_TYPE_SLICED_VBI_OUTPUT    ,
    WG_CAM_OUT_VIDEO_OUTPUT_OVERLAY = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY
}WG_CAM_OUT_TYPE;

typedef struct Wg_cam_fmtdesc{
    struct v4l2_fmtdesc desc;
    List_head list;
}Wg_cam_fmtdesc;

WG_PUBLIC wg_cam_status wg_cam_image_format_print(struct v4l2_format *format);

WG_PUBLIC wg_cam_status wg_cam_image_format_get(Wg_camera *cam, 
        WG_CAM_OUT_TYPE type, struct v4l2_format *format);

WG_PUBLIC wg_cam_status
wg_cam_image_format_set(Wg_camera *cam, struct v4l2_format *format);

WG_PUBLIC wg_cam_status
wg_cam_image_fmtdesc_list(Wg_camera *cam, WG_CAM_OUT_TYPE type, 
        List_head *head);

WG_PUBLIC wg_cam_status
wg_cam_image_fmtdesc_list_print(List_head *head);

WG_PUBLIC wg_cam_status
wg_cam_image_fmtdesc_print(Wg_cam_fmtdesc *desc);

WG_PUBLIC wg_cam_status
wg_cam_image_fmtdesc_list_cleanup(List_head *head);

#endif
