#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <img.h>

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
img_rgb_2_grayscale(Wg_image *rgb_img, Wg_image *grayscale_img)
{
    register gray_pixel *gs_pixel;
    register rgb24_pixel *rgb_pixel;
    wg_status status = WG_FAILURE;
    wg_uint width = 0;
    wg_uint height = 0;
    Img_iterator rgb_itr;
    Img_iterator gs_itr;

    CHECK_FOR_NULL_PARAM(rgb_img);
    CHECK_FOR_NULL_PARAM(grayscale_img);

    if (rgb_img->type != IMG_RGB){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                rgb_img->type, IMG_RGB);
        return WG_FAILURE;
    }

    img_get_width(rgb_img, &width);
    img_get_height(rgb_img, &height);

    status = img_fill(width, height, GS_COMPONENT_NUM, IMG_GS,
            grayscale_img);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }

    img_get_iterator(rgb_img, &rgb_itr);
    img_get_iterator(grayscale_img, &gs_itr);

    while (img_iterator_has_next_row(&rgb_itr)){
        img_iterator_next_row(&rgb_itr);
        img_iterator_next_row(&gs_itr);
        while (img_iterator_has_next_col(&rgb_itr)){
            rgb_pixel = (rgb24_pixel*)img_iterator_next_col(&rgb_itr);
            gs_pixel = (gray_pixel*)img_iterator_next_col(&gs_itr);
            *gs_pixel = RGB_2_GS(
                    RGB24_PIXEL_RED(*rgb_pixel),
                    RGB24_PIXEL_GREEN(*rgb_pixel),
                    RGB24_PIXEL_BLUE(*rgb_pixel)
                    );
        }
    }

    return WG_SUCCESS;
}

/** 
* @brief Get histogram of the image
* 
* @param grayscale_img  input image
* @param histogram      array to store histogram
* @param size           minimum must GS_PIXEL_MAX
* 
* @return 
*/
wg_status
img_gs_histogram(Wg_image* grayscale_img, wg_uint *histogram,
        wg_uint size)
{
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;
    gray_pixel *gs_pixel = NULL;
    wg_int i = 0;

    CHECK_FOR_NULL_PARAM(grayscale_img);

    CHECK_FOR_RANGE_LT(size, GS_PIXEL_MAX);

    for (i = 0; i < size; ++i){
        histogram[i] = 0;
    }

    if (grayscale_img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                grayscale_img->type, IMG_GS);
        return WG_FAILURE;
    }

    img_get_width(grayscale_img, &width);
    img_get_height(grayscale_img, &height);

    for (row = 0; row < height; ++row){
        img_get_row(grayscale_img, row, (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; ++col, ++gs_pixel){
            ++histogram[*gs_pixel];
        }
    }

    return WG_SUCCESS;
}

wg_status
img_gs_normalize(Wg_image* grayscale_img, gray_pixel new_max,
        gray_pixel new_min)
{
    gray_pixel *gs_pixel = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;
    gray_pixel gs_max = 0;
    gray_pixel gs_min = 0;
    wg_uint gs_range = 0;
    wg_uint new_range = 0;

    CHECK_FOR_NULL_PARAM(grayscale_img);

    if (grayscale_img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                grayscale_img->type, IMG_GS);
        return WG_FAILURE;
    }

    img_get_width(grayscale_img, &width);
    img_get_height(grayscale_img, &height);

    img_gs_max_min(grayscale_img, &gs_max, &gs_min);

    gs_range = FF_FLOAT(gs_max - gs_min);
    new_range = FF_FLOAT(new_max - new_min);

    if (gs_range != 0){
        for (row = 0; row < height; ++row){
            img_get_row(grayscale_img, row, (wg_uchar**)&gs_pixel);
            for (col = 0; col < width; ++col, ++gs_pixel){
                *gs_pixel = 
                    FF_INT((
                          FF_FLOAT(*gs_pixel - gs_min) * new_range) / gs_range
                          ) + new_min;
            }
        }
    }

    return WG_SUCCESS;
}

