#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_sort.h>
#include <img.h>

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
 * @retval WG_SUCCESS
 * @retval WG_SUCCESS
 */
wg_status
img_rgb_2_bgrx(Wg_image *rgb_img, Wg_image *bgrx_img)
{
    wg_status status = WG_FAILURE;
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
        return WG_FAILURE;
    }

    img_get_width(rgb_img, &width);
    img_get_height(rgb_img, &height);

    status = img_fill(width, height, BGRX_COMPONENT_NUM, IMG_BGRX,
            bgrx_img);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
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

    return WG_SUCCESS;
}

wg_status
img_bgrx_median_filter(Wg_image *img, Wg_image *new_img)
{
    register bgrx_pixel *pixel = NULL;
    bgrx_pixel *new_pixel = NULL;
    bgrx_pixel *tmp_pixel = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;
    wg_int rd = 0;
    wg_int rd2 = 0;
    wg_uint data_red[9];
    wg_uint data_green[9];
    wg_uint data_blue[9];
    bgrx_pixel pix_val = 0;

    CHECK_FOR_NULL_PARAM(img);

    if (img->type != IMG_BGRX){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_BGRX);
        return WG_FAILURE;
    }

    img_get_width(img, &width);
    img_get_height(img, &height);

    height -= 2;
    width  -= 2;

    img_fill(width, height, img->components_per_pixel, img->type,
            new_img);

    rd = img->width;
    rd2 = rd + rd;

    for (row = 0; row < height; ++row){
        img_get_row(img, row, (wg_uchar**)&tmp_pixel);
        pixel = tmp_pixel;
        img_get_row(new_img, row, (wg_uchar**)&new_pixel);
        for (col = 0; col < width; ++col, ++pixel, ++new_pixel){
            pix_val = pixel[1];
            data_red[0]   = BGRX_R(pix_val);
            data_green[0] = BGRX_G(pix_val);
            data_blue[0]  = BGRX_B(pix_val);

            pix_val = pixel[0];
            data_red[5]   = BGRX_R(pix_val);
            data_green[5] = BGRX_G(pix_val);
            data_blue[5]  = BGRX_B(pix_val);

            pix_val = pixel[2];
            data_red[6]   = BGRX_R(pix_val);
            data_green[6] = BGRX_G(pix_val);
            data_blue[6]  = BGRX_B(pix_val);

            pix_val = pixel[rd + 0];
            data_red[1]   = BGRX_R(pix_val);
            data_green[1] = BGRX_G(pix_val);
            data_blue[1]  = BGRX_B(pix_val);

            pix_val = pixel[rd + 1];
            data_red[2]   = BGRX_R(pix_val);
            data_green[2] = BGRX_G(pix_val);
            data_blue[2]  = BGRX_B(pix_val);

            pix_val = pixel[rd + 2];
            data_red[3]   = BGRX_R(pix_val);
            data_green[3] = BGRX_G(pix_val);
            data_blue[3]  = BGRX_B(pix_val);

            pix_val = pixel[rd2 + 1];
            data_red[4]   = BGRX_R(pix_val);
            data_green[4] = BGRX_G(pix_val);
            data_blue[4]  = BGRX_B(pix_val);

            pix_val = pixel[rd2 + 0];
            data_red[7]   = BGRX_R(pix_val);
            data_green[7] = BGRX_G(pix_val);
            data_blue[7]  = BGRX_B(pix_val);

            pix_val = pixel[rd2 + 2];
            data_red[8]   = BGRX_R(pix_val);
            data_green[8] = BGRX_G(pix_val);
            data_blue[8]  = BGRX_B(pix_val);

            wg_sort_uint_insert(data_red,   ELEMNUM(data_red));
            wg_sort_uint_insert(data_green, ELEMNUM(data_green));
            wg_sort_uint_insert(data_blue,  ELEMNUM(data_blue));

            *new_pixel = RGB_2_BGRX(data_red[4], data_green[4], data_blue[4]);
        }
    }

    return WG_SUCCESS;
}

/*! @} */
