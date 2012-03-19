#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include "include/gui_prim.h"
#include "include/collision_detect.h"


typedef enum CD_STATE{
    CD_STATE_INVALID       = 0,
    CD_STATE_INIT             ,
    CD_STATE_FILL_PIPELINE    ,
    CD_STATE_START            ,
    CD_STATE_STOP             ,
    CD_STATE_HIT_RECORDED
}CD_STATE;

#define WG_SIGN(val) ((val) > 0 ? 1 : -1)
#define HIT_COUNT_NUM 2
#define PANE_VERT_MARGIN_IN_PIX 30

typedef enum PANE_VERTICLES{
    V0 = 0,
    V1    ,
    V2    ,
    V3    ,
    PANE_VERTICLES_NUM
}PANE_VERTICLES;

WG_INLINE wg_boolean
is_valid_point(const Wg_point2d *point);

WG_INLINE wg_boolean
is_hit_detected(const Cd_instance *pane, wg_uint *index);

WG_STATIC wg_status
fix_pane_veticles(Cd_pane *pane);

WG_STATIC wg_status
fill_bars(Cd_instance *pane);

WG_STATIC wg_status
fill_horizontal_bar(const Wg_point2d *p0, const Wg_point2d *p1, Cd_bar *bar);

WG_STATIC wg_status
fill_vertical_bar(const Wg_point2d *p0, const Wg_point2d *p1, Cd_bar *bar);

WG_STATIC wg_boolean
is_hit_on_pane(Cd_instance *pane, const Wg_point2d *in_pos, 
        wg_float *x, wg_float *y);

wg_status
cd_define_pane(const Cd_pane *pane_dimention, Cd_instance *pane)
{
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL_PARAM(pane);
    CHECK_FOR_NULL_PARAM(pane_dimention);

    pane->pane_dimention = *pane_dimention;
    pane->state          = CD_STATE_INIT;
    pane->hit_cb         = NULL;
    pane->position_index = 0;

    memset(&pane->position_index, '\0', sizeof (pane->position_index));

    status = fix_pane_veticles(&pane->pane_dimention);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }

    fill_bars(pane);

    return status;
}

void
cd_reset_pane(Cd_instance *pane)
{
    CHECK_FOR_NULL_PARAM(pane);

    pane->position_index = 0;
    pane->state = CD_STATE_STOP;

    return;
}

cd_pane_hit_cb
cd_get_hit_callback(Cd_instance *pane, cd_pane_hit_cb hit_cb)
{
    CHECK_FOR_NULL_PARAM(pane);

    return pane->hit_cb;
}

void
cd_set_hit_callback(Cd_instance *pane, cd_pane_hit_cb hit_cb)
{
    CHECK_FOR_NULL_PARAM(pane);

    pane->hit_cb = hit_cb;

    return;
}

wg_status
cd_add_position(Cd_instance *pane, const Wg_point2d *point)
{
    wg_uint hit_index = 0;
    wg_float hit_x = WG_FLOAT(0.0);
    wg_float hit_y = WG_FLOAT(0.0);

    CHECK_FOR_NULL_PARAM(pane);
    CHECK_FOR_NULL_PARAM(point);
    
    if (NULL == pane->hit_cb){
        return WG_FAILURE;
    }

    switch (pane->state){
    case CD_STATE_INIT:
    case CD_STATE_STOP:
        if (is_valid_point(point)){
            pane->position_index = 0;
            pane->state = CD_STATE_FILL_PIPELINE;
            pane->position[pane->position_index++] = *point; 
            pane->position_index %= CD_POSITION_NUM;
        }
        break;
    case CD_STATE_FILL_PIPELINE:
        if (is_valid_point(point)){
            pane->position[pane->position_index++] = *point; 
            pane->position_index %= CD_POSITION_NUM;
            if (pane->position_index >= CD_PIPELINE_SIZE){
                pane->state = CD_STATE_START;
            }
        }
        break;
    case CD_STATE_START:
        if (is_valid_point(point)){
            pane->position[pane->position_index++] = *point; 
            pane->position_index %= CD_POSITION_NUM;
            if (is_hit_detected(pane, &hit_index)){
                if (is_hit_on_pane(pane, &pane->position[hit_index],
                    &hit_x, &hit_y)){
                        pane->hit_cb(hit_x, hit_y);
                        pane->state = CD_STATE_HIT_RECORDED;
                }
            }
        }else{
            pane->state          = CD_STATE_STOP;
            pane->position_index = 0;
        }
        break;
    case CD_STATE_HIT_RECORDED:
        if (!is_valid_point(point)){
            pane->state = CD_STATE_STOP;
        }
        break;
    default:
        WG_ERROR("BUG: Should not be here!\n");
    }

    return WG_SUCCESS;
}

