#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <setjmp.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <gdk/gdk.h>
#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/img.h"


/*! @todo make cam_img.c independent feom any format */
#include "include/img_bgrx.h"
#include "include/img_rgb24.h"

/*! @defgroup image Image Manipulation
 */

/*! @{ */

#define MAX_COLOR_VALUE     255

/**
 * @brief RGB color component
 */
typedef wg_uchar JSAMPLE;

/**
 * @brief RGB row
 */
typedef JSAMPLE* JSAMPROW;

WG_PRIVATE void
fast_memcpy(wg_uchar *restrict dest, wg_uchar *restrict src, wg_size size);

WG_PRIVATE void
xfree_cb(guchar *pixels, gpointer data);

WG_PRIVATE wg_uchar
crop_color(wg_int color, wg_int value);

/** 
* @brief Create new image instance
*  
* @param width     width in pixels
* @param height    height in pixels
* @param comp_num  number of components per pixel
* @param type  type of the image
* @param img       memory to store image instance
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
cam_status
img_fill(wg_uint width, wg_uint height, wg_uint comp_num, img_type type,
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
* @brief Clean all resources allocated by img_fill()
* 
* @param img  imge instance
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
cam_status
img_cleanup(Wg_image *img)
{
    CHECK_FOR_NULL_PARAM(img);

    WG_FREE(img->rows);
    WG_FREE(img->image);

    memset(img, '\0', sizeof (Wg_image));

    return CAM_SUCCESS;
}

/** 
* @brief Get the subimage from an image
*  If subimage exceeds source image it will be croped. img_dec must be 
* initialized by img_fill() to set dimenstion of the subimage.
* 
* @param img_src  source image instance
* @param x        x position in source
* @param y        y position in source image
* @param img_dest
* 
* @retval CAM_SUCCESS
* @retval CAM_FAILURE
*/
cam_status
img_get_subimage(Wg_image *img_src, wg_uint x, wg_uint y, 
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
* @brief Convert Wg_image instance into GdkPixbuf
*
*    This function binds Wg_image with GdkPixbuf. After this call source image 
*    should not be used. Before pixbuf is released a free callback is called to
*    free all resources allocated by img. If free_cb is NULL then a default
*    callback is used which assumes that Wg_image was allocated using
*    WG_MALLOC/WG_CALLOC.
* 
* @param img        source image
* @param pixbuf     memory to store GdkPixbuf object
* @param free_cb    callback used to free source image before pixbuf
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
img_convert_to_pixbuf(Wg_image *img, GdkPixbuf **pixbuf,
        void (*free_cb)(guchar *, gpointer))
{
    GdkPixbuf *pix = NULL;
    GdkPixbuf *pix_dest = NULL;

    if (IMG_RGB != img->type){
        WG_LOG("Only RGB24 supported\n");
        return WG_FAILURE;
    }

    free_cb = ((free_cb == NULL) ? xfree_cb : free_cb);

    pix = gdk_pixbuf_new_from_data(img->image, 
            GDK_COLORSPACE_RGB, FALSE, 8, 
            img->width, img->height, 
            img->row_distance, 
            NULL, NULL);

    if (NULL == pixbuf){
        WG_LOG("Wg_image -> GdkPixbuf conversion error\n");
        return WG_FAILURE;
    }

    pix_dest = gdk_pixbuf_copy(pix);

    g_object_unref(pix);

    *pixbuf = pix_dest;

    return WG_SUCCESS;
}

WG_PRIVATE void
xfree_cb(guchar *pixels, gpointer data)
{
    img_cleanup(data);

    WG_FREE(data);
}

WG_PRIVATE wg_uchar
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
