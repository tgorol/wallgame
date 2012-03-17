#include <stdlib.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include "include/gui_prim.h"


void
wg_point2d_new(wg_int x, wg_int y, Wg_point2d *point)
{
    CHECK_FOR_NULL_PARAM(point);

    point->x = x;
    point->y = y;

    return;
}

void
wg_rect_new(wg_int x, wg_int y, wg_uint width, wg_uint height, Wg_rect *rect)
{
    CHECK_FOR_NULL_PARAM(rect);

    rect->x = x;
    rect->y = y;
    rect->width  = width;
    rect->height = height;

    return;
}

void
wg_rect_new_from_points(wg_int x1, wg_int y1, wg_int x2,
        wg_int y2, Wg_rect *rect)
{
    wg_int xx1;
    wg_int yy1;
    wg_int xx2;
    wg_int yy2;

    xx1 = WG_MIN(x1, x2);
    yy1 = WG_MIN(y1, y2);
    xx2 = WG_MAX(x1, x2);
    yy2 = WG_MAX(y1, y2);

    wg_rect_new(xx1, yy1, xx2 - xx1, yy2 - yy1, rect);

    return;
}

void
wg_rect_move(Wg_rect *rect, wg_int dx, wg_int dy)
{
    CHECK_FOR_NULL_PARAM(rect);

    rect->x += dx;
    rect->y += dy;

    return;
}
