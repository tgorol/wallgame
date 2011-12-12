#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_linked_list.h>


#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/cam_img_yuyv.h"
#include "include/cam_img_jpeg.h"
#include "include/cam_format_selector.h"
#include "include/cam_output.h"


/**
 * @brief Format decompressor
 */
typedef struct Fmt_decomp{
    __u32 pixelformat;                /*!< Format supported by a decompressor */
    Wg_cam_decompressor decompressor; /*!< Decompressor function*/
}Fmt_decomp;

typedef wg_char u32str[sizeof (__u32) + 1]; 

WG_STATIC Fmt_decomp supported_formats[] = {
    {v4l2_fourcc('Y', 'U', 'Y', 'V'), {cam_img_yuyv_2_rgb24}},
    {v4l2_fourcc('M', 'J', 'P', 'G'), {cam_img_jpeg_decompress}},
    {v4l2_fourcc('J', 'P', 'E', 'G'), {cam_img_jpeg_decompress}}
};

WG_PRIVATE void
cam_print_u32_as_string(__u32 value, u32str cstr);

wg_status
cam_select_user_decompressor(Wg_camera *cam, __u32 pixelformat)
{
    cam_status status = CAM_FAILURE;
    Wg_cam_decompressor  decomp;
    struct v4l2_format format;
    u32str   pixelfmt_str;

    cam_print_u32_as_string(pixelformat, pixelfmt_str);

    status = cam_get_decompressor(pixelformat, &decomp);
    if (CAM_SUCCESS == status){
        format = cam->fmt[CAM_FMT_CAPTURE];
        format.fmt.pix.pixelformat = pixelformat;
        status = cam_output_format_set(cam, &format);
        if (CAM_SUCCESS == status){
            WG_LOG("Selected format is %s\n", pixelfmt_str);
            cam->fmt[CAM_FMT_CAPTURE] = format;
            cam->dcomp = decomp;
        }else{
            WG_LOG("Error during setting %s format\n", pixelfmt_str);
        }
    }else{
        WG_LOG("Format %s not supported\n", pixelfmt_str);
    }

    return status;
}

wg_status
cam_select_decompressor(Wg_camera *cam)
{
    wg_int index = 0;
    struct v4l2_format format;
    cam_status status = CAM_FAILURE;
    u32str   pixelfmt_str;

    for (index = 0; index < ELEMNUM(supported_formats); ++ index){
        format = cam->fmt[CAM_FMT_CAPTURE];

        format.fmt.pix.pixelformat = supported_formats[index].pixelformat;
  
        status = cam_output_format_set(cam, &format);
        switch (status){
            case CAM_BUSY:
                return status;
                break;
            case CAM_INVAL:
                /* means not supported format, try the next one */
                break;
            case CAM_SUCCESS:
                cam_print_u32_as_string(supported_formats[index].pixelformat,
                         pixelfmt_str);
                WG_LOG("Selected format is %s\n", pixelfmt_str);

                cam->fmt[CAM_FMT_CAPTURE] = format;
                cam->dcomp = supported_formats[index].decompressor;
                return CAM_SUCCESS;
                break;
            default:
                WG_ERROR("BUG: Should not happen\n");
                return CAM_FAILURE;
        }
    }

    return CAM_FAILURE;
}


cam_status
cam_get_decompressor(__u32 pixelformat, Wg_cam_decompressor *decomp)
{
    wg_int index = 0;
    cam_status status = CAM_FAILURE;

    for (index = 0; index < ELEMNUM(supported_formats) && 
                    pixelformat != supported_formats[index].pixelformat;
                    ++index)
        /* void loop */   ;
    
    if (ELEMNUM(supported_formats) > index){
        *decomp = supported_formats[index].decompressor;
        status  = CAM_SUCCESS;
    }

    return status;
}

wg_boolean
cam_is_format_supported(__u32 pixelformat)
{
    Wg_cam_decompressor dcomp;

    return cam_get_decompressor(pixelformat, &dcomp) == CAM_SUCCESS ?
        WG_TRUE : WG_FALSE;
}

WG_PRIVATE void
cam_print_u32_as_string(__u32 value, u32str cstr)
{
    wg_int i = 0;
    wg_char *c = (char*)&value;

    for (i = 0; i < sizeof (value); ++i){
        *cstr = c[i];
        ++cstr;
    }
    *cstr = '\0';

    return;
}
