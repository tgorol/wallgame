#ifndef _COLLISION_DETECT_H
#define _COLLISION_DETECT_H

#define CD_POSITION_NUM  64
#define CD_PIPELINE_SIZE 3
#define HIT_COUNT_NUM 1

#define CD_INVALID_COORD ((wg_uint)-1)

/** 
* @brief Screen orientation
*/
typedef enum Cd_orientation {
    CD_PANE_LEFT    = 0    ,     /*!< left orientation                     */
    CD_PANE_RIGHT          ,     /*!< right orientation                    */   
}Cd_orientation;

/** 
* @brief Pane corners
*/
typedef enum PANE_VERTICLES{
    V0 = 0,             /*!< v1 */
    V1    ,             /*!< v2 */
    V2    ,             /*!< v3 */
    V3    ,             /*!< v4 */
    PANE_VERTICLES_NUM  /*!< number of corners */
}PANE_VERTICLES;

typedef void (*cd_pane_hit_cb)(wg_float x, wg_float y, void *user_data);

/** 
* @brief Collision region
*/
typedef struct Cd_pane{
    Wg_point2d v1;               /*!< vetrex      */
    Wg_point2d v2;               /*!< vertex      */
    Wg_point2d v3;               /*!< vertex      */
    Wg_point2d v4;               /*!< vetrex      */
    Cd_orientation orientation;  /*!< orientation */
}Cd_pane;

/** 
* @brief Screen bar type
*/
typedef enum Cd_bar_type{
    CD_BAR_HORIZONTAL = 0,  /*!< Horizontal bar */
    CD_BAR_VERTICAL      ,  /*!< Vertical bar   */
}Cd_bar_type;


/** 
* @brief Bar instance
*
* This structure stores a bar in a y = m*x+b format
*/
typedef struct Cd_bar{
    Cd_bar_type type;          /*!< typeof the bar       */
    wg_float    a;             /*!< a component          */
    wg_float    b;             /*!< b component          */
    wg_float    limit_top;     /*!< top limit            */
    wg_float    limit_bottom;  /*!< bottom limit         */
    wg_float    length;        /*!< length               */
    Wg_point2d  point_top;     /*!< first end point      */
    Wg_point2d  point_bottom;  /*!< second end point     */
}Cd_bar;

/** 
* @brief Collision detector instance
*/
typedef struct Cd_instance{
    Cd_pane         pane_dimention;            /*!< collision region        */
    cd_pane_hit_cb  hit_cb;                    /*!< hit callback            */
    void           *hit_cb_user_data;          /*!< hit callback data       */
    int             state;                     /*!< detector state          */
    Wg_point2d      position[CD_POSITION_NUM]; /*!< position buffer         */
    wg_uint         position_index;            /*!< position buffer head    */
    Cd_bar          top_bar;                   /*!< top bar region          */
    Cd_bar          bottom_bar;                /*!< bottom bar region       */
    Cd_bar          left_bar;                  /*!< left bar region         */
    Cd_bar          right_bar;                 /*!< right bar region        */
} Cd_instance;


WG_PUBLIC wg_status
cd_init(const Cd_pane *pane_dimention, Cd_instance *pane);

WG_PUBLIC void
cd_cleanup(Cd_instance *pane);

WG_PUBLIC wg_status
cd_reset_pane(Cd_instance *pane);

WG_PUBLIC cd_pane_hit_cb
cd_get_hit_callback(Cd_instance *pane, cd_pane_hit_cb hit_cb);

WG_PUBLIC wg_status
cd_set_hit_callback(Cd_instance *pane, cd_pane_hit_cb hit_cb, void *user_data);

WG_PUBLIC wg_status
cd_add_position(Cd_instance *pane, const Wg_point2d *point);

WG_PUBLIC wg_status
cd_get_pane(Cd_instance *pane, Cd_pane *pane_dimention);

WG_PUBLIC wg_status
cd_set_pane(Cd_instance *pane, const Cd_pane *pane_dimention);

WG_PUBLIC wg_status
cd_set_pane_from_array(Cd_instance *pane, Wg_point2d array[PANE_VERTICLES_NUM]);

wg_status
cd_get_pane_as_array(Cd_instance *pane, Wg_point2d array[PANE_VERTICLES_NUM]);

#endif