WG_INLINE wg_boolean
is_valid_point(const Wg_point2d *point)
{
   return ((point->x != (wg_uint)-1) && (point->y != (wg_uint)-1)); 
}

WG_INLINE wg_boolean
is_hit_detected(const Cd_instance *pane, wg_uint *index)
{
    wg_int i      = 0;
    wg_int x_pos  = 0;
    wg_int dx_0   = 0;
    wg_int dx_1   = 0;
    wg_uint hit_count = 0;
    wg_boolean ret = WG_FALSE;

    CHECK_FOR_NULL_PARAM(pane);
    CHECK_FOR_NULL_PARAM(index);

    i = pane->position_index - CD_PIPELINE_SIZE;

    i = (i < 0) ? CD_POSITION_NUM - i : i;

    x_pos = pane->position[i++].x;
    i    %= CD_POSITION_NUM;
    dx_0  = pane->position[i].x - x_pos;
    x_pos = pane->position[i++].x;
    i    %= CD_POSITION_NUM;
    while (i != pane->position_index){
        dx_1 = pane->position[i].x - x_pos;
        if (WG_SIGN(dx_0) != WG_SIGN(dx_1)){
            ++hit_count;
            if (hit_count >= HIT_COUNT_NUM){
                i -= HIT_COUNT_NUM;
                *index = i % CD_POSITION_NUM;
                ret = WG_TRUE;
                break;
            }
        }else{
            hit_count -= ((hit_count == 0) ? 0 : 1);
            dx_0 = dx_1;
        }

        x_pos = pane->position[i].x;
        ++i;
        i %= CD_POSITION_NUM;
    }

    return ret;
}

WG_STATIC int
sort_verticles_left(const void *v1, const void *v2)
{
    Wg_point2d **p1 = NULL;
    Wg_point2d **p2 = NULL;

    p1 = (Wg_point2d**)v1;
    p2 = (Wg_point2d**)v2;

    return (*p2)->x - (*p1)->x;
}

WG_STATIC int
sort_verticles_right(const void *v1, const void *v2)
{
    Wg_point2d **p1 = NULL;
    Wg_point2d **p2 = NULL;

    p1 = (Wg_point2d**)v1;
    p2 = (Wg_point2d**)v2;

    return (*p1)->x - (*p2)->x;
}

WG_STATIC void
sort_pane_verticles(Wg_point2d **points, wg_uint num, 
        Cd_orientation orientation)
{
    qsort(points, num, sizeof (*points), 
        (orientation == CD_PANE_LEFT) ? sort_verticles_left 
                                      : sort_verticles_right
        );

    return;
}

WG_STATIC void
swap_points(Wg_point2d **p1, Wg_point2d **p2)
{
    Wg_point2d *tmp = NULL;

    tmp = *p1;
    *p1 = *p2;
    *p2 = tmp;

    return;
}

WG_STATIC wg_status
fix_pane_veticles(Cd_pane *pane_dimention)
{
    Wg_point2d *screen[PANE_VERTICLES_NUM];
    wg_int     l = 0;
    Cd_pane pd;

    screen[V0] = &pane_dimention->v1;
    screen[V1] = &pane_dimention->v2;
    screen[V2] = &pane_dimention->v3;
    screen[V3] = &pane_dimention->v4;

    sort_pane_verticles(screen, PANE_VERTICLES_NUM, 
            pane_dimention->orientation);

    l = abs(screen[V0]->x - screen[V1]->x);
    if (l > PANE_VERT_MARGIN_IN_PIX){
        return WG_FAILURE;
    }

    l = abs(screen[V2]->x - screen[V3]->x);
    if (l > PANE_VERT_MARGIN_IN_PIX){
        return WG_FAILURE;
    }

    if (screen[V0]->y > screen[V1]->y){
       swap_points(&screen[V0], &screen[V1]);
    }

    if (screen[V2]->y > screen[V3]->y){
       swap_points(&screen[V2], &screen[V3]);
    }
   
    pd.v4 = *screen[V1];
    pd.v1 = *screen[V0];
    pd.v3 = *screen[V3];
    pd.v2 = *screen[V2];
    pd.orientation = pane_dimention->orientation;

    *pane_dimention = pd;

    return WG_SUCCESS;
}

