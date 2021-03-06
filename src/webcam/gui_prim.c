#include <stdlib.h>
#include <math.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include "include/gui_prim.h"

/*! \defgroup gui_prim Graphics Primitives
    \ingroup plugin_webcam
*/

/*! @{ */

/** 
* @brief Define new point
* 
* @param x      x coordinate
* @param y      y coordinate
* @param point  memory fo point instance
*/
wg_status
wg_point2d_new(wg_int x, wg_int y, Wg_point2d *point)
{
    CHECK_FOR_NULL_PARAM(point);

    point->x = x;
    point->y = y;

    return WG_SUCCESS;
}

/** 
* @brief Calculate distance between two pointd
* 
* @param p1 point instance
* @param p2 point instance
* 
* @return distance between points
*/
wg_float
wg_point2d_distance(const Wg_point2d *p1, const Wg_point2d *p2)
{
    wg_int dx = 0;
    wg_int dy = 0;

    dx = p2->x - p1->x;
    dy = p2->y - p1->y;

    return WG_FLOAT(sqrt(DOUBLE(dx * dx + dy * dy)));
}

/** 
* @brief Define new rectangle
* 
* @param x       x origin coordinate
* @param y       y origin coordinate
* @param width   width of rectange
* @param height  heighr of rectange
* @param rect    memory for rectangle instance
*/
void
wg_rect_new(wg_int x, wg_int y, wg_uint width, wg_uint height, Wg_rect *rect)
{
    rect->x = x;
    rect->y = y;
    rect->width  = width;
    rect->height = height;

    return;
}

/** 
* @brief Define a rectangle from four points
* 
* @param pt1   point 
* @param pt2   point
* @param rect  memory for rectangle instance
*/
void
wg_rect_new_from_point2d(const Wg_point2d *pt1, const Wg_point2d *pt2,
        Wg_rect *rect)
{
    wg_rect_new_from_points(pt1->x, pt1->y, pt2->x, pt2->y, rect);

    return;
}

/** 
* @brief Define a rectangle from four points
* 
* @param x1  x left top point coordinate
* @param y1  y left top point coordinate
* @param x2  x right bottom point coordinate
* @param y2  y right bottom point coordinate
* @param rect  memory for rectangle instance
*/
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

/** 
* @brief Move rectangle
*
* Move rectangle from current position
* 
* @param rect rectangle instance
* @param dx  x offset
* @param dy  y offset
*/
wg_status
wg_rect_move(Wg_rect *rect, wg_int dx, wg_int dy)
{
    CHECK_FOR_NULL_PARAM(rect);

    rect->x += dx;
    rect->y += dy;

    return WG_SUCCESS;
}

/** @brief Get width
*
* @param rect rectangle instance
* @paeam width memory to store width
*
* @retval WG_SUCCESS 
* @retval WG_FAILURE
*/
wg_status
wg_rect_get_width(const Wg_rect *rect, wg_uint *width)
{
    CHECK_FOR_NULL_PARAM(rect);
    CHECK_FOR_NULL_PARAM(width);

    *width = rect->width;

    return WG_SUCCESS;
}

/** @brief Get height
*
* @param rect rectangle instance
* @param width memory to store height
*
* @retval WG_SUCCESS 
* @retval WG_FAILURE
*/
wg_status
wg_rect_get_height(const Wg_rect *rect, wg_uint *height)
{
    CHECK_FOR_NULL_PARAM(rect);
    CHECK_FOR_NULL_PARAM(height);

    *height = rect->height;

    return WG_SUCCESS;
}

/** @brief Get area
*
* @param[in]  rect rectangle instance
* @param[out] area returned area 
*
* @retval WG_SUCCESS 
* @retval WG_FAILURE
*/
wg_status
wg_rect_get_area(const Wg_rect *rect, wg_int *area)
{
    wg_uint width  = 0;
    wg_uint height = 0;
    CHECK_FOR_NULL_PARAM(rect);
    CHECK_FOR_NULL_PARAM(area);

    wg_rect_get_height(rect, &height);
    wg_rect_get_width(rect, &width);

    *area = height * width;

    return WG_SUCCESS;
}

/*! @{ */
