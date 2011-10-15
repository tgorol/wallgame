#include <stdlib.h>
#include <stdio.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_string.h>
#include <wg_cm.h>
#include <wg_gpm.h>
#include <wg_trans.h>

#include "include/gpm_console.h"
#include "include/gpm_ini.h"

wg_status 
cb_exit(wg_int argc, wg_char *args[], void *private_data)
{
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
cb_start(wg_int argc, wg_char *args[], void *private_data)
{
    return WG_SUCCESS;
}

wg_status
cb_stop(wg_int argc, wg_char *args[], void *private_data)
{
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

    if (argc == 1){
        gpm_ini_get_max_num_of_games(&num);

        for (index = 0; index < num; ++index){
            status = gpm_ini_get_game_by_index(index, &game);
            if (WG_SUCCESS == status){
                printf("%2u: %s\n", index,
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
    Transport trans = {0};
    wg_status status = WG_FAILURE;
    wg_uchar test_msg[] = "Test message";

    status = trans_unix_new(&trans, "/tmp/demosocket");
    CHECK_FOR_FAILURE(status);

    status = trans_unix_connect(&trans);
    if (WG_FAILURE == status){
        trans_unix_close(&trans);
        return WG_SUCCESS;
    }

    status = trans_unix_send(&trans, test_msg, sizeof (test_msg) - 1);
    if (WG_FAILURE == status){
        trans_unix_close(&trans);
        return WG_SUCCESS;
    }

    trans_unix_close(&trans);
    
    return WG_SUCCESS;
}
