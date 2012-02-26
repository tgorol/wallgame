#ifndef _CAM_IMG_GTAYSCALE_H
#define _CAM_IMG_GTAYSCALE_H

/* 8 means bits in 1 bytes */

/** Maximum value of gray scale pixel */
#define  GS_PIXEL_MAX    ((sizeof(gray_pixel) << 8) - 1)

/** Minimum value of gray scale pixel */
#define  GS_PIXEL_MIN    (0)

/** 
* @brief Gray scale pixel components layout
*/
enum {
    GS_PIXEL,           /*!< pixel value    */
    GS_COMPONENT_NUM    /*!< number of components in gray scale pixel format */
};


WG_PUBLIC wg_status
img_gs_draw_pixel(Wg_image *img, wg_int y, wg_int x, va_list color);

WG_PUBLIC wg_status
img_gs_sub(Wg_image *img_1, Wg_image *img_2);

WG_PUBLIC wg_status
img_gs_2_rgb(Wg_image *grayscale_img, Wg_image *rgb_img);

WG_PUBLIC wg_status
img_gs_max_min(Wg_image* grayscale_img, gray_pixel *gs_max,
        gray_pixel *gs_min);

WG_PUBLIC wg_status
img_gs_normalize(Wg_image* grayscale_img, gray_pixel new_max,
        gray_pixel new_min);

WG_PUBLIC cam_status
img_gs_histogram(Wg_image* grayscale_img, wg_uint *histogram, 
        wg_size size);

WG_PUBLIC cam_status
img_gs_save(Wg_image *img, wg_char *filename, wg_char *ext);


#endif

