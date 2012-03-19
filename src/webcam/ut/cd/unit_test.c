#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <ut_tools.h>

#include "include/gui_prim.h"
#include "include/collision_detect.h"

UT_DEFINE(collision_init_test_1)
    Cd_pane pane;
    Cd_instance inst;

    pane.v1.x = 0;
    pane.v1.y = 0;

    pane.v2.x = 10;
    pane.v2.y = 0;

    pane.v3.x = 10;
    pane.v3.y = 10;

    pane.v4.x = 0;
    pane.v4.y = 10;

    pane.orientation = CD_PANE_RIGHT;

    UT_PASS_ON(cd_define_pane(&pane, &inst) == WG_SUCCESS);

    UT_PASS_ON(inst.pane_dimention.v1.x == 0);
    UT_PASS_ON(inst.pane_dimention.v1.y == 0);

    UT_PASS_ON(inst.pane_dimention.v2.x == 10);
    UT_PASS_ON(inst.pane_dimention.v2.y == 0);

    UT_PASS_ON(inst.pane_dimention.v3.x == 10);
    UT_PASS_ON(inst.pane_dimention.v3.y == 10);

    UT_PASS_ON(inst.pane_dimention.v4.x == 0);
    UT_PASS_ON(inst.pane_dimention.v4.y == 10);
UT_END

UT_DEFINE(collision_init_test_2)
    Cd_pane pane;
    Cd_instance inst;

    pane.v1.x = 0;
    pane.v1.y = 0;

    pane.v4.x = 10;
    pane.v4.y = 0;

    pane.v3.x = 10;
    pane.v3.y = 10;

    pane.v2.x = 0;
    pane.v2.y = 10;

    pane.orientation = CD_PANE_RIGHT;

    UT_PASS_ON(cd_define_pane(&pane, &inst) == WG_SUCCESS);

    UT_PASS_ON(inst.pane_dimention.v1.x == 0);
    UT_PASS_ON(inst.pane_dimention.v1.y == 0);

    UT_PASS_ON(inst.pane_dimention.v2.x == 10);
    UT_PASS_ON(inst.pane_dimention.v2.y == 0);

    UT_PASS_ON(inst.pane_dimention.v3.x == 10);
    UT_PASS_ON(inst.pane_dimention.v3.y == 10);

    UT_PASS_ON(inst.pane_dimention.v4.x == 0);
    UT_PASS_ON(inst.pane_dimention.v4.y == 10);
UT_END

UT_DEFINE(collision_init_test_3)
    Cd_pane pane;
    Cd_instance inst;

    pane.v1.x = 0;
    pane.v1.y = 0;

    pane.v4.x = 100;
    pane.v4.y = 0;

    pane.v3.x = 100;
    pane.v3.y = 100;

    pane.v2.x = 31;
    pane.v2.y = 100;

    pane.orientation = CD_PANE_RIGHT;

    UT_PASS_ON(cd_define_pane(&pane, &inst) == WG_FAILURE);
UT_END

UT_DEFINE(collision_init_test_4)
    Cd_pane pane;
    Cd_instance inst;

    pane.v1.x = 0;
    pane.v1.y = 0;

    pane.v4.x = 100;
    pane.v4.y = 0;

    pane.v3.x = 131;
    pane.v3.y = 100;

    pane.v2.x = 0;
    pane.v2.y = 100;

    pane.orientation = CD_PANE_RIGHT;

    UT_PASS_ON(cd_define_pane(&pane, &inst) == WG_FAILURE);
UT_END

static void
hit_cb(wg_float x, wg_float y)
{
    WG_PRINT("Hit at %f:%f\n", x, y);
}

UT_DEFINE(collision_add_test_1)
    Cd_pane pane;
    Cd_instance inst;
    wg_int i = 0;

    Wg_point2d test_throw[] = {
        {-1, -1},
        {0,  0},
        {1,  0},
        {2,  0},
        {3,  0},
        {4,  0},
        {5,  0},
        {6,  0},
        {7,  0},
        {8,  0},
        {9,  0},
        {10, 0},
        {11, 0},
        {12, 0},
        {15, 0},
        {16, 0},
        {17, 0},
        {18, 0},
        {20, 0},
        {30, 0},
        {50, 0},
        {-1, -1}
    };

    pane.v1.x = 0;
    pane.v1.y = 0;
    pane.v2.x = 10;
    pane.v2.y = 0;
    pane.v3.x = 10;
    pane.v3.y = 10;
    pane.v4.x = 0;
    pane.v4.y = 10;

    cd_define_pane(&pane, &inst);

    UT_PASS_ON(cd_add_position(&inst, &test_throw[0]) == WG_FAILURE);

    cd_set_hit_callback(&inst, hit_cb);

    for (i = 0; i < ELEMNUM(test_throw); ++i){
        UT_PASS_ON(cd_add_position(&inst, &test_throw[i]) == WG_SUCCESS);
    }

    cd_reset_pane(&inst);

    test_throw[7].x = 5;
    test_throw[8].x = 4;
    test_throw[9].x = 3;

    for (i = 0; i < ELEMNUM(test_throw); ++i){
        UT_PASS_ON(cd_add_position(&inst, &test_throw[i]) == WG_SUCCESS);
    }
UT_END

UT_DEFINE(collision_add_test_2)
    Cd_pane pane;
    Cd_instance inst;
    wg_int i = 0;

    Wg_point2d test_throw[] = {
        {-1, -1},
        {0,  3},
        {1,  3},
        {2,  3},
        {3,  3},
        {4,  3},
        {5,  3},
        {6,  3},
        {7,  3},
        {8,  3},
        {7,  3},
        {6, 3},
        {3, 3},
        {1, 3},
        {-1, -1}
    };

    pane.v1.x = 0;
    pane.v1.y = 0;
    pane.v2.x = 10;
    pane.v2.y = 0;
    pane.v3.x = 10;
    pane.v3.y = 10;
    pane.v4.x = 0;
    pane.v4.y = 10;

    cd_define_pane(&pane, &inst);

    UT_PASS_ON(cd_add_position(&inst, &test_throw[0]) == WG_FAILURE);

    cd_set_hit_callback(&inst, hit_cb);

    for (i = 0; i < ELEMNUM(test_throw); ++i){
        UT_PASS_ON(cd_add_position(&inst, &test_throw[i]) == WG_SUCCESS);
    }
UT_END

int
main(int argc, char *argv[])
{
    UT_RUN_TEST(collision_init_test_1);
    UT_RUN_TEST(collision_init_test_2);
    UT_RUN_TEST(collision_init_test_3);
    UT_RUN_TEST(collision_init_test_4);
    UT_RUN_TEST(collision_add_test_1);
    UT_RUN_TEST(collision_add_test_2);

    return EXIT_SUCCESS;
}

