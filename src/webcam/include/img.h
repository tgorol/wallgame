#ifndef _CAM_IMG_H
#define _CAM_IMG_H

/*! @addtogroup image
 * @{
 */


typedef struct Wg_rgb{
    wg_uchar red;
    wg_uchar green;
    wg_uchar blue;
}Wg_rgb;

typedef struct Img_iterator{
    wg_uint width;
    wg_uint height;
    wg_uint row_distance;
    wg_uchar **row;
    wg_uchar *col;
    wg_uint col_index;
    wg_uint row_index;
    wg_uint comp_per_pixel;
}Img_iterator;

/**
 * @brief Get width of the image
 *
 * @param img    image structure
 * @param width  memory to store width
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
WG_INLINE wg_uint
img_get_width(Wg_image *img, wg_uint *width)
{
    CHECK_FOR_NULL_PARAM(img);

    *width = img->width;

    return CAM_SUCCESS;
}

/**
 * @brief Get haight of the image
 *
 * @param img    image structure
 * @param height  memory to store height
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
WG_INLINE wg_uint
img_get_height(Wg_image *img, wg_uint *height)
{
    CHECK_FOR_NULL_PARAM(img);

    *height = img->height;

    return CAM_SUCCESS;
}

/**
 * @brief Get a pointer to a row
 *
 * @param img      image structure
 * @param row_num  row index to get pointer to
 * @param row      memory to store pointer
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
WG_INLINE cam_status
img_get_row(Wg_image *img, wg_uint row_num, wg_uchar **row)
{
    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_RANGE_GE(row_num, img->height);

    *row = img->rows[row_num];

    return CAM_SUCCESS;
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
WG_INLINE cam_status
img_get_pixel(Wg_image *img, wg_uint row_off, wg_uint col_off, 
        wg_uchar **pixel)
{
    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_NULL_PARAM(pixel);
    CHECK_FOR_RANGE_GE(row_off, img->height);
    CHECK_FOR_RANGE_GE(col_off, img->width);

    *pixel = img->rows[row_off] + (col_off * img->components_per_pixel);

    return CAM_SUCCESS;
}

/**
 * @brief Get number of bytes berween rows
 *
 * @param img          image structure
 * @param row_distance memory to store distance
 *
 * @return CAM_SUCCESS
 * @return CAM_FAILURE
 */
WG_INLINE cam_status
img_get_row_distance(Wg_image *img, wg_uint *row_distance)
{
    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_NULL_PARAM(row_distance);

    *row_distance = img->row_distance;

    return CAM_SUCCESS;
}

/**
 * @brief Get number of components per pixel
 *
 * @param img    image structure
 * @param comp_per_pixel  memory to store number of components 
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
WG_INLINE cam_status
img_get_components_per_pixel(Wg_image *img, wg_uint *comp_per_pixel)
{
    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_NULL_PARAM(comp_per_pixel);

    *comp_per_pixel = img->components_per_pixel;

    return CAM_SUCCESS;
}

WG_INLINE void 
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

    return;

}

WG_INLINE wg_boolean
img_iterator_has_next_row(Img_iterator *iterator)
{
    register Img_iterator *itr = iterator;

    return (itr->row_index < itr->height);
}

WG_INLINE wg_uchar *
img_iterator_next_row(Img_iterator *iterator)
{
    register Img_iterator *itr = iterator;

    itr->col_index = 0;
    itr->col = *itr->row;

    return itr->row_index++ < itr->height ? *itr->row++ : NULL;   
}

WG_INLINE wg_boolean
img_iterator_has_next_col(Img_iterator *iterator)
{
    register Img_iterator *itr = iterator;

    return (itr->col_index < itr->width);
}

WG_INLINE wg_uchar *
img_iterator_next_col(Img_iterator *iterator)
{
    register Img_iterator *itr = iterator;
    register wg_uchar *old_col = itr->col;

    itr->col += itr->comp_per_pixel;

    return itr->col_index++ < itr->width ?  old_col : NULL;   
}


/** @brief Get RED compontent of the pixel
 * @todo change names to RGB_R, RGB_G RGB_B
 */
#define PIXEL_RED(pixel)   (pixel)[RGB24_R]

/** @brief Get GREEN component of the pixel
 */
#define PIXEL_GREEN(pixel) (pixel)[RGB24_G]

/** @brief Get BLUE component of the pixel
 */
#define PIXEL_BLUE(pixel)  (pixel)[RGB24_B]

cam_status
img_fill(wg_uint width, wg_uint height, wg_uint comp_num, img_type,
        Wg_image *img);

WG_PUBLIC cam_status
img_cleanup(Wg_image *img);

WG_PUBLIC cam_status
img_get_subimage(Wg_image *img_src, wg_uint x, wg_uint y, 
        Wg_image *img_dest);

WG_PUBLIC cam_status
img_filter_color_threshold(const Wg_image *img, const Wg_rgb *base, 
        const Wg_rgb *threshold, const Wg_rgb *background, 
        const Wg_rgb *foreground);

/*! @} */

#endif