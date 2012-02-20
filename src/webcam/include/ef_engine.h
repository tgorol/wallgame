#ifndef EF_ENGINE_H
#define EF_ENGINE_H

#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>

typedef wg_uint acc[90];



#define IMG_CIRCLE_ACC    (IMG_USER + 1)

WG_PUBLIC wg_status
ef_detect_edge(Wg_image *img, Wg_image *new_img);

WG_PUBLIC wg_status
ef_smooth(Wg_image *img, Wg_image *new_img);

WG_PUBLIC wg_status
ef_init(void);

WG_PUBLIC wg_status
ef_hough_lines(Wg_image *img, acc **width_acc, acc **height_acc);

WG_PUBLIC wg_status
ef_hough_paint_lines(Wg_image *img, acc *width_acc, acc *height_acc, wg_uint value);

WG_PUBLIC wg_status
ef_hough_paint_long_lines(Wg_image *img, acc *width_acc, acc *height_acc);

WG_PUBLIC wg_status
ef_hough_print_acc(Wg_image *img, acc *width_acc);

WG_PUBLIC wg_status
ef_threshold(Wg_image *img, gray_pixel value);

WG_PUBLIC wg_status
ef_detect_circle(Wg_image *img, Wg_image *acc);

WG_PUBLIC cam_status
ef_acc_save(Wg_image *acc, wg_char *filename, wg_char *type);

WG_PUBLIC wg_status
ef_hyst_thr(Wg_image *img, wg_uint upp, wg_uint low);

WG_PUBLIC cam_status
ef_acc_2_gs(Wg_image *acc, Wg_image *acc_gs);

#endif /* EF_ENGINE_H */