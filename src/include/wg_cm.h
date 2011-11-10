#ifndef _WG_CM_H
#define _WG_CM_H

#include <libini.h>

#define CONFIG_MAX_STRING_SIZE         128
#define CONFIG_MAX_FIELDS_NUM          8
#define CONFIG_MAX_FIELD_NAME_SIZE     32
#define CONFIG_MAX_SECTION_NAME_SIZE   32 
#define CONFIG_MAX_GAMES_NUM           (CONFIG_MAX_FIELDS_NUM)

/**
 * @brief Index of the field in a config section field array for a 
 * Server section.
 */
typedef enum CONFIG_SERVER_SECTION {
    PIPE_PATH       =  0        , 
    PLAYER_DB_PATH              ,
    SCORE_DB_PATH               ,
    CONFIG_SERVER_SECTION_SIZE
}CONFIG_SERVER_SECTION;

/**
 * @brief Index of the field in a config section field array for a 
 * Game section.
 */
typedef enum CONFIG_GAME_SECTION{
    GAME_PATH       =  0        , 
    GAME_ARGUMENTS              ,
    CONFIG_GAME_SECTION_SIZE    ,
    GAME_PLUGIN_NAME        
}CONFIG_GAME_SECTION;

/**
 * @brief Section type in a configuration file
 */
typedef enum CONFIG_SECTION_TYPE{
    INVALID_SECTION  = 0        ,   /*!< invalid section         */
    GAMES_LIST_SECTION          ,   /*!< section with game list  */
    SERVER_SECTION              ,   /*!< server specific section */
    GAME_SECTION                    /*!< game section            */
}CONFIG_SECTION_TYPE;

/**
 * @brief Configuration manager instance
 */
typedef struct Config {
    ini_fd_t ini_fd;    /*!< ini file instance */
} Config;

/**
 * @brief Type of the field in the ini file
 */
typedef enum CONFIG_FIELD_TYPE{
    CONFIG_FIELD_INVALID = 0 ,   /*!< invalid type */
    CONFIG_FIELD_STRING          /*!< string type  */
}CONFIG_FIELD_TYPE;

/**
 * @brief Defines a key in the ini file
 */
typedef struct Config_field{
    CONFIG_FIELD_TYPE type;                     /*!< type of the field */
    wg_char name[CONFIG_MAX_FIELD_NAME_SIZE];   /*!< name of the field */
    union{
        wg_char string[CONFIG_MAX_STRING_SIZE]; /*!< memory for string type */
    }value;
}Config_field;

/**
 * @brief Defines a section in the ini file
 */
typedef struct Config_section{
    wg_char name[CONFIG_MAX_FIELD_NAME_SIZE];   /*!< name of the section */
    wg_int size;                   /*!< number of elements is field(UNUSED) */
    CONFIG_SECTION_TYPE type;      /*!< type of the section                 */
    Config_field field[CONFIG_MAX_FIELDS_NUM]; /*!< list of fields in section */
}Config_section;

WG_PUBLIC wg_status
cm_read_section(Config *config, Config_section *section);

WG_PUBLIC wg_status
cm_init(wg_char *file_name, char *mode, Config *config);

WG_PUBLIC wg_status
cm_cleanup(Config *config);

WG_PUBLIC wg_status
cm_read_game_section(Config *config, char *name,
        Config_section *section);

WG_PUBLIC wg_status
cm_write_game(Config *config, char *game_name, Config_section *section);

#endif
