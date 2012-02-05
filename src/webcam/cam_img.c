#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <setjmp.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/cam_img.h"


/*! @todo make cam_img.c independent feom any format */
#include "include/cam_img_bgrx.h"
#include "include/cam_img_rgb.h"

/*! @defgroup image Image Manipulation
 */

/*! @{ */

#define BG_DEFAULT_RED      0
#define BG_DEFAULT_GREEN    0
#define BG_DEFAULT_BLUE     0

#define FG_DEFAULT_RED      255
#define FG_DEFAULT_GREEN    255
#define FG_DEFAULT_BLUE     255

#define MAX_COLOR_VALUE     255

/**
 * @brief RGB color component
 */
typedef wg_uchar JSAMPLE;

/**
 * @brief RGB row
 */
typedef JSAMPLE* JSAMPROW;

typedef struct Threshold{
    Wg_rgb base;
    Wg_rgb threshold;
    Wg_rgb background;
    Wg_rgb foreground;
} Threshold;


WG_STATIC void
fast_memcpy(wg_uchar *restrict dest, wg_uchar *restrict src, wg_size size);

WG_PRIVATE cam_status
row_filter_color_threshold(wg_uchar *row, wg_uint width, Threshold *threshold);

inline WG_PRIVATE wg_uchar
crop_color(wg_int color, wg_int value);

WG_PRIVATE const Wg_rgb default_bg = {
    .red   = BG_DEFAULT_RED     ,
    .green = BG_DEFAULT_GREEN   ,
    .blue  = BG_DEFAULT_BLUE    
};

WG_PRIVATE const Wg_rgb default_fg = {
    .red   = FG_DEFAULT_RED     ,
    .green = FG_DEFAULT_GREEN   ,
    .blue  = FG_DEFAULT_BLUE    
};
    

/**
 * @brief Fill image structure
 *
 * @param height   height of the image in pixels
 * @param width    width of the image in pixels
 * @param comp_num number of compenents per pixel
 * @param img      image structure
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_img_fill(wg_uint width, wg_uint height, wg_uint comp_num, img_type type,
        Wg_image *img)
{
    register JSAMPROW *tmp_raw_array = NULL;
    JSAMPROW *row_array     = NULL;
    wg_uint  row_size       = 0;
    JSAMPLE  *raw_data      = NULL;
    wg_int   i = 0;

    CHECK_FOR_NULL(img);

    /* allocate memory for arrayf of pointers to rows */
    row_array = WG_CALLOC(height, sizeof (JSAMPROW));
    if (NULL == row_array){
        return CAM_FAILURE;
    }

    /* calculate size of a row                      */
    row_size = width * comp_num * sizeof (JSAMPLE);

    /* allocate memory for decompressed image       */
    raw_data = WG_CALLOC(height, row_size);
    if (NULL == row_array){
        WG_FREE(row_array);
        return CAM_FAILURE;
    }

    /* fill an array of pointers to rows            */
     tmp_raw_array = row_array;
    *tmp_raw_array++ = raw_data;
    for (i = 1; i < height; ++i){
        *tmp_raw_array = *(tmp_raw_array - 1) + row_size;
        ++tmp_raw_array;
    }

    /* return image parameters to te caller         */
    img->image                = raw_data;
    img->width                = width;
    img->height               = height;
    img->size                 = height * row_size;
    img->components_per_pixel = comp_num;
    img->rows                 = row_array;
    img->row_distance         = row_size;
    img->type                 = type;

    return CAM_SUCCESS;
}

/**
 * @brief Cleanup decompressed image buffers
 *
 * @param img  decomressed image after decompression
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_img_cleanup(Wg_image *img)
{
    CHECK_FOR_NULL_PARAM(img);

    WG_FREE(img->rows);
    WG_FREE(img->image);

    memset(img, '\0', sizeof (Wg_image));

    return CAM_SUCCESS;
}

/**
 * @brief Get subimage
 *
 * img_dest must be initialized with cam_img_fill()
 *
 * @param img_src   source image
 * @param x         x
 * @param y         y
 * @param img_dest  destination image
 *
 * @return 
 */
cam_status
cam_img_get_subimage(Wg_image *img_src, wg_uint x, wg_uint y, 
        Wg_image *img_dest)
{
    wg_uint row_index = 0; 
    wg_uchar **row = NULL; 
    wg_uint width = 0;
    wg_uint x_off = 0;

    CHECK_FOR_NULL_PARAM(img_src);
    CHECK_FOR_NULL_PARAM(img_dest);
    CHECK_FOR_RANGE_GT(img_dest->width, img_src->width);
    CHECK_FOR_RANGE_GT(x + img_dest->width, img_src->width);
    CHECK_FOR_RANGE_GT(y + img_dest->height, img_src->height);

    row = &(img_src->rows[y]);

    width = img_dest->width * img_dest->components_per_pixel;
    x_off = img_dest->components_per_pixel * x;

    for (row_index = 0; row_index < img_dest->height; ++row_index, ++row){
        fast_memcpy(img_dest->rows[row_index], *row + x_off, width);
    }

    return CAM_SUCCESS;
}

