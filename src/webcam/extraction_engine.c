#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/cam_img.h"
#include "include/cam_img_grayscale.h"
#include "include/cam_img_bgrx.h"
#include "include/cam_img_rgb.h"
#include "include/extraction_engine.h"

#define   CACHE_TAN_NUM (135 + 45)


static wg_float  tan_cache[CACHE_TAN_NUM];

static wg_boolean ef_init_flag = WG_FALSE;

WG_PRIVATE void init_tan_cache(void);

wg_status
ef_init(void)
{
    wg_status status = WG_FAILURE;

    if (ef_init_flag == WG_FALSE){
        init_tan_cache();
    }

    return status;
}

cam_status
ef_threshold(Wg_image *img, gray_pixel value)
{
    wg_uint width;
    wg_uint height;
    wg_uint row;
    wg_uint col;
    gray_pixel *gs_pixel;

    CHECK_FOR_NULL_PARAM(img);

    if (img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    for (row = 0; row < height; ++row){
        cam_img_get_row(img, row, (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; ++col, ++gs_pixel){
            *gs_pixel = *gs_pixel > value ? 255 : 0;
        }
    }

    return WG_SUCCESS;
}

cam_status
ef_smooth(Wg_image *img, Wg_image *new_img)
{
    wg_uint width;
    wg_uint height;
    wg_uint row;
    wg_uint col;
    gray_pixel *gs_pixel;
    gray_pixel *gs_new_pixel;
    wg_int rd = 0;

    CHECK_FOR_NULL_PARAM(img);

    if (img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    cam_img_fill(width - 4, height - 4, GS_COMPONENT_NUM, IMG_GS,
            new_img);

    rd = img->row_distance;

    for (row = 0; row < height - 4; ++row){
        cam_img_get_row(img, row, (wg_uchar**)&gs_pixel);
        cam_img_get_row(new_img, row, (wg_uchar**)&gs_new_pixel);
        for (col = 0; col < width - 4; ++col, ++gs_pixel, ++gs_new_pixel){
            *gs_new_pixel = 
                (gs_pixel[0] + gs_pixel[1] + gs_pixel[2] + gs_pixel[3] + gs_pixel[4] +
                 gs_pixel[rd] + gs_pixel[rd + 1] + gs_pixel[rd + 2] + gs_pixel[rd + 3] + gs_pixel[rd + 4] +
                 gs_pixel[rd + rd] + gs_pixel[rd + rd + 1] + gs_pixel[rd + rd + 2] + gs_pixel[rd + rd + 3] + gs_pixel[rd + rd + 4]) / 25;

        }
    }

    return CAM_SUCCESS;
}

cam_status
ef_detect_edge(Wg_image *img, Wg_image *new_img)
{
    wg_uint width;
    wg_uint height;
    wg_uint row;
    wg_uint col;
    gray_pixel *gs_pixel;
    gray_pixel *gs_new_pixel;

    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_NULL_PARAM(new_img);

    if (img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    cam_img_fill(width - 1, height - 1, GS_COMPONENT_NUM, IMG_GS,
            new_img);

    for (row = 0; row < height - 1; ++row){
        cam_img_get_row(img, row, (wg_uchar**)&gs_pixel);
        cam_img_get_row(new_img, row, (wg_uchar**)&gs_new_pixel);
        for (col = 0; col < width - 1; ++col, ++gs_pixel, ++gs_new_pixel){
            *gs_new_pixel = WG_MAX(
                    abs(gs_pixel[0] - gs_pixel[img->row_distance + 1]),
                    abs(gs_pixel[1] - gs_pixel[img->row_distance - 1])
                    );
        }
    }

    return CAM_SUCCESS;
}

wg_status
ef_paint_pixel(Wg_image *img, wg_int x, wg_int y, gray_pixel value)
{
    wg_uint width;
    wg_uint height;
    gray_pixel *gs_pixel;

    CHECK_FOR_NULL_PARAM(img);

    if (img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    if ((x < width) && (y < height) && (x >= 0) && (y >= 0)){
        cam_img_get_pixel(img, x, y, &gs_pixel);
        *gs_pixel = value;
    }

    return WG_SUCCESS;

}

wg_status
ef_paint_line(Wg_image *img, wg_float m, wg_uint c, gray_pixel value)
{
    wg_uint width;
    wg_uint height;
    wg_int ypos = 0;
    wg_int xpos = 0;

    CHECK_FOR_NULL_PARAM(img);

    if (img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    for (xpos = 0; xpos < width; ++xpos){
        ypos = m * xpos + c;
        ef_paint_pixel(img, xpos, ypos, value);
    }

    return WG_SUCCESS;
}

cam_status
ef_hough_print_acc(Wg_image *img, acc *width_acc)
{
    wg_uint width;
    wg_uint height;
    wg_uint c, m;
    FILE *f = NULL;

    f = fopen("log.out", "w");

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    for (c = 0; c < height; ++c){
        fprintf(f, "%3d |", c);
        for (m = 0; m < 90; ++m){
            fprintf(f, "%u ", width_acc[c][m]);
        }
        fprintf(f, "\n");
    }

    fclose(f);

    return WG_SUCCESS;
}

cam_status
ef_hough_paint_long_lines(Wg_image *img, acc *width_acc, acc *height_acc)
{
    wg_uint width;
    wg_uint height;

    wg_uint max_c = 0;
    wg_uint max_m = 0;

    wg_uint c = 0;
    wg_uint m = 0;

    wg_uint max_value = 0;

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    for (c = 0; c < height; ++c){
        for (m = 0; m < 90; ++m){
            if (max_value < height_acc[c][m]){
                max_c = c;
                max_m = m;
                max_value = height_acc[c][m];
            }
        }
    }

    ef_paint_line(img, tan_cache[max_m], max_c, 128);

    return WG_SUCCESS;
}

cam_status
ef_hough_paint_lines(Wg_image *img, acc *width_acc, acc *height_acc, wg_uint value)
{
    wg_uint width;
    wg_uint height;

    wg_uint c = 0;
    wg_uint m = 0;

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    for (c = 0; c < height; ++c){
        for (m = 0; m < 90; ++m){
            if (value < height_acc[c][m]){
                ef_paint_line(img, tan_cache[m], c, 128);
            }
        }
    }

    return WG_SUCCESS;
}

cam_status
ef_hough_lines(Wg_image *img, acc **width_acc, acc **height_acc)
{
    wg_uint width;
    wg_uint height;
    wg_uint row;
    wg_uint col;
    gray_pixel *gs_pixel;
    wg_int  angle = 0; 
    wg_int b = 0;
    acc *acc_col = NULL;
    acc *acc_row = NULL;

    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_NULL_PARAM(width_acc);
    CHECK_FOR_NULL_PARAM(height_acc);

    if (img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    acc_row = WG_CALLOC(height, sizeof (acc));
    acc_col = WG_CALLOC(width, sizeof (acc));

    for (row = 0; row < height; ++row){
        cam_img_get_row(img, row, (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; ++col, ++gs_pixel){
            if (*gs_pixel != 0){
                for (angle = -45; angle < 45; angle += 2){
                    b = (wg_int)(row - tan_cache[angle + 45] * col);
                    if ((b < height) && (b > 0)){
                        ++acc_row[b][angle + 45];
                    }
                }
                for (angle = 45; angle < 135; angle += 2){
                    b = (wg_int)(col - row / tan_cache[angle + 45]);
                    if ((b < width) && (b > 0)){
                        ++acc_col[b][angle - 45];
                    }
                }
            }
        }
    }

    *height_acc = acc_row;
    *width_acc  = acc_col;

    return WG_SUCCESS;
}

    WG_PRIVATE void 
init_tan_cache(void)
{
    wg_int i = 0;

    for (i = 0; i < CACHE_TAN_NUM; ++i){
        tan_cache[i] = tan(((i - 45) * M_PI) / 180.0);
    }

    return;
}