WG_STATIC wg_status
fill_vertical_bar(const Wg_point2d *p0, const Wg_point2d *p1, Cd_bar *bar)
{
    CHECK_FOR_NULL_PARAM(p0);
    CHECK_FOR_NULL_PARAM(p1);
    CHECK_FOR_NULL_PARAM(bar);

    bar->type = CD_BAR_VERTICAL;

    bar->limit_bottom = WG_FLOAT(p0->y);
    bar->limit_top    = WG_FLOAT(p1->y);
    bar->point_bottom = *p0;
    bar->point_top    = *p1;

    bar->length = WG_FLOAT(abs(bar->limit_top - bar->limit_bottom));

    bar->b = 0;
    bar->a = WG_FLOAT((p0->x + p1->x) / 2.0);

    return WG_SUCCESS;
}

WG_STATIC wg_status
fill_horizontal_bar(const Wg_point2d *p0, const Wg_point2d *p1, Cd_bar *bar)
{
    wg_float dx = WG_FLOAT(0.0);
    wg_float dy = WG_FLOAT(0.0);

    CHECK_FOR_NULL_PARAM(p0);
    CHECK_FOR_NULL_PARAM(p1);
    CHECK_FOR_NULL_PARAM(bar);

    bar->type = CD_BAR_HORIZONTAL;

    bar->limit_bottom = WG_FLOAT(p0->x);
    bar->limit_top    = WG_FLOAT(p1->x);
    bar->point_bottom = *p0;
    bar->point_top    = *p1;
    bar->length = wg_point2d_distance(p0, p1);
    
    dx =  p1->x - p0->x;
    dy =  p1->y - p0->y;

    bar->a = dy / dx;
    bar->b = p0->y - (bar->a * p0->x);

    return WG_SUCCESS;
}

WG_STATIC wg_status
fill_bars(Cd_instance *pane)
{
    CHECK_FOR_NULL_PARAM(pane);

    /* Fill left bar */
    fill_vertical_bar(&pane->pane_dimention.v1, &pane->pane_dimention.v4,
            &pane->left_bar);

    /* Fill right bar */
    fill_vertical_bar(&pane->pane_dimention.v2, &pane->pane_dimention.v3,
            &pane->right_bar);

    /* Fill bottom bar */
    fill_horizontal_bar(&pane->pane_dimention.v1, &pane->pane_dimention.v2,
            &pane->bottom_bar);

    /* Fill top bar */
    fill_horizontal_bar(&pane->pane_dimention.v4, &pane->pane_dimention.v3,
            &pane->top_bar);

    return WG_SUCCESS;
}



WG_STATIC wg_boolean
is_hit_on_pane(Cd_instance *pane, const Wg_point2d *in_pos, 
        wg_float *x, wg_float *y)
{
    Cd_bar *l_bar = NULL;
    Cd_bar *r_bar = NULL;
    Cd_bar *t_bar = NULL;
    Cd_bar *b_bar = NULL;
    wg_float y_pos_bottom = WG_FLOAT(0.0);
    wg_float y_pos_top    = WG_FLOAT(0.0);
    Wg_point2d pt;

    l_bar = &pane->left_bar;
    if (in_pos->x < l_bar->a){
        return WG_FALSE;
    }

    r_bar = &pane->right_bar;
    if (in_pos->x > r_bar->a){
        return WG_FALSE;
    }

    t_bar = &pane->top_bar;
    y_pos_top = t_bar->a * in_pos->x + t_bar->b;
    if (in_pos->y > y_pos_top){
       return WG_FALSE;
    }

    b_bar = &pane->bottom_bar;
    y_pos_bottom = b_bar->a * in_pos->x + b_bar->b;
    if (in_pos->y < y_pos_bottom){
       return WG_FALSE;
    }

    *y = (in_pos->y - y_pos_bottom) / (y_pos_top - y_pos_bottom);

    wg_point2d_new(in_pos->x, WG_INT(y_pos_bottom), &pt);
    *x = wg_point2d_distance(&pt, &b_bar->point_bottom) / b_bar->length;

    return WG_TRUE;
}

