#ifndef _COLLISION_DETECT_H
#define _COLLISION_DETECT_H

#define CD_POSITION_NUM  32
#define CD_PIPELINE_SIZE 5

typedef enum Cd_orientation {
    CD_PANE_LEFT    = 0, 
    CD_PANE_RIGHT
}Cd_orientation;

typedef void (*cd_pane_hit_cb)(wg_float x, wg_float y);

typedef struct Cd_pane{
    Wg_point2d v1;
    Wg_point2d v2;
    Wg_point2d v3;
    Wg_point2d v4;
    Cd_orientation orientation;
}Cd_pane;

typedef struct Cd_instance{
    Cd_pane         pane_dimention;
    cd_pane_hit_cb  hit_cb;
    int             state;
    Wg_point2d      position[CD_POSITION_NUM];
    wg_uint         position_index;
} Cd_instance;


WG_PUBLIC void
cd_define_pane(const Cd_pane *pane_dimention, Cd_instance *pane);

WG_PUBLIC void
cd_reset_pane(Cd_instance *pane);

WG_PUBLIC cd_pane_hit_cb
cd_get_hit_callback(Cd_instance *pane, cd_pane_hit_cb hit_cb);

WG_PUBLIC void
cd_set_hit_callback(Cd_instance *pane, cd_pane_hit_cb hit_cb);

WG_PUBLIC wg_status
cd_add_position(Cd_instance *pane, const Wg_point2d *point);

#endif

