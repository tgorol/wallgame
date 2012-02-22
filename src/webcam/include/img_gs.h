#ifndef _CAM_IMG_GTAYSCALE_H
#define _CAM_IMG_GTAYSCALE_H

wg_status
img_gs_draw_pixel(Wg_image *img, wg_int y, wg_int x, va_list color);

wg_status
img_gs_sub(Wg_image *img_1, Wg_image *img_2);

#endif

