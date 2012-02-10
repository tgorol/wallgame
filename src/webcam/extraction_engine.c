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

#define NB_X   6
#define NB_Y   6

#define NB_X2   4
#define NB_Y2   4

#define FPPOS   4
#define FPPOS_M ((FPPOS) >> 1) 
#define FPPOS_MAX (1 << FPPOS)

#define FPPOS_INC(val)  ((val) += (FPPOS_VAL(1)))
#define FPPOS_VAL(val)  ((wg_int32)(((wg_uint32)(val)) << FPPOS))
#define FPPOS_MUL(val1, val2)                  \
                    (FPPOS_INT((val1) * (val2)))

#define FPPOS_DIV(num, dnum)                  \
                    ((num / ((dnum) >> (FPPOS_M))) << (FPPOS_M))

/*! @todo handle negative numbers properly */
#define FPPOS_INT(val)  ((wg_int32)(((wg_int32)(val)) >> FPPOS))

static wg_int32 tan_c_array[2 * NB_X + 1][2 * NB_Y + 1];

static wg_int32 tan_cache[CACHE_TAN_NUM];

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

wg_status
ef_threshold(Wg_image *img, gray_pixel value)
{
    gray_pixel *gs_pixel = NULL;
    Cam_img_iterator itr;

    CHECK_FOR_NULL_PARAM(img);

    if (img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_iterator(img, &itr);

    while (cam_img_iterator_next_row(&itr) != NULL){
        while ((gs_pixel = cam_img_iterator_next_col(&itr)) != NULL){
            *gs_pixel = *gs_pixel >= value ? 255 : 0;
        }
    }

    return WG_SUCCESS;
}


WG_PRIVATE wg_status
detect_circle(Wg_image *img, Wg_image *acc, wg_int y1, wg_int x1, 
        wg_uint nb_x, wg_uint nb_y)
{
    gray_pixel *gs_pixel = NULL;
    wg_uint *acc_pixel = NULL;
    wg_int x2 = 0;
    wg_int y2 = 0;
    wg_int x0 = 0;
    wg_int y0 = 0;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_int xm = 0;
    wg_int ym = 0;
    wg_int m = 0;

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    x1     = FPPOS_VAL(x1);;
    y1     = FPPOS_VAL(y1);
    nb_x   = FPPOS_VAL(nb_x);
    nb_y   = FPPOS_VAL(nb_y);
    width  = FPPOS_VAL(width);
    height = FPPOS_VAL(height);

    for (x2 = x1 - nb_x; x2 < x1 + nb_x; FPPOS_INC(x2)){
        for (y2 = y1 - nb_y; y2 < y1 + nb_y; FPPOS_INC(y2)){
            if ((abs(x2 - x1) > FPPOS_VAL(NB_X2)) | 
                    (abs(y2 - y1) > FPPOS_VAL(NB_Y2))){
                if ((x2 > 0) && (y2 > 0) && (x2 < width) && (y2 < height)){
                    cam_img_get_pixel(img, FPPOS_INT(y2), FPPOS_INT(x2),
                            (wg_uchar**)&gs_pixel);
                    if (*gs_pixel == 255){
                        xm = (x1 + x2) >> 1;
                        ym = (y1 + y2) >> 1;
                        m = tan_c_array[FPPOS_INT(x2 - x1) + NB_X]
                            [FPPOS_INT(y2 - y1) + NB_Y];
                        if ((m > FPPOS_VAL(-1)) && (m < FPPOS_VAL(1))){
                            for (x0 = 0; x0 < width; FPPOS_INC(x0)){
                                y0 = ym + FPPOS_MUL(m, (xm - x0));
                                if ((y0 > 0) && (y0 < height)){
                                    cam_img_get_pixel(acc,
                                            FPPOS_INT(y0), FPPOS_INT(x0), 
                                            (wg_uchar**)&acc_pixel);
                                    ++*acc_pixel;
                                }
                            }
                        }else{
                            for (y0 = 0; y0 < height; FPPOS_INC(y0)){
                                x0 = xm + FPPOS_DIV((ym - y0), m);
                                if ((x0 > 0) && (x0 < width)){
                                    cam_img_get_pixel(acc, 
                                            FPPOS_INT(y0), FPPOS_INT(x0),
                                            (wg_uchar**)&acc_pixel);
                                    ++*acc_pixel;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return WG_SUCCESS;
}

wg_status
ef_detect_circle(Wg_image *img, Wg_image *acc)
{
    cam_status status = CAM_FAILURE;
    gray_pixel *gs_pixel = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;

    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_NULL_PARAM(acc);

    if (img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    status = cam_img_fill(width, height, sizeof (wg_uint), IMG_CIRCLE_ACC,
            acc);
    if (CAM_SUCCESS != status){
        return CAM_FAILURE;
    }

    for (row = 0; row < height; ++row){
        cam_img_get_row(img, row, (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; ++col, ++gs_pixel){
            if (*gs_pixel == 255){
                detect_circle(img, acc, row, col, NB_X, NB_Y);
            }
        }
    }

    return CAM_SUCCESS;
}

cam_status
ef_paint_cross(Wg_image *img, wg_uint y, wg_uint x, gray_pixel color)
{
    wg_uint height = 0;
    wg_uint row = 0;
    gray_pixel *gs_pixel = NULL;

    cam_img_get_height(img, &height);

    ef_paint_line(img, 0, y, color);

    for (row = 0; row < height; ++row){
        cam_img_get_row(img, row, (wg_uchar**)&gs_pixel);
        gs_pixel[x] = color;
    }

    return CAM_SUCCESS;
}

cam_status
ef_acc_get_max(Wg_image *acc, wg_uint *row_par, wg_uint *col_par)
{
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0 ;
    wg_uint *acc_pixel = NULL;
    wg_uint max_value = 0;


    CHECK_FOR_NULL_PARAM(acc);
    CHECK_FOR_NULL_PARAM(row_par);
    CHECK_FOR_NULL_PARAM(col_par);

    if (acc->type != IMG_CIRCLE_ACC){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                acc->type, IMG_CIRCLE_ACC);
        return CAM_FAILURE;
    }

    cam_img_get_width(acc, &width);
    cam_img_get_height(acc, &height);

    for (row = 0; row < height; ++row){
        cam_img_get_row(acc, row, (wg_uchar**)&acc_pixel);
        for (col = 0; col < width; ++col, ++acc_pixel){
            if (*acc_pixel > max_value){
                max_value = *acc_pixel;
                *col_par = col;
                *row_par = row;
            }
        }
    }

    return CAM_SUCCESS;
}

cam_status
ef_acc_save(Wg_image *acc, wg_char *filename, wg_char *type)
{
    cam_status status = WG_FAILURE;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;
    gray_pixel *gs_pixel = NULL;
    wg_uint *acc_pixel = NULL;
    wg_uint max_val = 0;
    Wg_image acc_gs;

    CHECK_FOR_NULL_PARAM(acc);
    CHECK_FOR_NULL_PARAM(filename);
    CHECK_FOR_NULL_PARAM(type);

    if (acc->type != IMG_CIRCLE_ACC){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                acc->type, IMG_CIRCLE_ACC);
        return CAM_FAILURE;
    }

    cam_img_get_width(acc, &width);
    cam_img_get_height(acc, &height);

    status = cam_img_fill(width, height, GS_COMPONENT_NUM, IMG_GS,
            &acc_gs);
    if (CAM_SUCCESS != status){
        return CAM_FAILURE;
    }

    for (row = 0; row < height; ++row){
        cam_img_get_row(acc, row, (wg_uchar**)&acc_pixel);
        for (col = 0; col < width; ++col, ++acc_pixel){
            max_val = WG_MAX(max_val, *acc_pixel);
        }
    }

    for (row = 0; row < height; ++row){
        cam_img_get_row(acc, row, (wg_uchar**)&acc_pixel);
        cam_img_get_row(&acc_gs, row, (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; ++col, ++gs_pixel, ++acc_pixel){
            *gs_pixel = (255.0 * (*acc_pixel) / max_val);
        }
    }

    cam_img_grayscale_save(&acc_gs, filename, type);

    cam_img_cleanup(&acc_gs);

    return CAM_SUCCESS;

}

wg_status
ef_smooth(Wg_image *img, Wg_image *new_img)
{
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;
    gray_pixel *gs_pixel = NULL;
    gray_pixel *gs_new_pixel = NULL;
    wg_int rd = 0;
    wg_int rd2 = 0;
    wg_int rd3 = 0;
    wg_int rd4 = 0;

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
    rd2 = rd + rd;
    rd3 = rd2 + rd;
    rd4 = rd3 + rd;

    for (row = 0; row < height - 4; ++row){
        cam_img_get_row(img, row, (wg_uchar**)&gs_pixel);
        cam_img_get_row(new_img, row, (wg_uchar**)&gs_new_pixel);
        for (col = 0; col < width - 4; ++col, ++gs_pixel, ++gs_new_pixel){
            *gs_new_pixel = 
                (
                 /* 1st row */
                 gs_pixel[0] + gs_pixel[1] + 
                 gs_pixel[2] + gs_pixel[3] + 
                 gs_pixel[4] +
                 /* 2nd row */
                 gs_pixel[rd + 0] + gs_pixel[rd + 1] + 
                 gs_pixel[rd + 2] + gs_pixel[rd + 3] + 
                 gs_pixel[rd + 4] +
                 /* 3rd row */
                 gs_pixel[rd2 + 0] + gs_pixel[rd2 + 1] + 
                 gs_pixel[rd2 + 2] + gs_pixel[rd2 + 3] + 
                 gs_pixel[rd2 + 4] +
                 /* 4th row */
                 gs_pixel[rd3 + 0] + gs_pixel[rd3 + 1] + 
                 gs_pixel[rd3 + 2] + gs_pixel[rd3 + 3] + 
                 gs_pixel[rd3 + 4] +
                 /* 5th row */
                 gs_pixel[rd4 + 0] + gs_pixel[rd4 + 1] + 
                 gs_pixel[rd4 + 2] + gs_pixel[rd4 + 3] + 
                 gs_pixel[rd4 + 4])/ 25;

        }
    }

    return CAM_SUCCESS;
}

wg_status
ef_detect_edge(Wg_image *img, Wg_image *new_img)
{
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;
    gray_pixel *gs_pixel = NULL;
    gray_pixel *gs_new_pixel = NULL;
    wg_uint rd = 0;
    wg_uint rd2 = 0;

    CHECK_FOR_NULL_PARAM(img);
    CHECK_FOR_NULL_PARAM(new_img);

    if (img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);
    cam_img_get_row_distance(img, &rd);

    rd2 = rd + rd;

    cam_img_fill(width - 2, height - 2, GS_COMPONENT_NUM, IMG_GS,
            new_img);

    for (row = 0; row < height - 2; ++row){
        cam_img_get_row(img, row, (wg_uchar**)&gs_pixel);
        cam_img_get_row(new_img, row, (wg_uchar**)&gs_new_pixel);
        for (col = 0; col < width - 2; ++col, ++gs_pixel, ++gs_new_pixel){
            *gs_new_pixel = WG_MAX(
                    abs(gs_pixel[0] - gs_pixel[2] + 
                        (gs_pixel[rd] << 1) - (gs_pixel[rd + 2] << 1) +
                        gs_pixel[rd2] - gs_pixel[rd2 + 2]),
                    abs(gs_pixel[0] + (gs_pixel[1] << 1) + gs_pixel[2] -
                        gs_pixel[rd2] - (gs_pixel[rd2 + 1] << 1) - 
                        gs_pixel[rd2 + 2])
                    );
        }
    }

    return CAM_SUCCESS;
}

WG_PRIVATE wg_boolean
hist_check(Wg_image *img, wg_uint row, wg_uint col)
{
    wg_uint width = 0;
    wg_uint height = 0;

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    return (col >= 1) && (col < width - 2) && (row >= 1) && (row < height - 2);
}

WG_PRIVATE wg_status
hist_connect(Wg_image *img, wg_uint row, wg_uint col, wg_uint low)
{
    gray_pixel *gs_pixel = NULL;
    wg_uint x1 = 0;
    wg_uint y1 = 0;
    gray_pixel pix = 0;

    for (x1 = col - 1; x1 <= col + 1; ++x1){
        for (y1 = row - 1; y1 <= row + 1; ++y1, ++gs_pixel){
            cam_img_get_pixel(img, y1, x1, (wg_uchar**)&gs_pixel);
            pix = *gs_pixel;
            if ((pix >= low) && (pix != 255) && hist_check(img, y1, x1)){
                *gs_pixel = 255;
                hist_connect(img, y1, x1, low);
            }
        }
    }

    return WG_SUCCESS;
}

wg_status
ef_hyst_thr(Wg_image *img, wg_uint upp, wg_uint low)
{
    gray_pixel *gs_pixel = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint row = 0;
    wg_uint col = 0;
    gray_pixel pix = 0;

    CHECK_FOR_NULL_PARAM(img);

    if (img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    for (row = 1; row < height - 2; ++row){
        cam_img_get_row(img, row, (wg_uchar**)&gs_pixel);
        for (col = 1; col < width - 2; ++col, ++gs_pixel){
            pix = *gs_pixel;
            if ((pix >= upp) && (pix !=255)){
                *gs_pixel = 255;
                hist_connect(img, row, col, low);
            }
        }
    }

    return WG_SUCCESS;
}

wg_status
ef_paint_pixel(Wg_image *img, wg_int x, wg_int y, gray_pixel value)
{
    gray_pixel *gs_pixel = NULL;
    wg_uint width = 0;
    wg_uint height = 0;

    CHECK_FOR_NULL_PARAM(img);

    if (img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_GS);
        return CAM_FAILURE;
    }

    cam_img_get_width(img, &width);
    cam_img_get_height(img, &height);

    if ((x < width) && (y < height) && (x >= 0) && (y >= 0)){
        cam_img_get_pixel(img, y, x, &gs_pixel);
        *gs_pixel = value;
    }

    return WG_SUCCESS;

}

wg_status
ef_paint_line(Wg_image *img, wg_float m, wg_uint c, gray_pixel value)
{
    wg_uint width = 0;
    wg_uint height = 0;
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

wg_status
ef_hough_print_acc(Wg_image *img, acc *width_acc)
{
    wg_uint width = 0;
    wg_uint height = 0;
    wg_uint c = 0;
    wg_uint m = 0;
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

wg_status
ef_hough_paint_long_lines(Wg_image *img, acc *width_acc, acc *height_acc)
{
    wg_uint width = 0;
    wg_uint height = 0;

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

    ef_paint_line(img, tan_cache[max_m] / WG_FLOAT(FPPOS_MAX), max_c, 128);

    return WG_SUCCESS;
}

wg_status
ef_hough_paint_lines(Wg_image *img, acc *width_acc, acc *height_acc, wg_uint value)
{
    wg_uint width = 0;
    wg_uint height = 0;

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

wg_status
ef_hough_lines(Wg_image *img, acc **width_acc, acc **height_acc)
{
    acc *acc_col = NULL;
    acc *acc_row = NULL;
    gray_pixel *gs_pixel = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_int row = 0;
    wg_int col = 0;
    wg_int  angle = 0; 
    wg_int b = 0;

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

    width = FPPOS_VAL(width);
    height = FPPOS_VAL(height);

    for (row = 0; row < height; FPPOS_INC(row)){
        cam_img_get_row(img, FPPOS_INT(row), (wg_uchar**)&gs_pixel);
        for (col = 0; col < width; FPPOS_INC(col), ++gs_pixel){
            if (*gs_pixel != 0){
                for (angle = -45; angle < 45; angle += 1){
                    b = row - FPPOS_MUL(tan_cache[angle + 45], col);
                    if ((b < height) && (b > 0)){
                        ++acc_row[FPPOS_INT(b)][angle + 45];
                    }
                }
                for (angle = 45; angle < 135; angle += 1){
                    b = col - FPPOS_DIV(row, tan_cache[angle + 45]);
                    if ((b < width) && (b > 0)){
                        ++acc_col[FPPOS_INT(b)][angle - 45];
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
    wg_int x = 0;
    wg_int y = 0;
    wg_int i = 0;
    wg_int tg = 0;
    wg_float val = WG_FLOAT(val);

    /* cache tangents for line detector */
    for (i = 0; i < CACHE_TAN_NUM; ++i){
        val = tan(((i - 45) * M_PI) / 180.0) * WG_FLOAT(FPPOS_MAX);
        tan_cache[i] = val;

        /* if tangent equals 0 change to the smallest possible to 
         * avoid dividing by 0
         */
        if (tan_cache[i] == 0){
            tan_cache[i] = 1;
        }
    }

    /* cache tangents for circle detector */
    for (x = -NB_X ; x <= NB_X; ++x){
        for (y = -NB_Y ; y <= NB_Y; ++y){
            if (y != 0){
                tg = (WG_FLOAT(x) * WG_FLOAT(FPPOS_MAX) / WG_FLOAT(y));
            }else{
                tg = FPPOS_VAL(9999);
            }
            tan_c_array[x + NB_X][y + NB_Y] = tg;
        }
    }

    return;
}
