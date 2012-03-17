#include <stdio.h>
#include <sys/types.h>
#include <string.h>

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

WG_INLINE wg_boolean
is_valid_point(const Wg_point2d *point);

WG_INLINE wg_boolean
is_hit_detected(const Cd_instance *pane, wg_uint *index);

void
cd_define_pane(const Cd_pane *pane_dimention, Cd_instance *pane)
{
    CHECK_FOR_NULL_PARAM(pane);
    CHECK_FOR_NULL_PARAM(pane_dimention);

    pane->pane_dimention = *pane_dimention;
    pane->state          = CD_STATE_INIT;
    pane->hit_cb         = NULL;
    pane->position_index = 0;

    memset(&pane->position_index, '\0', sizeof (pane->position_index));

    return;
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

    CHECK_FOR_NULL_PARAM(pane);
    CHECK_FOR_NULL_PARAM(point);
    CHECK_FOR_NULL_PARAM(pane->hit_cb);

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
                pane->hit_cb(pane->position[hit_index].x, 
                        pane->position[hit_index].y);
                pane->state = CD_STATE_HIT_RECORDED;
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


