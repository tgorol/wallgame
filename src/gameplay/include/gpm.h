#ifndef _GPM_H
#define _GPM_H


#define GETOPT_STRING ":i:h"

#define PROMPT_DEFAULT "wg> "

#define INI_BACKUP_DIR "/etc/"
#define INI_DEFAULT_NAME "wg_server.ini"

#define COMMAND_LINE_OPTION_SIZE 128

typedef struct App_option_entry{
    char value[COMMAND_LINE_OPTION_SIZE + 1];
}App_option_entry;

typedef struct App_options{
    App_option_entry ini_file_name;
}App_options;

#endif
