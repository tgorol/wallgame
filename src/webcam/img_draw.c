#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/img.h"
#include "include/img_gs.h"
#include "include/img_bgrx.h"
#include "include/img_rgb24.h"
#include "include/extraction_engine.h"

#include "include/img_draw.h"

WG_PUBLIC wg_status
img_draw_get_context(img_type type, Img_draw *draw_ctx)
{
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL_PARAM(draw_ctx);

    switch (type){
    case IMG_GS:
        draw_ctx->draw_pixel = (put_pix)img_gs_draw_pixel;
        break;
    default:
        status = WG_FAILURE;
    }

    return status;
}

WG_PUBLIC wg_status
img_draw_cleanup_context(Img_draw *draw_ctx)
{
    CHECK_FOR_NULL_PARAM(draw_ctx);

    WG_ZERO_STRUCT(draw_ctx);

    return WG_SUCCESS;
}

cam_status
img_draw_cross(Img_draw *ctx, Wg_image *img, wg_uint y, wg_uint x, ...)
{
    wg_uint height = 0;
    wg_uint width = 0;
    wg_uint row = 0;
    wg_uint col = 0;
    va_list arg_lst;

    CHECK_FOR_NULL_PARAM(ctx);
    CHECK_FOR_NULL_PARAM(img);

    va_start(arg_lst, x);

    img_get_height(img, &height);
    img_get_width(img, &width);

    for (col = 0; col < width; ++col){
        img_gs_draw_pixel(img, y, col, arg_lst);
    }

    va_start(arg_lst, x);

    for (row = 0; row < height; ++row){
        ctx->draw_pixel(img, row, x, arg_lst);
    }

    return CAM_SUCCESS;
}

wg_status
img_draw_line_mc(Img_draw *ctx, Wg_image *img, wg_float m, wg_uint c, ...)
{
    wg_uint width = 0;
    wg_uint height = 0;
    wg_int ypos = 0;
    wg_int xpos = 0;
    va_list arg_lst;

    CHECK_FOR_NULL_PARAM(ctx);
    CHECK_FOR_NULL_PARAM(img);

    if (img->type != IMG_GS){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                img->type, IMG_GS);
        return CAM_FAILURE;
    }

    va_start(arg_lst, c);

    img_get_width(img, &width);
    img_get_height(img, &height);

    for (xpos = 0; xpos < width; ++xpos){
        ctx->draw_pixel(img, xpos, ypos, arg_lst);
    }

    return WG_SUCCESS;
}

wg_status
img_draw_line(Img_draw *ctx, Wg_image *img, 
        wg_uint y1, wg_uint x1,
        wg_uint y2, wg_uint x2, ...)
{
    wg_int y2_1 = 0;
    wg_int x2_1 = 0;
    va_list arg_lst;
    wg_int dx = 0;
    wg_int incx = 0;
    wg_int dy = 0;
    wg_int incy = 0;

    CHECK_FOR_NULL_PARAM(ctx);
    CHECK_FOR_NULL_PARAM(img);

    va_start(arg_lst, x2);

    y2_1 = y2 - y1;
    x2_1 = x2 - x1;

    if (abs(x2_1) > abs(y2_1)){
        incx = x2_1 > 0 ? -1 : 1; 
        for (dx = 0; x2 != x1; x2 += incx){
            if (dx > y2_1){
                dx -= x2_1;
            }

            dx += y2_1;
            ctx->draw_pixel(img, y2, x2, arg_lst);
        }
    }else{
        incy = y2_1 > 0 ? -1 : 1; 
        for (dy = 0; y2 != y1; y2 += incy){
            if (dy > x2_1){
                dy -= y2_1;
            }

            dy += x2_1;
            ctx->draw_pixel(img, y2, x2, arg_lst);
        }
    }

    return WG_SUCCESS;
}
