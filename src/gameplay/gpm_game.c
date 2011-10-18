#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_trans.h>

#include "include/gpm_game.h"


wg_status
gpm_game_run(wg_char *argv[], wg_uint address_index, Game *game)
{
    pid_t game_pid = 0;
    wg_status status = WG_FAILURE;

    switch (game_pid = fork()){
        case 0:
            execvp(argv[0], argv);
            WG_ERROR("BUG: Shoudn't be here\n");
            exit(1);
        case -1:
            WG_ERROR("forking game %s: %s\n", argv[0], strerror(errno));
            return WG_FAILURE;
        default:
            game->process_id = game_pid;
            status = trans_unix_new(&game->transport, argv[address_index]);
            if (WG_FAILURE == status){
                kill(game->process_id, SIGTERM);
                waitpid(game->process_id, NULL, WNOHANG);
                return WG_FAILURE;
            }

            break;
    }

    return WG_SUCCESS;
}

wg_status
gpm_game_send_ln(Game *game, wg_char *line)
{
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL(game);
    CHECK_FOR_NULL(line);

    status = trans_unix_connect(&game->transport);
    CHECK_FOR_FAILURE(status);

    status = trans_unix_send(&game->transport, (wg_uchar*)line, strlen(line));
    CHECK_FOR_FAILURE(status);

    status = trans_unix_close(&game->transport);
    CHECK_FOR_FAILURE(status);

    return WG_SUCCESS;
}


wg_status
gpm_game_kill(Game *game)
{
    CHECK_FOR_NULL(game);

    trans_unix_close(&game->transport);

    kill(game->process_id, SIGTERM);
    waitpid(game->process_id, NULL, WNOHANG);

    return WG_SUCCESS;
}