/**
 * @brief Filter image using color threshold method
 *
 * If background is NULL then default background is used (Black).
 *
 * @param img         image instance
 * @param base        threshold base color
 * @param threshold   threshold color range
 * @param background  background color
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_img_filter_color_threshold(const Wg_image *img, const Wg_rgb *base, 
        const Wg_rgb *threshold, const Wg_rgb *background, 
        const Wg_rgb *foreground)
{
    wg_int      i = 0;
    Threshold th;

    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_NULL_PARAM(base);
    CHECK_FOR_NULL_PARAM(threshold);

    th.base       = *base;
    th.background = background != NULL ? *background : default_bg;
    th.foreground = foreground != NULL ? *foreground : default_fg;

    /* crop each color component value at MAX_COLOR_VALUE   */
    th.threshold.red = 
        crop_color(base->red + threshold->red, MAX_COLOR_VALUE);

    th.threshold.green = 
        crop_color(base->green + threshold->green, MAX_COLOR_VALUE);

    th.threshold.blue = 
        crop_color(base->blue + threshold->blue, MAX_COLOR_VALUE);

    /* filter all rows */
    for (i = 0; i < img->height; ++i){
        row_filter_color_threshold(img->rows[i], img->width, &th);
    }

    return CAM_SUCCESS;
}

/**
 * @brief Filter row of the image using color threshold method
 *
 * @param row         row to filter
 * @param width       number of color components in the row
 * @param base        threshold base color
 * @param threshold   threshold color range
 * @param background  background color
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
WG_PRIVATE cam_status
row_filter_color_threshold(wg_uchar *row, wg_uint width, Threshold *threshold)
{
    register Threshold *th = NULL;
    register rgb24_pixel *component = NULL;
    wg_int R = 0;
    wg_int G = 0;
    wg_int B = 0;
    wg_boolean put_through = WG_FALSE;

    CHECK_FOR_NULL_PARAM(row);
    CHECK_FOR_NULL_PARAM(threshold);

    component = (rgb24_pixel*) row;

    th = threshold;

    /* loop through all pixels in the row */
    while (width-- != 0){
        R = PIXEL_RED(component[width]);
        G = PIXEL_GREEN(component[width]);
        B = PIXEL_BLUE(component[width]);

        put_through = WG_FALSE;

        /* check if pixel inside defined range */
        do{
            if ((R < th->base.red) || (R > th->threshold.red)){
                break;
            }

            if ((G < th->base.green) || (G > th->threshold.green)){
                break;
            }

            if ((B < th->base.blue) || (B > th->threshold.blue)){
                break;
            }
            put_through = WG_TRUE;
        }while(0);
        /* if pixel outside the range overwrite it with background color */
        if (put_through == WG_FALSE){
            PIXEL_RED(component[width])   = th->background.red;
            PIXEL_GREEN(component[width]) = th->background.green;
            PIXEL_BLUE(component[width])  = th->background.blue;
        }else{
            PIXEL_RED(component[width])   = th->foreground.red;
            PIXEL_GREEN(component[width]) = th->foreground.green;
            PIXEL_BLUE(component[width])  = th->foreground.blue;
        }
    }

    return CAM_SUCCESS;
}

inline WG_PRIVATE wg_uchar
crop_color(wg_int color, wg_int value)
{
    return color > value ? value : color;
}

/**
 * @brief Fast memcpy
 *
 * @param dest   destination buffer
 * @param src    source buffer
 * @param size   number of bytes to copy
 *
 * @return void
 */
inline WG_STATIC void
fast_memcpy(wg_uchar *restrict dest, wg_uchar *restrict src, const wg_size size)
{
    __asm__ (
            "movl %0, %%edi\n\t"
            "movl %1, %%esi\n\t"
            "movl %2, %%ecx\n\t"
            "movl %%ecx, %%edx\n\t"
            "andl $0x3, %%ecx\n\t"
            "rep movsb\n\t"
            "movl %%edx, %%ecx\n\t"
            "shr $2, %%ecx\n\t"
            "rep movsd\n\t"
            :
            :"g"(dest), "g"(src), "g"(size)
            :"edx", "ecx", "esi", "edi"
            );
    return;
}

/*! @} */
