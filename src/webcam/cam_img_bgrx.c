#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/cam_img.h"
#include "include/cam_img_bgrx.h"
#include "include/cam_img_rgb.h"

#define BGRX_COMPONENT_NUM    4

/*! @defgroup image_bgrx BGRX manipulation
 * @ingroup image
 * @{ 
 */


/**
 * @brief Convert RGB24 to BGRX format
 *
 * @param rgb_img     RGB24 image
 * @param bgrx_img    Memory to store BGRX image
 *
 * @retval CAM_SUCCESS
 * @retval CAM_SUCCESS
 */
cam_status
cam_img_rgb_2_bgrx(Wg_image *rgb_img, Wg_image *bgrx_img)
{
    cam_status status = CAM_FAILURE;
    wg_uint width;
    wg_uint height;
    wg_uint row;
    wg_uint col;
    wg_uint32 *bgrx_pixel;
    rgb24_pixel *rgb_pixel;

    CHECK_FOR_NULL_PARAM(rgb_img);
    CHECK_FOR_NULL_PARAM(bgrx_img);

    if (rgb_img->type != IMG_RGB){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                rgb_img->type, IMG_RGB);
        return CAM_FAILURE;
    }

    cam_img_get_width(rgb_img, &width);
    cam_img_get_height(rgb_img, &height);

    status = cam_img_fill(width, height, BGRX_COMPONENT_NUM, IMG_BGRX,
            bgrx_img);
    if (CAM_SUCCESS != status){
        return CAM_FAILURE;
    }
    for (row = 0; row < height; ++row){
        cam_img_get_row(rgb_img, row, (wg_uchar**)&rgb_pixel);
        cam_img_get_row(bgrx_img, row, (wg_uchar**)&bgrx_pixel);
        for (col = 0; col < width; ++col, ++bgrx_pixel, ++rgb_pixel){
            *bgrx_pixel = RGB_2_BGRX(
                    PIXEL_RED(*rgb_pixel),
                    PIXEL_GREEN(*rgb_pixel),
                    PIXEL_BLUE(*rgb_pixel)
                    );
        }
    }

    return CAM_SUCCESS;
}

/*! @} */
