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

/** @brief Get RED compontent of the pixel
 * @todo change names to RGB_R, RGB_G RGB_B
 */
#define RGB24_PIXEL_RED(pixel)   ((pixel)[RGB24_R])

/** @brief Get GREEN component of the pixel
 */
#define RGB24_PIXEL_GREEN(pixel) ((pixel)[RGB24_G])

/** @brief Get BLUE component of the pixel
 */
#define RGB24_PIXEL_BLUE(pixel)  ((pixel)[RGB24_B])

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

    
WG_PUBLIC wg_status
img_rgb_draw_pixel(Wg_image *img, wg_int y, wg_int x, va_list args);

WG_PUBLIC cam_status
img_rgb_2_hsv(Wg_image *rgb_img, Wg_image *hsv_img);

WG_PUBLIC cam_status
img_rgb_2_hsv_fast(Wg_image *rgb_img, Wg_image *hsv_img);

WG_PUBLIC cam_status
img_rgb_2_bgrx(Wg_image *rgb_img, Wg_image *bgrx_img);

WG_PUBLIC cam_status
img_rgb_2_hsv_gtk(Wg_image *rgb_img, Wg_image *hsv_img);

WG_PUBLIC wg_status
img_rgb_2_grayscale(Wg_image *rgb_img, Wg_image *grayscale_img);

WG_PUBLIC cam_status
img_rgb_from_buffer(wg_uchar *buffer, wg_uint width, wg_uint height, 
        Wg_image *img);

WG_PUBLIC wg_status
img_rgb_median_filter(Wg_image *img, Wg_image *new_img);

WG_PUBLIC cam_status
img_bgrx_2_rgb(Wg_image *bgrx_img, Wg_image *rgb_img);

#endif

