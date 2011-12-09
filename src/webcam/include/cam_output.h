#ifndef _cam_output_H
#define _cam_output_H


typedef enum CAM_OUT_TYPE {
    CAM_OUT_VIDEO_CAPTURE        = V4L2_BUF_TYPE_VIDEO_CAPTURE        ,
    CAM_OUT_VIDEO_OUTPUT         = V4L2_BUF_TYPE_VIDEO_OUTPUT         ,
    CAM_OUT_VIDEO_OVERLAY        = V4L2_BUF_TYPE_VIDEO_OVERLAY        ,
    CAM_OUT_VBI_CAPTURE          = V4L2_BUF_TYPE_VBI_CAPTURE          ,
    CAM_OUT_VBI_OUTPUT           = V4L2_BUF_TYPE_VBI_OUTPUT           ,
    CAM_OUT_SLICED_VBI_CAPTURE   = V4L2_BUF_TYPE_SLICED_VBI_CAPTURE   ,
    CAM_OUT_SLICED_VBI_OUTPUT    = V4L2_BUF_TYPE_SLICED_VBI_OUTPUT    ,
    CAM_OUT_VIDEO_OUTPUT_OVERLAY = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY
}CAM_OUT_TYPE;

/**
 * @brief Camera Format Descripton 
 */
typedef struct Wg_cam_format_description{
    struct v4l2_fmtdesc desc;    /*!< Description */
    List_head list;              /*!< List head   */
}Wg_cam_format_description;

WG_PUBLIC cam_status cam_output_format_print(struct v4l2_format *format);

WG_PUBLIC cam_status cam_output_format_get(Wg_camera *cam, 
        CAM_OUT_TYPE type, struct v4l2_format *format);

WG_PUBLIC cam_status
cam_output_format_set(Wg_camera *cam, struct v4l2_format *format);

WG_PUBLIC cam_status
cam_output_format_description_list(Wg_camera *cam, CAM_OUT_TYPE type, 
        List_head *head);

WG_PUBLIC cam_status
cam_output_format_description_list_print(List_head *head);

WG_PUBLIC cam_status
cam_output_format_description_print(Wg_cam_format_description *desc);

WG_PUBLIC cam_status
cam_output_format_description_list_cleanup(List_head *head);

#endif
