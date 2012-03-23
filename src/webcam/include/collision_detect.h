#ifndef _COLLISION_DETECT_H
#define _COLLISION_DETECT_H

#define CD_POSITION_NUM  64
#define CD_PIPELINE_SIZE 3
#define HIT_COUNT_NUM 2

typedef enum Cd_orientation {
    CD_PANE_RIGHT   = 0 ,
    CD_PANE_LEFT        , 
}Cd_orientation;

typedef void (*cd_pane_hit_cb)(wg_float x, wg_float y);

typedef struct Cd_pane{
    Wg_point2d v1;
    Wg_point2d v2;
    Wg_point2d v3;
    Wg_point2d v4;
    Cd_orientation orientation;
}Cd_pane;

typedef enum Cd_bar_type{
    CD_BAR_HORIZONTAL = 0,
    CD_BAR_VERTICAL      ,
}Cd_bar_type;

/* This structure stores a bar in a y = m*x+b format */
typedef struct Cd_bar{
    Cd_bar_type type;
    wg_float    a;
    wg_float    b;
    wg_float    limit_top;
    wg_float    limit_bottom;
    wg_float    length;
    Wg_point2d  point_top;
    Wg_point2d  point_bottom;
}Cd_bar;

typedef struct Cd_instance{
    Cd_pane         pane_dimention;
    cd_pane_hit_cb  hit_cb;
    int             state;
    Wg_point2d      position[CD_POSITION_NUM];
    wg_uint         position_index;
    Cd_bar top_bar;
    Cd_bar bottom_bar;
    Cd_bar left_bar;
    Cd_bar right_bar;
} Cd_instance;


WG_PUBLIC wg_status
cd_define_pane(const Cd_pane *pane_dimention, Cd_instance *pane);

WG_PUBLIC void
cd_reset_pane(Cd_instance *pane);

WG_PUBLIC cd_pane_hit_cb
cd_get_hit_callback(Cd_instance *pane, cd_pane_hit_cb hit_cb);

WG_PUBLIC void
cd_set_hit_callback(Cd_instance *pane, cd_pane_hit_cb hit_cb);

WG_PUBLIC wg_status
cd_add_position(Cd_instance *pane, const Wg_point2d *point);

WG_PUBLIC wg_status
cd_get_pane(Cd_instance *pane, Cd_pane *pane_dimention);

#endif

