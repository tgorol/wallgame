#ifndef _CAM_IMG_GTAYSCALE_H
#define _CAM_IMG_GTAYSCALE_H


/* 8 means bits in 1 bytes */
#define  GS_PIXEL_MAX    ((sizeof(gray_pixel) << 8) - 1)
#define  GS_PIXEL_MIN    (0)

#define GS_COMPONENT_NUM   (sizeof(gray_pixel))


wg_status
img_gs_draw_pixel(Wg_image *img, wg_int y, wg_int x, va_list color);

#endif

