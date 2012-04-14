#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_sort.h>
#include <img.h>

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
 * @retval WG_SUCCESS
 * @retval WG_SUCCESS
 */
wg_status
img_bgrx_2_rgb(Wg_image *bgrx_img, Wg_image *rgb_img)
{
    wg_status status = WG_FAILURE;
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
        return WG_FAILURE;
    }

    img_get_width(bgrx_img, &width);
    img_get_height(bgrx_img, &height);

    status = img_fill(width, height, RGB24_COMPONENT_NUM, IMG_RGB,
            rgb_img);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }
    for (row = 0; row < height; ++row){
        img_get_row(rgb_img, row, (wg_uchar**)&rgb_pixel);
        img_get_row(bgrx_img, row, (wg_uchar**)&bgrx_pixel);
        for (col = 0; col < width; ++col, ++bgrx_pixel, ++rgb_pixel){
            bgrx_2_rgb(*bgrx_pixel, *rgb_pixel);
        }
    }

    return WG_SUCCESS;
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
img_gs_2_rgb(Wg_image *grayscale_img, Wg_image *rgb_img)
{
    wg_status status = WG_FAILURE;
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
        return WG_FAILURE;
    }

    img_get_width(grayscale_img, &width);
    img_get_height(grayscale_img, &height);

    status = img_fill(width, height, RGB24_COMPONENT_NUM, IMG_RGB,
            rgb_img);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }
    for (row = 0; row < height; ++row){
        img_get_row(rgb_img, row, (wg_uchar**)&rgb_pixel);
        img_get_row(grayscale_img, row, (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; ++col, ++rgb_pixel, ++gs_pixel){
            gs_2_rgb(*gs_pixel, *rgb_pixel);

        }
    }

    return WG_SUCCESS;
}

/** 
* @brief Draw pixel on RGB24 image
* 
* @param img    image instance
* @param y      y position
* @param x      x position
* @param args   color (rgb_pixel*)
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
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

/** 
* @brief Create RGB24 image from buffer
* 
* @param buffer   buffer 
* @param width    width of image
* @param height   height of image
* @param img      memory to store image
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
img_rgb_from_buffer(wg_uchar *buffer, wg_uint width, wg_uint height, 
        Wg_image *img)
{
    wg_uint i = 0;
    rgb24_pixel *pixel = NULL;
    wg_uint row_size = 0;

    CHECK_FOR_NULL_PARAM(buffer);

    row_size = width * RGB24_COMPONENT_NUM;

    img_fill(width, height, RGB24_COMPONENT_NUM, IMG_RGB, img);

    for (i = 0; i < height; ++i){
        img_get_row(img, i, (wg_uchar**)&pixel);
        fast_memcpy((wg_uchar*)pixel, (wg_uchar*)buffer, row_size);
        buffer += row_size;
    }

    return WG_SUCCESS;
}


/** @brief Average pixel */
#define avg(gs_pixel, c)                                       \
                 ((gs_pixel[0][c] + gs_pixel[1][c] +           \
                 gs_pixel[2][c] +                              \
                 gs_pixel[rd + 0][c] + gs_pixel[rd + 1][c] +   \
                 gs_pixel[rd + 2][c] +                         \
                 gs_pixel[rd2 + 0][c] + gs_pixel[rd2 + 1][c] + \
                 gs_pixel[rd2 + 2][c]) / 9)

/** 
* @brief Use median filter on the image.
*  
* @param img      source image instance
* @param new_img  memory for filtered image instance
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
img_rgb_median_filter(Wg_image *img, Wg_image *new_img)
{
    register rgb24_pixel *rgb_pixel = NULL;
    rgb24_pixel *tmp_pixel = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;
    rgb24_pixel *rgb_new_pixel = NULL;
    wg_int rd = 0;
    wg_int rd2 = 0;
    wg_uint data[5];

    CHECK_FOR_NULL_PARAM(img);

    if (img->type != IMG_RGB){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_RGB);
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
        rgb_pixel = tmp_pixel;
        img_get_row(new_img, row, (wg_uchar**)&rgb_new_pixel);
        for (col = 0; col < width; ++col, ++rgb_pixel, ++rgb_new_pixel){
            data[0] = rgb_pixel[0][RGB24_R];
            data[1] = rgb_pixel[2][RGB24_R];

            data[2] = rgb_pixel[rd + 1][RGB24_R];

            data[3] = rgb_pixel[rd2 + 0][RGB24_R];
            data[4] = rgb_pixel[rd2 + 2][RGB24_R];

            wg_sort_uint(data, ELEMNUM(data));
            rgb_new_pixel[0][RGB24_R] = data[2];

            data[0] = rgb_pixel[0][RGB24_G];
            data[1] = rgb_pixel[2][RGB24_G];

            data[2] = rgb_pixel[rd + 1][RGB24_G];

            data[3] = rgb_pixel[rd2 + 0][RGB24_G];
            data[4] = rgb_pixel[rd2 + 2][RGB24_G];

            wg_sort_uint(data, ELEMNUM(data));
            rgb_new_pixel[0][RGB24_G] = data[2];

            data[0] = rgb_pixel[0][RGB24_B];
            data[1] = rgb_pixel[2][RGB24_B];

            data[2] = rgb_pixel[rd + 1][RGB24_B];

            data[3] = rgb_pixel[rd2 + 0][RGB24_B];
            data[4] = rgb_pixel[rd2 + 2][RGB24_B];

            wg_sort_uint(data, ELEMNUM(data));
            rgb_new_pixel[0][RGB24_B] = data[2];
        }
    }

    return WG_SUCCESS;
}

/*! @} */
