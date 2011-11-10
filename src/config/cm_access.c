#include <stdlib.h>
#include <stdio.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_cm.h>

#include "include/cm.h"

/*! \addtogroup  Ini
 */

/*! @{ */

WG_PRIVATE wg_status
read_server_section(ini_fd_t ini_fd, Config_section *section);

WG_PRIVATE wg_status
read_games_list_section(ini_fd_t *ini_fd, Config_section *section);

WG_PRIVATE wg_status
read_game_section(ini_fd_t *ini_fd, char *game_name, Config_section *section); 

WG_PRIVATE wg_status
save_field_name(wg_char *name, Config_field *field);

WG_PRIVATE wg_status
add_key(ini_fd_t ini_fd, char *key_name);

WG_PRIVATE wg_status
add_key_value(ini_fd_t ini_fd, Config_field config_field[], wg_int num);

WG_PRIVATE wg_boolean
find_key(char *game_name, Config_field field[], wg_int num, wg_int *pos);

WG_PRIVATE wg_boolean
find_invalid_key(Config_field field[],
        wg_int num, wg_int *pos);

/**
 * @brief Read a regular section from the ini file.
 *
 * This function reads a section from the ini file and stories the data
 * in the variable pointed by section.
 * Type of the section to read must be set by a user in the section variable 
 * before calling this function.
 *
 * @param config    config file instance
 * @param section   section to read
 *
 * @retval WG_FAILURE 
 * @retval WG_SUCCESS
 */
wg_status
cm_read_section(Config *config, Config_section *section)
{
    wg_status status = WG_FAILURE;
    CHECK_FOR_NULL(config);
    CHECK_FOR_NULL(section);

    switch (section->type){
      case SERVER_SECTION:
        status = 
            read_server_section(config->ini_fd, section);
        break;
      case GAMES_LIST_SECTION:
        status = 
            read_games_list_section(config->ini_fd, section);
        break;
      default:
        WG_ERROR("Invalid section type\n");
        status = WG_FAILURE;
    }

    return status;
}

/**
 * @brief Read a game section from the ini file
 *
 * @param config    config file instance
 * @param name      name of the game
 * @param section   section to store keys
 *
 * @retval WG_FAILURE 
 * @retval WG_SUCCESS
 */
wg_status
cm_read_game_section(Config *config, char *name,
        Config_section *section)
{
    wg_status status = WG_FAILURE;
    CHECK_FOR_NULL(config);
    CHECK_FOR_NULL(section);

    status = 
        read_game_section(config->ini_fd, name, section);

    section->type = GAME_SECTION;

    return status;
}

/**
 * @brief Add/overwrite a game section in the ini file
 *
 * @param config    config file instance
 * @param game_name name of the game to add
 * @param section   section with keys 
 *
 * @retval WG_FAILURE 
 * @retval WG_SUCCESS
 */
wg_status
cm_write_game(Config *config, char *game_name, Config_section *section)
{
    int        status        = INI_ERROR;
    wg_status  wg_stat       = WG_FAILURE;
    Config_section games_section;
    wg_int index = 0;
    wg_boolean boolean_status = WG_FALSE;
    Config_field cf;

    CHECK_FOR_NULL(config);
    CHECK_FOR_NULL(game_name);
    CHECK_FOR_NULL(section);

    /* read game list from the ini file */
    wg_stat = read_games_list_section(config->ini_fd, &games_section);
    CHECK_FOR_FAILURE(wg_stat);

    /* check if the game is already in the list */
    boolean_status = find_key(game_name, &(games_section.field[0]),
            CONFIG_MAX_FIELDS_NUM, 
            NULL);
    /* 
     * if the game is in the list overwrite, if not find
     * an empty entry in the list and create key
     */
    if (WG_FALSE == boolean_status){
        boolean_status = find_invalid_key(&(games_section.field[0]),
                CONFIG_MAX_FIELDS_NUM, 
                &index);
        if (WG_TRUE == boolean_status){
            snprintf(cf.name, CONFIG_MAX_FIELD_NAME_SIZE, 
                    GAME_PREFIX"%d", index);
            add_key_value(config->ini_fd, &cf, 1);
        }else{
            WG_ERROR("To many games in the ini file\n");
            return WG_FAILURE;
        }
    }

    /* locate a game key in the games section */
    status = ini_locateKey(config->ini_fd, game_name);
    if (INI_ERROR != status){
        /* write a game key */
        status = ini_writeString(config->ini_fd, game_name);
    }
    CHECK_FOR_INI_ERROR(status);


    /* locate a game section */
    status = ini_locateHeading(config->ini_fd, game_name);
    if (INI_ERROR != status){
        /* if exists delete it */
        status = ini_deleteHeading(config->ini_fd);
    }
    CHECK_FOR_INI_ERROR(status);

    /* create an empty game section */
    status = ini_writeString(config->ini_fd, game_name);
    CHECK_FOR_INI_ERROR(status);

    /* fill the game section */
    wg_stat = add_key_value(config->ini_fd, section->field, 
            CONFIG_MAX_FIELDS_NUM);
    
    return wg_stat;
}

