#ifndef _GUI_PRIM_H
#define _GUI_PRIM_H

typedef struct Wg_rect{
    wg_int x;
    wg_int y;
    wg_uint width;
    wg_uint height;
}Wg_rect;

WG_PUBLIC void
wg_rect_new(wg_int x, wg_int y, wg_uint width, wg_uint height, Wg_rect *rect);

WG_PUBLIC void
wg_rect_new_from_points(wg_int x1, wg_int y1, wg_int x2,
        wg_int y2, Wg_rect *rect);

WG_PUBLIC void
wg_rect_move(Wg_rect *rect, wg_int dx, wg_int dy);

#endif
