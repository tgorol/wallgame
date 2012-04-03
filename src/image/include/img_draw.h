#ifndef IMG_DRAW_H
#define IMG_DRAW_H

typedef wg_status (*put_pix)(Wg_image *img, wg_int y, wg_int x, va_list args);

/** 
* @brief Image drawing context
*/
typedef struct Img_draw{
    put_pix draw_pixel;   /*!< function to draw a pixel on the image */
}Img_draw;

WG_PUBLIC wg_status
img_draw_get_context(img_type type, Img_draw *draw_ctx);

WG_PUBLIC wg_status
img_draw_cleanup_context(Img_draw *draw_ctx);

wg_status
img_draw_cross(Img_draw *ctx, Wg_image *img, wg_uint y, wg_uint x, ...);

WG_PUBLIC wg_status
img_draw_line_mc(Img_draw *ctx, Wg_image *img, wg_float m, wg_uint c, ...);

WG_PUBLIC wg_status
img_draw_line(Img_draw *ctx, Wg_image *img, 
        wg_uint y1, wg_uint x1,
        wg_uint y2, wg_uint x2, ...);

#endif
