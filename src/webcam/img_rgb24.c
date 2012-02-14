#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/img.h"
#include "include/img_bgrx.h"
#include "include/img_rgb24.h"
#include "include/img_gs.h"

/*! @defgroup webcam_rgb24 RGB24 Conversion Functions
 *  @ingroup image
 */

/*! @{ */

WG_INLINE void
gs_2_rgb(gray_pixel gs, rgb24_pixel rgb)
{
    rgb[RGB24_R] = gs;
    rgb[RGB24_G] = gs;
    rgb[RGB24_B] = gs;
}

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
img_bgrx_2_rgb(Wg_image *bgrx_img, Wg_image *rgb_img)
{
    cam_status status = CAM_FAILURE;
    bgrx_pixel *bgrx_pixel = NULL;
    rgb24_pixel *rgb_pixel = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;

    CHECK_FOR_NULL_PARAM(rgb_img);
    CHECK_FOR_NULL_PARAM(bgrx_img);

    if (bgrx_img->type != IMG_BGRX){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                bgrx_img->type, IMG_BGRX);
        return CAM_FAILURE;
    }

    img_get_width(bgrx_img, &width);
    img_get_height(bgrx_img, &height);

    status = img_fill(width, height, RGB24_COMPONENT_NUM, IMG_BGRX,
            rgb_img);
    if (CAM_SUCCESS != status){
        return CAM_FAILURE;
    }
    for (row = 0; row < height; ++row){
        img_get_row(rgb_img, row, (wg_uchar**)&rgb_pixel);
        img_get_row(bgrx_img, row, (wg_uchar**)&bgrx_pixel);
        for (col = 0; col < width; ++col, ++bgrx_pixel, ++rgb_pixel){
            bgrx_2_rgb(*bgrx_pixel, *rgb_pixel);
        }
    }

    return CAM_SUCCESS;
}

/** 
* @brief Convert grayscale image to rgb24
* 
* @param grayscale_img grayscale image
* @param rgb_img       memory to store rgb24 picture
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
img_grayscale_2_rgb(Wg_image *grayscale_img, Wg_image *rgb_img)
{
    cam_status status = CAM_FAILURE;
    gray_pixel *gs_pixel = NULL;
    rgb24_pixel *rgb_pixel = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;

    CHECK_FOR_NULL_PARAM(rgb_img);
    CHECK_FOR_NULL_PARAM(grayscale_img);

    if (grayscale_img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                grayscale_img->type, IMG_GS);
        return CAM_FAILURE;
    }

    img_get_width(grayscale_img, &width);
    img_get_height(grayscale_img, &height);

    status = img_fill(width, height, RGB24_COMPONENT_NUM, IMG_RGB,
            rgb_img);
    if (CAM_SUCCESS != status){
        return CAM_FAILURE;
    }
    for (row = 0; row < height; ++row){
        img_get_row(rgb_img, row, (wg_uchar**)&rgb_pixel);
        img_get_row(grayscale_img, row, (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; ++col, ++rgb_pixel, ++gs_pixel){
            gs_2_rgb(*gs_pixel, *rgb_pixel);

        }
    }

    return CAM_SUCCESS;
}

wg_status
img_rgb_draw_pixel(Wg_image *img, wg_int y, wg_int x, va_list args)
{
    rgb24_pixel *rgb_pixel = NULL;
    rgb24_pixel *new_pixel = NULL;
    wg_uint width = 0;
    wg_uint height = 0;

    if (img->type != IMG_RGB){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_RGB);
        return WG_FAILURE;
    }

    new_pixel = (rgb24_pixel*)va_arg(args, rgb24_pixel*);

    img_get_width(img, &width);
    img_get_height(img, &height);

    if ((x < width) && (y < height) && (x >= 0) && (y >= 0)){
        img_get_pixel(img, y, x, (wg_uint8**)&rgb_pixel);
        rgb_pixel[0][RGB24_R] = new_pixel[0][RGB24_R];
        rgb_pixel[0][RGB24_G] = new_pixel[0][RGB24_G];
        rgb_pixel[0][RGB24_B] = new_pixel[0][RGB24_B];
    }

    return WG_SUCCESS;
}

/*! @} */
