#ifndef _GUI_PRIM_H
#define _GUI_PRIM_H

/** 
* @brief Rectancke
*/
typedef struct Wg_rect{
    wg_int x;       /*!< x origin */
    wg_int y;       /*!< y orogin */
    wg_uint width;  /*!< width    */
    wg_uint height; /*!< height   */
}Wg_rect;

/** 
* @brief Point in 2D pane
*/
typedef struct Wg_point2d{
    wg_int x;       /*!< x coordinate */
    wg_int y;       /*!< y coordinate */
}Wg_point2d;

WG_PUBLIC wg_status
wg_point2d_new(wg_int x, wg_int y, Wg_point2d *point);

WG_PUBLIC wg_float
wg_point2d_distance(const Wg_point2d *p1, const Wg_point2d *p2);

WG_PUBLIC void
wg_rect_new(wg_int x, wg_int y, wg_uint width, wg_uint height, Wg_rect *rect);

WG_PUBLIC void
wg_rect_new_from_point2d(const Wg_point2d *pt1, const Wg_point2d *pt2,
        Wg_rect *rect);

WG_PUBLIC void
wg_rect_new_from_points(wg_int x1, wg_int y1, wg_int x2,
        wg_int y2, Wg_rect *rect);

WG_PUBLIC wg_status
wg_rect_move(Wg_rect *rect, wg_int dx, wg_int dy);

WG_PUBLIC wg_status
wg_rect_get_area(const Wg_rect *rect, wg_int *area);

WG_PUBLIC wg_status
wg_rect_get_height(const Wg_rect *rect, wg_uint *height);

WG_PUBLIC wg_status
wg_rect_get_width(const Wg_rect *rect, wg_uint *width);

#endif
