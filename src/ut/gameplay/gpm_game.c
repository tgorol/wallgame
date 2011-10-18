#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_trans.h>

#include <ut_tools.h>

#include "gameplay/include/gpm_game.h"


UT_DEFINE(game_test_1)
    Game game;
    wg_char *args[] = {
        "gimp",
        NULL
    };

    UT_PASS_ON(gpm_game_run(args, 0, &game) == WG_SUCCESS)

    sleep(10);

    UT_PASS_ON(gpm_game_kill(&game) == WG_SUCCESS);

UT_END


int
main(int argc, char *argv[])
{
    UT_RUN_TEST(game_test_1);

    return 0;
}
