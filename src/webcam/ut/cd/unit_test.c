#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <ut_tools.h>

#include "include/gui_prim.h"
#include "include/collision_detect.h"

UT_DEFINE(collision_init_test)
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

    cd_define_pane(&pane, &inst);
UT_END

static void
hit_cb(wg_float x, wg_float y)
{
    WG_PRINT("Hit at %f:%f\n", x, y);
}

UT_DEFINE(collision_add_test)
    Cd_pane pane;
    Cd_instance inst;
    wg_int i = 0;

    Wg_point2d test_throw[] = {
        {0,  0},
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

    test_throw[3].x = 4;
    test_throw[4].x = 3;
    test_throw[5].x = 2;

    for (i = 0; i < ELEMNUM(test_throw); ++i){
        UT_PASS_ON(cd_add_position(&inst, &test_throw[i]) == WG_SUCCESS);
    }


UT_END

int
main(int argc, char *argv[])
{
    UT_RUN_TEST(collision_init_test);
    UT_RUN_TEST(collision_add_test);

    return EXIT_SUCCESS;
}

