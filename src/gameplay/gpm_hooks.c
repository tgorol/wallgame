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
#include <wg_msg.h>

#include "include/gpm_console.h"
#include "include/gpm_console_parser.h"
#include "include/gpm_ini.h"
#include "include/gpm_game.h"

WG_PRIVATE wg_status
create_args(const wg_char* plug_path, const wg_char *pipe, 
        char ***argv);

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
    const Config_field *plugin = NULL;
    const wg_char *pipe_name = NULL;
    const wg_char *path_name = NULL;
    wg_status status = WG_FAILURE;
    wg_char **argv_app = NULL;
    wg_char **argv_plugin = NULL;

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

    /* get plugin name for a game */
    status = gpm_ini_get_field_by_id(game, GAME_PLUGIN_NAME, &plugin);
    if (WG_FAILURE == status){
        WG_LOG("No plugin defined for a game %s\n", game->name);
        return WG_FAILURE;
    }

    /* get game path             */
    status = gpm_ini_get_field_by_id(game, GAME_PATH, &path);
    if (WG_FAILURE == status){
        WG_LOG("No path for game %s\n", game->name);
        return WG_FAILURE;
    }

    /* get server section        */
    status = gpm_ini_get_server(&server);
    if (WG_FAILURE == status){
        WG_LOG("Server section access error\n");
        return WG_FAILURE;
    }

    /* get server pipe name      */
    status = gpm_ini_get_field_by_id(server, PIPE_PATH, &pipe);
    if (WG_FAILURE == status){
        WG_LOG("No pipe path in Server section of ini file\n");
        return WG_FAILURE;
    }

    /* create argv for plugin */
    pipe_name = (const wg_char*)&pipe->value.string;
    path_name = (const wg_char*)&plugin->value.string;
    status = create_args(path_name, pipe_name, &argv_plugin);
    CHECK_FOR_FAILURE(status);

    /* create argv for application */
    path_name = (const wg_char*)&path->value.string;
    status = create_args(path_name, pipe_name, &argv_app);
    CHECK_FOR_FAILURE(status);

    /* start the application */
    status = gpm_game_run(argv_app, argv_plugin, pipe_name);
    if (WG_FAILURE == status){
        gpm_console_remove_args(argv_app);
        gpm_console_remove_args(argv_plugin);
        return WG_FAILURE;
    }

    gpm_console_remove_args(argv_app);
    gpm_console_remove_args(argv_plugin);
    gpm_game_set_id(game_id);

    WG_LOG("Game Started !\n");

    return WG_SUCCESS;
}

wg_status
cb_stop(wg_int argc, wg_char *args[], void *private_data)
{
    if (WG_TRUE == gpm_game_is_running()){
        gpm_game_kill();
    }else{
        WG_PRINT("There is no game running!\n");
    }
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

                while (gpm_ini_field_iterator_next(&itr, &field) == WG_SUCCESS){
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
        status = gpm_game_add_message(MSG_STRING);
        if (WG_FAILURE == status){
            WG_LOG("Sending error\n");
            return WG_FAILURE;
        }
        ++args;
    }

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
create_args(const wg_char* plug_path, const wg_char *pipe, 
        char ***argv)
{
    wg_status status = WG_FAILURE;
    wg_char *full_path = NULL;
    List_head argv_list;


    /* substitute %p with pipe name  */
    status = wg_substitute(plug_path, '%', "p", pipe, &full_path);
    CHECK_FOR_FAILURE(status);

    list_init(&argv_list);

    /* parse game parameters */
    status = gpm_console_parse(full_path, &argv_list);
    if (WG_FAILURE == status){
        WG_FREE(full_path);
        return WG_FAILURE;
    }

    WG_FREE(full_path);

    /* convert game parameters into an array */
    status = gpm_console_tokens_to_array(&argv_list, argv);
    if (WG_FAILURE == status){
        gpm_console_remove_token_list(&argv_list);
        return WG_FAILURE;
    }

    gpm_console_remove_token_list(&argv_list);

    return WG_SUCCESS;
}
