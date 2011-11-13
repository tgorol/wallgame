#ifndef _GPM_H
#define _GPM_H


#define GETOPT_STRING ":i:h"

#define PROMPT_DEFAULT "wg> "

#define INI_BACKUP_DIR "/etc/"
#define INI_DEFAULT_NAME "wg_server.ini"

#define COMMAND_LINE_OPTION_SIZE 128

/**
 * @brief Application option entry
 */
typedef struct App_option_entry{
    char value[COMMAND_LINE_OPTION_SIZE + 1]; /*!< option value */
}App_option_entry;

/**
 * @brief Application options structure
 */
typedef struct App_options{
    App_option_entry ini_file_name;  /*!< config file path */
}App_options;

#endif
