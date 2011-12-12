#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <setjmp.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <linux/videodev2.h>

#include "include/cam.h"

/*! @defgroup webcam_img Webcam Image Manipulation
 *  @ingroup webcam 
 */

/*! @{ */

/**
 * @brief RGB color component
 */
typedef wg_uchar JSAMPLE;

/**
 * @brief RGB row
 */
typedef JSAMPLE* JSAMPROW;

WG_STATIC void
fast_memcpy(wg_uchar *restrict dest, wg_uchar *restrict src, wg_size size);

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
cam_img_fill(wg_uint width, wg_uint height, wg_uint comp_num, Wg_image *img)
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
    CHECK_FOR_RANGE_GE(img_dest->width, img_src->width);
    CHECK_FOR_RANGE_GE(x + img_dest->width, img_src->width);
    CHECK_FOR_RANGE_GE(y + img_dest->height, img_src->height);

    row = &(img_src->rows[y]);

    width = img_dest->width * img_dest->components_per_pixel;
    x_off = img_dest->components_per_pixel * x;

    for (row_index = 0; row_index < img_dest->height; ++row_index, ++row){
        fast_memcpy(img_dest->rows[row_index], *row + x_off, width);
    }

    return CAM_SUCCESS;
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
            "shr $2, %%ecx\n\t"
            "rep movsd\n\t"
            "2:\tandl $0x3, %%edx\n\t"
            "movl %%edx, %%ecx\n\t"
            "rep movsb\n\t"
            "4:\n\t"
            :
            :"g"(dest), "g"(src), "g"(size)
            :"edx", "ecx", "esi", "edi"
            );
    return;
}

/*! @} */
