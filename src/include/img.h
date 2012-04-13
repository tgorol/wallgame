#ifndef _IMG_H
#define _IMG_H

#include <gdk/gdk.h>


/*! @addtogroup image
 * @{
 */

typedef enum img_type {
    IMG_INVALI  ,
    IMG_RGB     ,
    IMG_BGRX    ,
    IMG_YUYV    ,
    IMG_HSV     ,
    IMG_GS      ,
    IMG_USER
} img_type;

/**
 * @brief Represents an image returned by a decompressor
 */
typedef struct Wg_image{
    img_type type;         /*!< type of image                          */
    wg_uchar **rows;       /*!< array of rows                          */
    wg_uchar *image;       /*!< start of the image                     */
    wg_ssize size;         /*!< number of bytes in the image           */
    wg_uint  width;        /*!< width in pixels                        */
    wg_uint  height;       /*!< height in pixels                       */
    wg_uint  row_distance; /*!< distanse in bytes between rows         */
    wg_uint  components_per_pixel; /*!< number of components per pixel */
}Wg_image;


/** 
* @brief Image iterator
*/
typedef struct Img_iterator{
    wg_uint width;                /*!< width of the image              */
    wg_uint height;               /*!< height of the image             */
    wg_uint row_distance;         /*!< distance in bytes between rows  */
    wg_uchar **row;               /*!< array of rows                   */
    wg_uchar *col;                /*!< selected column                 */
    wg_uint col_index;            /*!< column index                    */
    wg_uint row_index;            /*!< row index                       */
    wg_uint comp_per_pixel;       /*!< components per pixel            */
}Img_iterator;

/**
 * @brief Get width of the image
 *
 * @param img    image structure
 * @param width  memory to store width
 *
 * @retval WG_SUCCESS
 * @retval CAM_FAILURE
 */
WG_INLINE wg_uint
img_get_width(const Wg_image *img, wg_uint *width)
{
    CHECK_FOR_NULL_PARAM(img);

    *width = img->width;

    return WG_SUCCESS;
}

/**
 * @brief Get height of the image
 *
 * @param img    image structure
 * @param height  memory to store height
 *
 * @retval WG_SUCCESS
 * @retval CAM_FAILURE
 */
WG_INLINE wg_uint
img_get_height(const Wg_image *img, wg_uint *height)
{
    CHECK_FOR_NULL_PARAM(img);

    *height = img->height;

    return WG_SUCCESS;
}

/**
 * @brief Get a pointer to a row
 *
 * @param img      image structure
 * @param row_num  row index to get pointer to
 * @param row      memory to store pointer
 *
 * @retval WG_SUCCESS
 * @retval CAM_FAILURE
 */
WG_INLINE wg_status
img_get_row(const Wg_image *img, wg_uint row_num, wg_uchar **row)
{
    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_RANGE_GE(row_num, img->height);

    *row = img->rows[row_num];

    return WG_SUCCESS;
}

/**
 * @brief Get a pointer to a pixel from a image
 *
 * @param img      image structure
 * @param row_off  row index
 * @param col_off  column index
 * @param pixel    memory to store a pointer to a pixel
 *
 * @return 
 */
WG_INLINE wg_status
img_get_pixel(Wg_image *img, wg_uint row_off, wg_uint col_off, 
        wg_uchar **pixel)
{
    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_NULL_PARAM(pixel);
    CHECK_FOR_RANGE_GE(row_off, img->height);
    CHECK_FOR_RANGE_GE(col_off, img->width);

    *pixel = img->rows[row_off] + (col_off * img->components_per_pixel);

    return WG_SUCCESS;
}

/**
 * @brief Get number of bytes berween rows
 *
 * @param img          image structure
 * @param row_distance memory to store distance
 *
 * @return WG_SUCCESS
 * @return CAM_FAILURE
 */
WG_INLINE wg_status
img_get_row_distance(Wg_image *img, wg_uint *row_distance)
{
    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_NULL_PARAM(row_distance);

    *row_distance = img->row_distance;

    return WG_SUCCESS;
}