wg_status
img_gs_max_min(Wg_image* grayscale_img, gray_pixel *gs_max,
        gray_pixel *gs_min)
{
    gray_pixel *gs_pixel = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;
    gray_pixel max_val = GS_PIXEL_MIN;
    gray_pixel min_val = GS_PIXEL_MAX;

    CHECK_FOR_NULL_PARAM(grayscale_img);
    CHECK_FOR_NULL_PARAM(gs_max);
    CHECK_FOR_NULL_PARAM(gs_min);

    if (grayscale_img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                grayscale_img->type, IMG_GS);
        return WG_FAILURE;
    }

    img_get_width(grayscale_img, &width);
    img_get_height(grayscale_img, &height);

    for (row = 0; row < height; ++row){
        img_get_row(grayscale_img, row, (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; ++col, ++gs_pixel){
            max_val = WG_MAX(max_val, *gs_pixel);
            min_val = WG_MIN(min_val, *gs_pixel);
        }
    }

    *gs_max = max_val;
    *gs_min = min_val;

    return WG_SUCCESS;
}

wg_status
img_gs_save(Wg_image *img, wg_char *filename, wg_char *ext)
{
    Wg_image rgb;
    wg_status status = WG_FAILURE;
    gboolean error_flag = FALSE;
    GdkPixbuf *pixbuf = NULL;

    status = img_gs_2_rgb(img, &rgb);
    if (WG_SUCCESS != status){
        WG_LOG("GS to RGB conversion error\n");
        return WG_FAILURE;
    }

    pixbuf = gdk_pixbuf_new_from_data(rgb.image, GDK_COLORSPACE_RGB, FALSE, 8, 
                rgb.width, rgb.height, rgb.row_distance, NULL, NULL);

    error_flag = gdk_pixbuf_save(pixbuf, filename, ext, NULL, NULL);
    if (TRUE != error_flag){
        WG_LOG("GS to RGB conversion error\n");
        /* pass through to release resources */
        status = WG_FAILURE;
    }

    g_object_unref(pixbuf);

    img_cleanup(&rgb);

    return status;
}

wg_status
img_gs_draw_pixel(Wg_image *img, wg_int y, wg_int x, va_list args)
{
    gray_pixel *gs_pixel = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint value;

    CHECK_FOR_NULL_PARAM(img);

    if (img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_GS);
        return WG_FAILURE;
    }

    value = (gray_pixel)va_arg(args, wg_uint);

    img_get_width(img, &width);
    img_get_height(img, &height);

    if ((x < width) && (y < height) && (x >= 0) && (y >= 0)){
        img_get_pixel(img, y, x, &gs_pixel);
        *gs_pixel = (gray_pixel)value;
    }

    return WG_SUCCESS;

}

wg_status
img_gs_sub(Wg_image *img_1, Wg_image *img_2)
{
    gray_pixel *gs_pixel_1 = NULL;
    gray_pixel *gs_pixel_2 = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;

    CHECK_FOR_NULL_PARAM(img_1);
    CHECK_FOR_NULL_PARAM(img_2);

    if ((img_1->type != IMG_GS) || (img_2->type != IMG_GS)){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img_1->type, IMG_GS);
        return WG_FAILURE;
    }

    img_get_width(img_1, &width);
    img_get_height(img_1, &height);

    for (row = 0; row < height; ++row){
        img_get_row(img_1, row, (wg_uchar**)&gs_pixel_1);
        img_get_row(img_2, row, (wg_uchar**)&gs_pixel_2);
        for (col = 0; col < width; ++col, ++gs_pixel_1, ++gs_pixel_2){
            *gs_pixel_1 = abs(*gs_pixel_1 - *gs_pixel_2);
        }
    }

    return WG_SUCCESS;
}