WG_PRIVATE wg_status
read_server_section(ini_fd_t ini_fd, Config_section *section)
{
    int status = -1;
    Config_field *config_field = NULL;
    Config_field *field = NULL;

    CHECK_FOR_NULL(section);

    field = section->field;

    /* locate Server section */
    status = ini_locateHeading(ini_fd, SERVER_SECTION_NAME);
    CHECK_FOR_INI_ERROR(status);

    strncpy(section->name, SERVER_SECTION_NAME, 
            CONFIG_MAX_FIELD_NAME_SIZE);

    /* read pipe path key */
    status = ini_locateKey(ini_fd, SERVER_PIPE_PATH_KEY);
    if (INI_ERROR != status){
        config_field = &(field[PIPE_PATH]);
        status = ini_readString(ini_fd, 
                config_field->value.string, 
                CONFIG_MAX_STRING_SIZE);
        save_field_name(SERVER_PIPE_PATH_KEY, config_field);
    }
    config_field->type = (status != INI_ERROR) ?
        CONFIG_FIELD_STRING   :
        CONFIG_FIELD_INVALID;

    /* read player database path key */
    status = ini_locateKey(ini_fd, SERVER_PLAYER_DB_KEY);
    if (INI_ERROR != status){
        config_field = &(field[PLAYER_DB_PATH]);
        status = ini_readString(ini_fd, 
                config_field->value.string, 
                CONFIG_MAX_STRING_SIZE);
        save_field_name(SERVER_PLAYER_DB_KEY, config_field);
    }
    config_field->type = (status != INI_ERROR) ?
        CONFIG_FIELD_STRING   :
        CONFIG_FIELD_INVALID;

    /* read highscore database path key */
    status = ini_locateKey(ini_fd, SERVER_SCORE_DB_KEY);
    if (INI_ERROR != status){
        config_field = &(field[SCORE_DB_PATH]);
        status = ini_readString(ini_fd, 
                config_field->value.string, 
                CONFIG_MAX_STRING_SIZE);
        save_field_name(SERVER_SCORE_DB_KEY, config_field);
    }
    config_field->type = (status != INI_ERROR) ?
        CONFIG_FIELD_STRING   :
        CONFIG_FIELD_INVALID;

    section->size = 3;

    return WG_SUCCESS;

}

