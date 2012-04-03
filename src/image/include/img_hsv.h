#ifndef _CAM_HSV_H
#define _CAM_HSV_H

/** 
* @brief HSV color representation
*/
typedef struct Hsv{
    wg_double hue;      /*!< hue        */
    wg_double sat;      /*!< Struration */
    wg_double val;      /*!< value      */
}Hsv;

#define HSV_COMPONENT_NUM (sizeof (Hsv))

WG_PUBLIC wg_status
img_hsv_filter(const Wg_image *img, Wg_image *filtered_img, va_list args);

WG_PUBLIC wg_status
img_hsv_hist(Wg_image *img, wg_uint **h, wg_uint **s, wg_uint **v,
                  wg_size *hs, wg_size *ss, wg_size *vs);

WG_PUBLIC wg_status
img_hsv_median_filter(Wg_image *img, Wg_image *new_img);

#endif /* _CAM_HSV_H */
