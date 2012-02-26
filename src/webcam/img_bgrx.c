#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/img.h"

#define BGRX_COMPONENT_NUM    4

/*! @defgroup image_bgrx BGRX manipulation
 * @ingroup image
 */

/*! @{ */


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
img_rgb_2_bgrx(Wg_image *rgb_img, Wg_image *bgrx_img)
{
    cam_status status = CAM_FAILURE;
    wg_uint32 *bgrx_pixel = NULL;
    rgb24_pixel *rgb_pixel = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;

    CHECK_FOR_NULL_PARAM(rgb_img);
    CHECK_FOR_NULL_PARAM(bgrx_img);

    if (rgb_img->type != IMG_RGB){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                rgb_img->type, IMG_RGB);
        return CAM_FAILURE;
    }

    img_get_width(rgb_img, &width);
    img_get_height(rgb_img, &height);

    status = img_fill(width, height, BGRX_COMPONENT_NUM, IMG_BGRX,
            bgrx_img);
    if (CAM_SUCCESS != status){
        return CAM_FAILURE;
    }
    for (row = 0; row < height; ++row){
        img_get_row(rgb_img, row, (wg_uchar**)&rgb_pixel);
        img_get_row(bgrx_img, row, (wg_uchar**)&bgrx_pixel);
        for (col = 0; col < width; ++col, ++bgrx_pixel, ++rgb_pixel){
            *bgrx_pixel = RGB_2_BGRX(
                    RGB24_PIXEL_RED(*rgb_pixel),
                    RGB24_PIXEL_GREEN(*rgb_pixel),
                    RGB24_PIXEL_BLUE(*rgb_pixel)
                    );
        }
    }

    return CAM_SUCCESS;
}

/*! @} */
