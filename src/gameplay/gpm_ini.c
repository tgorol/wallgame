#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_cm.h>
#include <wg_gpm.h>

#include "include/gpm.h"
#include "include/gpm_ini.h"

/*! \defgroup  Gameplay_ini Gameplay Config
 * @ingroup gameplay
 */

/*! @{ */

/**
 * @brief Server section
 */

WG_PRIVATE Config_section config_server = {.type = SERVER_SECTION};

/**
 * @brief GameList section
 */
WG_PRIVATE Config_section config_games_list = {.type = GAMES_LIST_SECTION};


/**
 * @brief List of Game sections
 */
WG_PRIVATE Config_section config_game[CONFIG_MAX_GAMES_NUM];

/**
 * @brief Read ini file and store sections internally.
 *
 * @param file_name ini file name
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_ini_read(wg_char *file_name)
{
    wg_status status = WG_FAILURE;
    Config config;
    wg_int index = 0;

    memset(&config, '\0', sizeof (Config));

    status = cm_init(file_name, "r", &config);
    CHECK_FOR_FAILURE(status);

    WG_DEBUG("Ini file %s opended\n", file_name);

    status = cm_read_section(&config, &config_server);
    if (WG_FAILURE == status){
        cm_cleanup(&config);
        return WG_FAILURE;
    }
    WG_DEBUG("Server section read\n");

    status = cm_read_section(&config, &config_games_list);
    if (WG_FAILURE == status){
        cm_cleanup(&config);
        return WG_FAILURE;
    }

    WG_DEBUG("Games List section read\n");

    for (index = 0; index < CONFIG_MAX_GAMES_NUM; ++index){
        if (CONFIG_FIELD_INVALID != config_games_list.field[index].type){
            status = cm_read_game_section(&config,
                    config_games_list.field[index].value.string,
                    &(config_game[index]));
            if (WG_FAILURE == status){
                cm_cleanup(&config);
                return WG_FAILURE;
            }
            WG_DEBUG("Game section read : %s\n",
                    config_games_list.field[index].value.string);
        }
    }

    cm_cleanup(&config);
    CHECK_FOR_FAILURE(status);

    WG_DEBUG("Ini file %s closed\n", file_name);

    return WG_SUCCESS;
}

/**
 * @brief Get Server section
 *
 * @param server memory to store the section 
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_ini_get_server(const Config_section **server)
{
    CHECK_FOR_NULL(server);

    *server = &config_server;

    return WG_SUCCESS;
}

/**
 * @brief Search a games list by index
 *
 * @param index index of the game in games list
 * @param game memory to store a game section if found
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_ini_get_game_by_index(wg_uint index, const Config_section **game)
{
    CHECK_FOR_NULL(game);
    CHECK_FOR_RANGE_GE(index, CONFIG_MAX_GAMES_NUM);

    *game = &(config_game[index]);

    return WG_SUCCESS;
}

/**
 * @brief Get maximum numner of games.
 *
 * @param num
 *
 * @return 
 */
wg_status
gpm_ini_get_max_num_of_games(wg_uint *num)
{
    CHECK_FOR_NULL(num);

    *num =  CONFIG_MAX_GAMES_NUM;

    return WG_SUCCESS;
}

/**
 * @brief Get field iterator
 *
 * @param itr  Iterator to initialize
 * @param section section to get field iterator for.
 *
 * @return 
 */
wg_status
gpm_ini_get_field_iterator(Field_iterator *itr, const Config_section *section)
{
    CHECK_FOR_NULL(itr);
    CHECK_FOR_NULL(section);

    itr->section = section;
    itr->index = 0;
    itr->last_index = CONFIG_MAX_FIELDS_NUM - 1;

    return WG_SUCCESS;
}

/**
 * @brief Get next field from the iterator
 *
 * @param itr Iterator
 * @param config_field Config field to get iterator for
 *
 * @return Field element or NULL if end of list
 */
wg_status
gpm_ini_field_iterator_next(Field_iterator *itr, 
        const Config_field **config_field)
{
    const Config_field *cf = NULL;
    wg_status status = WG_FAILURE;

    /* Skip empty sections */
    while (itr->index <= itr->last_index){
        cf = &(itr->section->field[itr->index++]);
        if (cf->type != CONFIG_FIELD_INVALID){
            *config_field = cf;
            status = WG_SUCCESS;
            break;
        }
        cf = NULL;
    } 

    return status;
}


/**
 * @brief Search a games list by name
 *
 * @param name name of the game to find
 * @param game memory to store a game section if found
 * @param index [optional] memory to store an index of the game
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_ini_get_game_by_name(wg_char *name, const Config_section **game,
        wg_uint *index)
{
    wg_int i = 0;
    int compare_status = -1;
    Config_field *config_field = NULL;

    CHECK_FOR_NULL(game);
    CHECK_FOR_NULL(name);

    for (i = 0; (i < CONFIG_MAX_GAMES_NUM) && (compare_status != 0); ++i){
        config_field = &(config_games_list.field[i]);
        if (CONFIG_FIELD_INVALID != config_field->type){
           switch (config_field->type){
               case CONFIG_FIELD_STRING:
                   compare_status = 
                       strcmp( config_field->value.string, name);
                   break;
                default:
                   WG_ERROR("Unsuported type %d", 
                           config_field->type);
                   break;
           }
        }
    }

    if (NULL != index){
        *index = i;
    }

    *game = (compare_status == 0) ? &(config_game[i]) : NULL;

    return (compare_status == 0) ? WG_SUCCESS : WG_FAILURE;
}

/**
 * @brief Get a field of the section by its id
 *
 * @param section   section 
 * @param id        id of the field
 * @param config_field  memory to store a pointer to the field
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_ini_get_field_by_id(const Config_section *section, wg_uint id,
        const Config_field **config_field)
{
    const Config_field *cf = NULL; 
    wg_status status = WG_FAILURE;

    CHECK_FOR_NULL(section);
    CHECK_FOR_NULL(config_field);

    CHECK_FOR_RANGE_GE(id, CONFIG_MAX_FIELDS_NUM);

    cf = &(section->field[id]);

    if (cf->type != CONFIG_FIELD_INVALID){
        *config_field = cf;
        status = WG_SUCCESS;
    }

    return status;
}

/*! @} */