WG_PRIVATE wg_status
read_games_list_section(ini_fd_t *ini_fd, Config_section *section)
{
    int status = -1;
    int index  = 0;
    Config_field *config_field = NULL;
    Config_field *field = NULL;
    char key_name[CONFIG_MAX_STRING_SIZE];

    CHECK_FOR_NULL(section);

    field = section->field;

    /* locate a game list section */
    status = ini_locateHeading(ini_fd, GAMES_SECTION_NAME);
    CHECK_FOR_INI_ERROR(status);

    strncpy(section->name, GAMES_SECTION_NAME,
            CONFIG_MAX_FIELD_NAME_SIZE);

    /* 
     * Iterate through first game%CONFIG_MAX_FIELDS_NUM keys in the
     * section and store it in the field array 
     */
    for (index = 0; index < CONFIG_MAX_FIELDS_NUM; ++index){
        snprintf(key_name, CONFIG_MAX_STRING_SIZE, GAME_PREFIX"%d", index);
        status = ini_locateKey(ini_fd, key_name);
        config_field = &(field[index]);
        if (INI_ERROR != status){
            status = ini_readString(ini_fd, 
                    config_field->value.string, 
                    CONFIG_MAX_STRING_SIZE);
            save_field_name(key_name, config_field);
        }
        config_field->type = (status != INI_ERROR) ?
            CONFIG_FIELD_STRING   :
            CONFIG_FIELD_INVALID;
    }

    section->size = index -1;

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
read_game_section(ini_fd_t *ini_fd, char *game_name, Config_section *section)
{
    int status = -1;
    Config_field *config_field = NULL;
    Config_field *field = NULL;

    CHECK_FOR_NULL(section);
    CHECK_FOR_NULL(game_name);

    field = section->field;

    /* locate a game section */
    status = ini_locateHeading(ini_fd, game_name);
    CHECK_FOR_INI_ERROR(status);

    strncpy(section->name, game_name, 
            CONFIG_MAX_FIELD_NAME_SIZE);

    /* locate a game path key and read */
    config_field = &(field[GAME_PATH]);
    status = ini_locateKey(ini_fd, GAME_PATH_KEY);
    if (INI_ERROR != status){
        status = ini_readString(ini_fd, 
                config_field->value.string, 
                CONFIG_MAX_STRING_SIZE);
        save_field_name(GAME_PATH_KEY, config_field);
    }
    config_field->type = (status != INI_ERROR) ?
        CONFIG_FIELD_STRING   :
        CONFIG_FIELD_INVALID;

    /* locate a plugin name key and read */
    config_field = &(field[GAME_PLUGIN_NAME]);
    status = ini_locateKey(ini_fd, GAME_PLUGIN_NAME_KEY);
    if (INI_ERROR != status){
        status = ini_readString(ini_fd, 
                config_field->value.string, 
                CONFIG_MAX_STRING_SIZE);
        save_field_name(GAME_PLUGIN_NAME_KEY, config_field);
    }
    config_field->type = (status != INI_ERROR) ?
        CONFIG_FIELD_STRING   :
        CONFIG_FIELD_INVALID;

    /* locate a game arguments key and read */
    config_field = &(field[GAME_ARGUMENTS]);
    status = ini_locateKey(ini_fd, GAME_ARGUMENTS_KEY);
    if (INI_ERROR != status){
        status = ini_readString(ini_fd, 
                config_field->value.string, 
                CONFIG_MAX_STRING_SIZE);
        save_field_name(GAME_ARGUMENTS_KEY, config_field);
    }
    config_field->type = (status != INI_ERROR) ?
        CONFIG_FIELD_STRING   :
        CONFIG_FIELD_INVALID;

    section->size = 2;

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
save_field_name(wg_char *name, Config_field *field)
{
    snprintf(field->name, CONFIG_MAX_FIELD_NAME_SIZE, name);

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
add_key(ini_fd_t ini_fd, char *key_name)
{
    int status = INI_ERROR;

    status = ini_locateKey(ini_fd, key_name);
    if (INI_ERROR != status){
        status = ini_writeString(ini_fd, key_name);
    }
    CHECK_FOR_INI_ERROR(status);

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
add_key_value(ini_fd_t ini_fd, Config_field config_field[], wg_int num)
{
    int index         = 0;
    int status        = INI_ERROR;
    wg_status wg_stat = WG_FAILURE;
    Config_field *cf  = NULL;

    for (index = 0; index < num; ++index){
        cf = &(config_field[index]);
        if (CONFIG_FIELD_INVALID != cf->type){
            wg_stat = add_key(ini_fd, cf->name);
            CHECK_FOR_FAILURE(wg_stat);

            status = ini_locateKey(ini_fd, cf->name);
            CHECK_FOR_INI_ERROR(status);
            
            switch (cf->type){
              case CONFIG_FIELD_STRING:
                status = 
                    ini_writeString(ini_fd, cf->value.string);
                    CHECK_FOR_INI_ERROR(status);
                break;
              default:
                WG_ERROR("Invalid field type %d\n", cf->type);
                return WG_FAILURE;
            }
        }
    }

    return WG_SUCCESS;
}

WG_PRIVATE wg_boolean
find_invalid_key(Config_field field[], wg_int num, wg_int *pos)
{
    wg_int index = 0;

    CHECK_FOR_NULL(field);
    CHECK_FOR_NULL(pos);

    for (index = 0; 
            (index < num)                                && 
            (field[index].type != CONFIG_FIELD_INVALID) 
            ; ++index)
        /* VOID LOOP */ ;

    *pos = index;

    return (index >= num) ? WG_FALSE : WG_TRUE;
}

WG_PRIVATE wg_boolean
find_key(char *game_name, Config_field field[], wg_int num, wg_int *pos)
{
    wg_int index = 0;
    int compare_flag = -1;

    CHECK_FOR_NULL(game_name);
    CHECK_FOR_NULL(field);

    for (index = 0; (index < num) && (compare_flag != 0) ; ++index){
        switch (field[index].type){
          case CONFIG_FIELD_STRING:
              compare_flag = strcmp(game_name, field[index].name);
              break;
          case CONFIG_FIELD_INVALID:
          default:
              break;
        }
    }

    if (NULL != pos){
        *pos = index - 1;
    }

    return (compare_flag == 0) ? WG_TRUE : WG_FALSE;
}

/*! @} */
