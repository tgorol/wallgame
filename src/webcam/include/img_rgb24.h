#ifndef CAM_IMG_RGB_H
#define CAM_IMG_RGB_H

/**
 * @brief Position of a color component in pixel
 */
enum RGB24_PIXEL{
    RGB24_R = 0 ,    /*!< Red component index   */
    RGB24_G     ,    /*!< Green component index */
    RGB24_B     ,    /*!< Blue component index  */
    RGB24_COMPONENT_NUM   ,    /*!< Number of components  */
};

/**
 * @brief Pixel type
 */
typedef wg_uchar rgb24_pixel[RGB24_COMPONENT_NUM];

WG_INLINE void
bgrx_2_rgb(bgrx_pixel bgrx, rgb24_pixel rgb)
{
    rgb[RGB24_R] = BGRX_R(bgrx);
    rgb[RGB24_G] = BGRX_G(bgrx);
    rgb[RGB24_B] = BGRX_B(bgrx);

    return;
}

    


#endif

