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
#include "include/cam_img_grayscale.h"

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
cam_img_bgrx_2_rgb(Wg_image *bgrx_img, Wg_image *rgb_img)
{
    cam_status status = CAM_FAILURE;
    wg_uint width;
    wg_uint height;
    wg_uint row;
    wg_uint col;
    bgrx_pixel *bgrx_pixel;
    rgb24_pixel *rgb_pixel;

    CHECK_FOR_NULL_PARAM(rgb_img);
    CHECK_FOR_NULL_PARAM(bgrx_img);

    if (bgrx_img->type != IMG_BGRX){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                bgrx_img->type, IMG_BGRX);
        return CAM_FAILURE;
    }

    cam_img_get_width(bgrx_img, &width);
    cam_img_get_height(bgrx_img, &height);

    status = cam_img_fill(width, height, RGB24_COMPONENT_NUM, IMG_BGRX,
            rgb_img);
    if (CAM_SUCCESS != status){
        return CAM_FAILURE;
    }
    for (row = 0; row < height; ++row){
        cam_img_get_row(rgb_img, row, (wg_uchar**)&rgb_pixel);
        cam_img_get_row(bgrx_img, row, (wg_uchar**)&bgrx_pixel);
        for (col = 0; col < width; ++col, ++bgrx_pixel, ++rgb_pixel){
            bgrx_2_rgb(*bgrx_pixel, *rgb_pixel);
        }
    }

    return CAM_SUCCESS;
}

wg_status
cam_img_grayscale_2_rgb(Wg_image *grayscale_img, Wg_image *rgb_img)
{
    cam_status status = CAM_FAILURE;
    wg_uint width;
    wg_uint height;
    wg_uint row;
    wg_uint col;
    gray_pixel *gs_pixel;
    rgb24_pixel *rgb_pixel;

    CHECK_FOR_NULL_PARAM(rgb_img);
    CHECK_FOR_NULL_PARAM(grayscale_img);

    if (grayscale_img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                grayscale_img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(grayscale_img, &width);
    cam_img_get_height(grayscale_img, &height);

    status = cam_img_fill(width, height, RGB24_COMPONENT_NUM, IMG_RGB,
            rgb_img);
    if (CAM_SUCCESS != status){
        return CAM_FAILURE;
    }
    for (row = 0; row < height; ++row){
        cam_img_get_row(rgb_img, row, (wg_uchar**)&rgb_pixel);
        cam_img_get_row(grayscale_img, row, (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; ++col, ++rgb_pixel, ++gs_pixel){
            gs_2_rgb(*gs_pixel, *rgb_pixel);

        }
    }

    return CAM_SUCCESS;
}

/*! @} */
