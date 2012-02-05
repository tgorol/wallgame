#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/cam_img.h"
#include "include/cam_img_grayscale.h"
#include "include/cam_img_bgrx.h"
#include "include/cam_img_rgb.h"

#define FIX_POINT    16

#define MAX_16   ((1 << FIX_POINT) - 1)
#define C1      ((wg_uint32)(MAX_16 * 0.3))
#define C2      ((wg_uint32)(MAX_16 * 0.59))
#define C3      ((wg_uint32)(MAX_16 * 0.11))


/* Below expression calculates Y=0.3 RED + 0.59 GREEN + 0.11 Blue using
 * fixed point arythmetics
 */
#define  RGB_2_GS(r, g, b)  (((C1 * r) + (C2 * g) + (C3 * b)) >> FIX_POINT)

#define FF_POW  8

#define FF_FLOAT(val)   ((wg_uint32)(val) << (FF_POW))

#define FF_INT(val)      ((wg_uint32)(val) >> (FF_POW))


wg_status
cam_img_rgb_2_grayscale(Wg_image *rgb_img, Wg_image *grayscale_img)
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

    if (rgb_img->type != IMG_RGB){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                rgb_img->type, IMG_RGB);
        return CAM_FAILURE;
    }

    cam_img_get_width(rgb_img, &width);
    cam_img_get_height(rgb_img, &height);

    status = cam_img_fill(width, height, GS_COMPONENT_NUM, IMG_GS,
            grayscale_img);
    if (CAM_SUCCESS != status){
        return CAM_FAILURE;
    }
    for (row = 0; row < height; ++row){
        cam_img_get_row(rgb_img, row, (wg_uchar**)&rgb_pixel);
        cam_img_get_row(grayscale_img, row, (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; ++col, ++rgb_pixel, ++gs_pixel){
            *gs_pixel = RGB_2_GS(
                    PIXEL_RED(*rgb_pixel),
                    PIXEL_GREEN(*rgb_pixel),
                    PIXEL_BLUE(*rgb_pixel)
                    );

        }
    }

    return CAM_SUCCESS;
}

cam_status
cam_img_grayscale_histogram(Wg_image* grayscale_img, wg_int *histogram, 
        wg_size size)
{
    wg_uint width;
    wg_uint height;
    wg_uint row;
    wg_uint col;
    gray_pixel *gs_pixel;
    wg_int i;

    CHECK_FOR_NULL_PARAM(grayscale_img);

    CHECK_FOR_RANGE_LT(size, GS_PIXEL_MAX);

    for (i = 0; i < size; ++i){
        histogram[i] = 0;
    }

    if (grayscale_img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                grayscale_img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(grayscale_img, &width);
    cam_img_get_height(grayscale_img, &height);

    for (row = 0; row < height; ++row){
        cam_img_get_row(grayscale_img, row, (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; ++col, ++gs_pixel){
            ++histogram[*gs_pixel];
        }
    }

    return CAM_SUCCESS;
}

wg_status
cam_img_grayscale_normalize(Wg_image* grayscale_img, gray_pixel new_max,
        gray_pixel new_min)
{
    wg_uint width;
    wg_uint height;
    wg_uint row;
    wg_uint col;
    gray_pixel *gs_pixel;
    gray_pixel gs_max;
    gray_pixel gs_min;
    wg_uint gs_range;
    wg_uint new_range;

    CHECK_FOR_NULL_PARAM(grayscale_img);

    if (grayscale_img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                grayscale_img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(grayscale_img, &width);
    cam_img_get_height(grayscale_img, &height);

    cam_img_grayscale_max_min(grayscale_img, &gs_max, &gs_min);

    gs_range = FF_FLOAT(gs_max - gs_min);
    new_range = FF_FLOAT(new_max - new_min);

    for (row = 0; row < height; ++row){
        cam_img_get_row(grayscale_img, row, (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; ++col, ++gs_pixel){
            *gs_pixel = 
               FF_INT((FF_FLOAT(*gs_pixel - gs_min) * new_range) / gs_range) 
		 + new_min;
        }
    }

    return CAM_SUCCESS;
}

wg_status
cam_img_grayscale_max_min(Wg_image* grayscale_img, gray_pixel *gs_max,
        gray_pixel *gs_min)
{
    gray_pixel *gs_pixel;
    wg_uint width;
    wg_uint height;
    wg_uint row;
    wg_uint col;
    gray_pixel max_val = GS_PIXEL_MIN;
    gray_pixel min_val = GS_PIXEL_MAX;

    CHECK_FOR_NULL_PARAM(grayscale_img);
    CHECK_FOR_NULL_PARAM(gs_max);
    CHECK_FOR_NULL_PARAM(gs_min);

    if (grayscale_img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                grayscale_img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(grayscale_img, &width);
    cam_img_get_height(grayscale_img, &height);

    for (row = 0; row < height; ++row){
        cam_img_get_row(grayscale_img, row, (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; ++col, ++gs_pixel){
            max_val = WG_MAX(max_val, *gs_pixel);
            min_val = WG_MIN(min_val, *gs_pixel);
        }
    }

    *gs_max = max_val;
    *gs_min = min_val;

    return CAM_SUCCESS;
}
