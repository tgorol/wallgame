#include <stdlib.h>
#include <stdio.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_string.h>
#include <wg_cm.h>
#include <wg_gpm.h>
#include <wg_trans.h>
#include <wg_linked_list.h>

#include "include/gpm_console.h"
#include "include/gpm_console_parser.h"
#include "include/gpm_ini.h"
#include "include/gpm_game.h"


wg_status 
cb_exit(wg_int argc, wg_char *args[], void *private_data)
{
    gpm_game_kill();
    printf("Bye!\n");

    return WG_SUCCESS;
}

wg_status
cb_info(wg_int argc, wg_char *args[], void *private_data)
{
    printf("Author  : %s\n"
           "Version : %s\n",
           STR(AUTHOR),
           STR(VERSION)
          );

    return WG_SUCCESS;
}

wg_status
cb_pause(wg_int argc, wg_char *args[], void *private_data)
{
    return WG_SUCCESS;
}

wg_status
cb_connect(wg_uint argc, wg_char *args[], void *private_data)
{
    if (gpm_game_is_running() == WG_FALSE){
        WG_LOG("No game running\n");
        return WG_FAILURE;
    }

    return WG_SUCCESS;
}

wg_status
cb_start(wg_int argc, wg_char *args[], void *private_data)
{
    wg_uint game_id = 0;
    const Config_section *game = NULL;
    const Config_section *server = NULL;
    const Config_field *path = NULL;
    const Config_field *pipe = NULL;
    wg_char *full_path = NULL;
    wg_status status = WG_FAILURE;
    wg_char **argv = NULL;
    List_head argv_list;

    if (argc != 2){
        WG_LOG("No argument. Pass id of the game to run\n");
        return WG_FAILURE;
    }

    if (gpm_game_is_running() == WG_TRUE){
        WG_LOG("Game running ! Please close first\n");
        return WG_FAILURE;
    }

    /* TODO Fix integer parsing */
    game_id = atoi(args[1]);

    status = gpm_ini_get_game_by_index(game_id, &game);
    if (WG_FAILURE == status){
        WG_LOG("Invalid game id : %s\n", args[1]);
        return WG_FAILURE;
    }

    status = gpm_ini_get_field_by_id(game, GAME_PATH, &path);
    if (WG_FAILURE == status){
        WG_LOG("No path for game %s\n", game->name);
        return WG_FAILURE;
    }

    status = gpm_ini_get_server(&server);
    if (WG_FAILURE == status){
        WG_LOG("Server section access error\n");
        return WG_FAILURE;
    }

    status = gpm_ini_get_field_by_id(server, PIPE_PATH, &pipe);
    if (WG_FAILURE == status){
        WG_LOG("No pipe path in Server section of ini file\n");
        return WG_FAILURE;
    }

    status = wg_substitute((const wg_char*)&path->value.string, '%', "p", 
                (const wg_char*)&pipe->value.string, &full_path);
    CHECK_FOR_FAILURE(status);

    list_init(&argv_list);

    status = gpm_console_parse(full_path, &argv_list);
    if (WG_FAILURE == status){
        WG_FREE(full_path);
        return WG_FAILURE;
    }

    WG_FREE(full_path);

    status = gpm_console_tokens_to_array(&argv_list, &argv);
    if (WG_FAILURE == status){
        gpm_console_remove_token_list(&argv_list);
        return WG_FAILURE;
    }

    gpm_console_remove_token_list(&argv_list);

    status = gpm_game_run(argv, (wg_char*)&pipe->value.string);
    if (WG_FAILURE == status){
        return WG_FAILURE;
    }

    gpm_console_remove_args(argv);
    gpm_game_set_id(game_id);

    WG_LOG("Game Started !\n");

    return WG_SUCCESS;
}

wg_status
cb_stop(wg_int argc, wg_char *args[], void *private_data)
{
    gpm_game_kill();
    return WG_SUCCESS;
}

wg_status
cb_lsg(wg_int argc, wg_char *argv[], void *private_data)
{
    wg_uint num = 0;
    wg_uint index = 0;   
    const Config_section *game = NULL;
    const Config_field *field = NULL;
    wg_status status = WG_FAILURE;
    Field_iterator itr;
    wg_uint game_id = (wg_uint)-1;

    if (argc == 1){
        gpm_ini_get_max_num_of_games(&num);
        gpm_game_get_id(&game_id);

        for (index = 0; index < num; ++index){
            status = gpm_ini_get_game_by_index(index, &game);
            if (WG_SUCCESS == status){
                printf("%2u: %c %s\n", index,
                        (game_id == index ? '*' : ' '),
                        (game->type == INVALID_SECTION) ? "" : game->name);
            }
        }
    }else{
        ++argv;

        while (NULL != *argv){
            status = gpm_ini_get_game_by_index(atoi(*argv), &game);
            if ((WG_SUCCESS == status) & (game->type != INVALID_SECTION)){
                printf("[%s]\n", game->name); 
                gpm_ini_get_field_iterator(&itr, game);

                while (wg_ini_field_iterator_next(&itr, &field) == WG_SUCCESS){
                    printf("%s : %s\n", field->name, field->value.string);
                }
            }
            ++argv;
        }
    }

    return WG_SUCCESS;
}

wg_status
cb_send(wg_int argc, wg_char *args[], void *private_data)
{
    wg_status status = WG_FAILURE;

    if (gpm_game_is_running() == WG_FALSE){
        WG_LOG("No game running\n");
        return WG_FAILURE;
    }

    ++args;

    while (*args != NULL){
        WG_DEBUG("Sending : \"%s\"\n", *args);
        status = gpm_game_send((wg_uchar*)*args, strlen(*args));
        if (WG_FAILURE == status){
            WG_LOG("Sending error\n");
            return WG_FAILURE;
        }
        ++args;
    }

    return WG_SUCCESS;
}