/**
 * @brief Get number of components per pixel
 *
 * @param img    image structure
 * @param comp_per_pixel  memory to store number of components 
 *
 * @retval WG_SUCCESS
 * @retval CAM_FAILURE
 */
WG_INLINE wg_status
img_get_components_per_pixel(Wg_image *img, wg_uint *comp_per_pixel)
{
    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_NULL_PARAM(comp_per_pixel);

    *comp_per_pixel = img->components_per_pixel;

    return WG_SUCCESS;
}

/** 
* @brief Get image iterator
* 
* @param img image to get iterator for
* @param itr memory to store iterator
*/
WG_INLINE wg_status
img_get_iterator(Wg_image *img, Img_iterator *itr)
{
    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_NULL_PARAM(itr);

    itr->col_index = itr->row_index = 0;

    itr->row = img->rows;
    itr->col = 0;

    img_get_width(img, &itr->width);
    img_get_height(img, &itr->height);
    img_get_row_distance(img, &itr->row_distance);
    img_get_components_per_pixel(img, &itr->comp_per_pixel);

    return WG_SUCCESS;
}

/** 
* @brief Check if there are more rows
* 
* @param iterator iterator instance
* 
* @retval TRUE more rows available
* @retval FALSE no more rows
*/
WG_INLINE wg_boolean
img_iterator_has_next_row(Img_iterator *iterator)
{
    register Img_iterator *itr = iterator;

    return (itr->row_index < itr->height);
}

/** 
* @brief Get next row
* 
* @param iterator iterator instance
* 
* @return pointer on next row in a row or NULL if no more rows
*/
WG_INLINE wg_uchar *
img_iterator_next_row(Img_iterator *iterator)
{
    register Img_iterator *itr = iterator;

    itr->col_index = 0;

    if (itr->row_index++ < itr->height){
        itr->col = *itr->row++;
    }else{
        itr->col = NULL;
    }

    return itr->col;
}

/** 
* @brief Check if there are more columns
* 
* @param iterator iterator instance
* 
* @retval TRUE more columns available
* @retval FALSE no more columns
*/
WG_INLINE wg_boolean
img_iterator_has_next_col(Img_iterator *iterator)
{
    register Img_iterator *itr = iterator;

    return (itr->col_index < itr->width);
}

/** 
* @brief Get next column
* 
* @param iterator iterator instance
* 
* @return pointer on next column in a row or NULL if no more columns
*/
WG_INLINE wg_uchar *
img_iterator_next_col(Img_iterator *iterator)
{
    register Img_iterator *itr = iterator;
    register wg_uchar *old_col = itr->col;

    itr->col += itr->comp_per_pixel;

    return itr->col_index++ < itr->width ?  old_col : NULL;   
}

WG_INLINE wg_status
img_get_data(Wg_image *img, wg_uchar **data, wg_size *num, wg_size *size)
{
    CHECK_FOR_NULL_PARAM(data);
    CHECK_FOR_NULL_PARAM(num);
    CHECK_FOR_NULL_PARAM(size);

    *data = img->image;
    *num  = img->width * img->height;
    *size = img->components_per_pixel;

    return WG_SUCCESS;
}

wg_status
img_fill(wg_uint width, wg_uint height, wg_uint comp_num, img_type,
        Wg_image *img);

WG_PUBLIC wg_status
img_cleanup(Wg_image *img);

WG_PUBLIC wg_status
img_get_subimage(Wg_image *img_src, wg_uint x, wg_uint y, 
        Wg_image *img_dest);

wg_status
img_convert_to_pixbuf(Wg_image *img, GdkPixbuf **pixbuf,
        void (*free_cb)(guchar *, gpointer));

WG_PUBLIC wg_status
img_copy(Wg_image *src, Wg_image *dest);

WG_PUBLIC void
fast_memcpy(wg_uchar *restrict dest, wg_uchar *restrict src, 
        const wg_size size);

/*! @} */

#include "../image/include/img_bgrx.h"
#include "../image/include/img_draw.h"
#include "../image/include/img_gs.h"
#include "../image/include/img_hsv.h"
#include "../image/include/img_jpeg.h"
#include "../image/include/img_rgb24.h"
#include "../image/include/img_yuyv.h"

#endif
